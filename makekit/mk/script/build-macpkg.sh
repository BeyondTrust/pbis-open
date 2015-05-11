_build_macpkg()
{
    # $1 = package output dir
    # $2 = package version
    # $3 = work directory
    # ... = subpackages

    MACPKG_OUTPUT="$1"
    MACPKG_VER="$2"
    MACPKG_DIR="$3"
    MACPKG_NAME="${MACPKG_OUTPUT#$MK_PACKAGE_MAC_DIR/}"

    shift 3

    mk_msg_domain "macpkg"

    mk_msg "begin $MACPKG_NAME"

    # Build image directories

    for _sub
    do
        mk_msg "$MACPKG_NAME $_sub"

        MACPKG_IMAGE="$MACPKG_DIR/$_sub"

        mk_safe_rm "$MACPKG_IMAGE"
        mk_mkdir "$MACPKG_IMAGE"

        cat "$MACPKG_DIR/${_sub}.dirs" | while mk_read_line
        do
            mk_mkdir "${MACPKG_IMAGE}${result}"
        done

        cat "$MACPKG_DIR/${_sub}.files" | while mk_read_line
        do
            _transfer "${MK_STAGE_DIR}${result}" "${MACPKG_IMAGE}${result}"
        done
    done
    
    mk_msg "$MACPKG_NAME packagemaker"

    mk_mkdir "$MACPKG_OUTPUT"

    mk_run_or_fail ${PACKAGEMAKER} \
        --doc "$MACPKG_DIR/dist.pmdoc" \
        -w \
        -o "$MACPKG_OUTPUT/$MACPKG_NAME-$MACPKG_VER.pkg"
    
    mk_msg "wrote $MACPKG_NAME-$MACPKG_VER.pkg"
    mk_msg "end $MACPKG_NAME"
}

_transfer()
{
    mk_mkdirname "$2"
    mk_run_or_fail cp -RP "$1" "$2"
    
    if [ "$MK_DEBUG" = "no" ]
    then
        case "`file -h "$2"`" in
            *"Mach-O"*)
                mk_get_file_mode "$2"
                _old_mode="$result"
                mk_run_or_fail chmod u+w "$2"
                mk_run_or_fail strip -S -x "$2"
                mk_run_or_fail chmod "$_old_mode" "$2"
                ;;
        esac
    fi
}

_build_macpkg "$@"
