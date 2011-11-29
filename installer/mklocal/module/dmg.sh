DEPENDS="package-mac"

### section configure

lw_dmg()
{
    mk_push_vars DMG VOLNAME PACKAGES TEMPLATE
    mk_parse_params

    mk_unquote_list "$PACKAGES"
    for _pkgname
    do
        mk_quote "@${MK_PACKAGE_DIR}/mac/${_pkgname}"
        PKGDIRS="$PKGDIRS $result"
    done

    mk_target \
        TARGET="@dmg/$DMG.dmg" \
        DEPS="$PKGDIRS" \
        _lw_dmg '$@' "$VOLNAME" "&$TEMPLATE" "*$PKGDIRS"

    _dmg_target="$result"
    mk_quote "$result"
    _MK_DMG_TARGETS="$_MK_DMG_TARGETS $result"

    mk_pop_vars

    result="$_dmg_target"
}

make()
{
    # Allow 'make dmg' to build all dmg installers
    mk_target \
        TARGET="@dmg" \
        DEPS="$_MK_DMG_TARGETS"

    mk_add_phony_target "$result"

    # Remove dmg directory on "make scrub"
    mk_add_scrub_target "@dmg"
}

### section build

_uncompress()
{
    case "$1" in
        *.bz2)
            mk_run_or_fail bunzip2 < "$1"
            ;;
        *.gz)
            mk_run_or_fail gunzip < "$1"
            ;;
        *)
            mk_fail "unknown archive format: ${1##*.}"
            ;;
    esac
}

_unpack()
{
    _uncompress "$1" | { cd "$2" && tar xf -; } || mk_fail "could not unpack $1"
}

_lw_dmg()
{
    # $1 = output
    # $2 = volume name
    # $3 = template
    # ... = package dirs

    DMG_OUTPUT="$1"
    DMG_VOLNAME="$2"
    DMG_TEMPLATE="$3"
    
    shift 3

    mk_msg_domain "dmg"

    mk_msg "${DMG_OUTPUT#dmg/}"

    mk_resolve_file ".dmg-temp-$$"
    TEMP_DIR="$result"

    mk_quote "$TEMP_DIR"
    trap "mk_safe_rm $result" 0

    mk_mkdir "$TEMP_DIR"
    #_unpack "$DMG_TEMPLATE" "$TEMP_DIR"
    mk_mkdir "$TEMP_DIR/Installer"
    
    for _pkgdir
    do
        mk_run_or_fail cp "$_pkgdir"/*.pkg "$TEMP_DIR/Installer/"
    done

    mk_run_or_fail chmod -Rf go-w "$TEMP_DIR"

    mk_mkdirname "$DMG_OUTPUT"
    mk_safe_rm "$DMG_OUTPUT"
   
    mk_run_quiet_or_fail hdiutil create \
        -fs HFS+ \
        -srcfolder "$TEMP_DIR" \
        -format UDZO \
        -imagekey zlib-level=9 \
        -volname "$DMG_VOLNAME" \
        -o "$DMG_OUTPUT"
}
