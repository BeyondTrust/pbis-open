DEPENDS="package"

### section configure

lw_sfx()
{
    mk_push_vars FILES SCRIPT SFX PKGDIRS FORMAT BASE UPGRADE OBSOLETE GUI
    mk_parse_params

    for _pkgname in "$@"
    do
        mk_quote "@${MK_PACKAGE_DIR}/${FORMAT}/${_pkgname}"
        PKGDIRS="$PKGDIRS $result"
    done

    mk_target \
        TARGET="@sfx/$SFX.$MK_HOST_OS.$MK_HOST_ARCH.$FORMAT.sh" \
        DEPS="$PKGDIRS" \
        _lw_sfx %FILES %SCRIPT %FORMAT %BASE %UPGRADE %OBSOLETE %GUI \
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

_lw_sfx()
{
    mk_push_vars FILES SCRIPT SFX LABEL
    mk_parse_params

    mk_msg_domain "sfx"

    SFX="$1"
    shift

    LABEL="${SFX##*/}"
    LABEL="${LABEL%.*}"

    mk_resolve_resource "makeself/makeself.sh"
    MAKESELF_SH="$result"
    
    mk_resolve_resource "makeself/makeself-header.sh"
    MAKESELF_HEADER="$result"

    mk_resolve_file ".sfx-temp-$$"
    TEMP_DIR="$result"

    mk_safe_rm "$TEMP_DIR"
    mk_mkdir "$TEMP_DIR/${LABEL}"

    for _file in ${FILES} ${SCRIPT}
    do
        mk_resolve_file "$_file"
        mk_run_or_fail cp "$result" "$TEMP_DIR/${LABEL}/"
    done

    mk_mkdir "$TEMP_DIR/${LABEL}/packages"

    for _pkgdir in "$@"
    do
        mk_run_or_fail cp "$_pkgdir"/* "$TEMP_DIR/${LABEL}/packages/"
    done

    {
        echo "PREFIX=\"$MK_PREFIX\""
        echo "INSTALL_UPGRADE_PACKAGE=\"$UPGRADE\""
        echo "INSTALL_OBSOLETE_PACKAGES=\"$OBSOLETE\""
        echo "INSTALL_BASE_PACKAGE=\"$BASE\""
        echo "INSTALL_GUI_PACKAGE=\"$GUI\""
    } > "$TEMP_DIR/${LABEL}/MANIFEST" || mk_fail "could not write manifest"
    
    mk_mkdirname "$SFX"

    TMP_DIR="$TEMP_DIR"
    export TMP_DIR

    mk_msg "${SFX#sfx/}"

    mk_run_quiet_or_fail \
        "$MAKESELF_SH" \
        --nox11 --notemp \
        --header "$MAKESELF_HEADER" \
        "$TEMP_DIR/${LABEL}" \
        "$SFX" \
        "$LABEL" \
        "./`basename $SCRIPT`" \
        --echo-dir "$LABEL"
}
