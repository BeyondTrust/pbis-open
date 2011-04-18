DEPENDS="package"

### section configure

lw_sfx()
{
    mk_push_vars FILES PACKAGES SFX PKGDIRS FORMAT BASE UPGRADE OBSOLETE GUI
    mk_parse_params

    mk_unquote_list "$PACKAGES"
    for _pkgname
    do
        mk_quote "@${MK_PACKAGE_DIR}/${FORMAT}/${_pkgname}"
        PKGDIRS="$PKGDIRS $result"
    done

    mk_target \
        TARGET="@sfx/$SFX.$MK_HOST_OS.$MK_HOST_ARCH.$FORMAT.sh" \
        DEPS="$PKGDIRS $FILES" \
        _lw_sfx %FILES %FORMAT %BASE %UPGRADE %OBSOLETE %GUI \
        '$@' "*$PKGDIRS"

    _sfx_target="$result"
    mk_quote "$result"
    _MK_SFX_TARGETS="$_MK_SFX_TARGETS $result"

    mk_pop_vars

    result="$_sfx_target"
}

make()
{
    # Allow 'make sfx' to build all sfx installers
    mk_target \
        TARGET="@sfx" \
        DEPS="$_MK_SFX_TARGETS"

    mk_add_phony_target "$result"

    # Remove sfx directory on "make scrub"
    mk_add_scrub_target "@sfx"
}

### section build

_lw_package_native_name()
{
    case "$FORMAT" in
        lpp)
            result=`echo "$1" | tr '-' '.'`
            ;;
        depot)
            _IFS="$IFS"
            IFS="-"
            set -- $1
            IFS="$_IFS"
            result=""
            for part
            do
                rest="${part#?}"
                first=`echo "${part%$rest}" | tr '[:lower:]' '[:upper:]'`
                result="$result$first$rest"
            done
            ;;
        *)
            result="$1"
            ;;
    esac
}

_lw_package_native_list()
{
    _list=""
    for _name
    do
        _lw_package_native_name "$_name"
        _list="$_list $result"
    done

    result="${_list# }"
}

_lw_sfx()
{
    mk_push_vars FILES SCRIPT SFX LABEL BASE FORMAT OBSOLETE GUI
    mk_parse_params

    mk_msg_domain "sfx"

    SFX="$1"
    shift

    mk_msg "${SFX#sfx/}"

    LABEL="${SFX##*/}"
    LABEL="${LABEL%.*}"

    mk_resolve_resource "makeself/makeself.sh"
    MAKESELF_SH="$result"
    
    mk_resolve_resource "makeself/makeself-header.sh"
    MAKESELF_HEADER="$result"

    mk_resolve_resource "makeself/install.sh"
    INSTALL_SH="$result"

    mk_resolve_file ".sfx-temp-$$"
    TEMP_DIR="$result"

    mk_quote "$TEMP_DIR"
    trap "mk_safe_rm $result" 0

    mk_safe_rm "$TEMP_DIR"
    mk_mkdir "$TEMP_DIR/$LABEL"

    mk_run_or_fail cp "$INSTALL_SH" "$TEMP_DIR/$LABEL/"

    for _file in ${FILES}
    do
        mk_resolve_file "$_file"
        mk_run_or_fail cp "$result" "$TEMP_DIR/$LABEL/"
    done

    mk_mkdir "$TEMP_DIR/$LABEL/packages"

    for _pkgdir
    do
        mk_run_or_fail cp "$_pkgdir"/* "$TEMP_DIR/$LABEL/packages/"
    done

    {
        echo "PREFIX=\"$MK_PREFIX\""
        _lw_package_native_list ${UPGRADE}
        echo "INSTALL_UPGRADE_PACKAGE=\"$result\""
        _lw_package_native_list ${OBSOLETE}
        echo "INSTALL_OBSOLETE_PACKAGES=\"$result\""
        _lw_package_native_list ${BASE}
        echo "INSTALL_BASE_PACKAGE=\"$result\""
        _lw_package_native_list ${GUI}
        echo "INSTALL_GUI_PACKAGE=\"$result\""
    } > "$TEMP_DIR/$LABEL/MANIFEST" || mk_fail "could not write manifest"
    
    mk_mkdirname "$SFX"

    TMP_DIR="$TEMP_DIR"
    export TMP_DIR

    mk_run_quiet_or_fail \
        "$MAKESELF_SH" \
        --nox11 --notemp \
        --header "$MAKESELF_HEADER" \
        "$TEMP_DIR/$LABEL" \
        "$SFX" \
        "$LABEL" \
        "./install.sh" \
        --echo-dir "$LABEL"
}
