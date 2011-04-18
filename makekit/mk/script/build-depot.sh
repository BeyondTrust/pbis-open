_build_depot()
{
    # $1 = package output dir
    # $2 = package version
    # $3 = work directory
    # ... = filesets

    DEPOT_OUTPUT="$1"
    DEPOT_VER="$2"
    DEPOT_DIR="$3"
    DEPOT_NAME="${DEPOT_OUTPUT#$MK_PACKAGE_DEPOT_DIR/}"

    DEPOT_BUILD="$DEPOT_DIR/build"
    DEPOT_IMAGE="$DEPOT_BUILD/image"

    _depot_name "$DEPOT_NAME"
    DEPOT_NAMEREAL="$result"

    shift 3

    mk_msg_domain "depot"

    mk_msg "begin $DEPOT_NAME"

    mk_safe_rm "$DEPOT_BUILD"

    # Build image

    mk_msg "$DEPOT_NAMEREAL image"

    mk_mkdir "$DEPOT_IMAGE"
    cat "$DEPOT_DIR"/*.dirs | while mk_read_line
    do
        mk_mkdir "${DEPOT_IMAGE}${result}"
    done

    cat "$DEPOT_DIR"/*.files | while mk_read_line
    do
        _transfer "${MK_STAGE_DIR}${result}" "${DEPOT_IMAGE}${result}"
    done
    
    mk_msg "$DEPOT_NAMEREAL meta"

    # Construct file lists for insertion into psf
    for _fileset
    do
        {
            echo "file_permissions -u 0022 -o root -g root"
            {
                while mk_read_line
                do
                    echo "file ${DEPOT_IMAGE}${result} ${result}"
                done
            } < "$DEPOT_DIR/${_fileset}.dirs"

            {
                while mk_read_line
                do
                    echo "file ${DEPOT_IMAGE}${result} ${result}"
                done
            } < "$DEPOT_DIR/${_fileset}.files"
        } > "$DEPOT_BUILD/${_fileset}.subst"
    done
    
    # Construct sed script to perform insertion
    {
        for _fileset
        do
            _mk_define_name "$_fileset"
            _var="DEPOT_FILESET_$result"
            echo "/@$_var@/ {"
            echo "    r ${DEPOT_BUILD}/${_fileset}.subst"
            echo "    d"
            echo "}"
        done
    } > "${DEPOT_BUILD}/subst.sed"
    
    # Generate final psf
    mk_run_or_fail sed -f "${DEPOT_BUILD}/subst.sed" \
        < "${DEPOT_DIR}/product.psf" \
        > "${DEPOT_BUILD}/product.psf"

    # Build depot file
    mk_msg "$DEPOT_NAMEREAL depot"
    mk_mkdir "$DEPOT_OUTPUT"

    mk_run_quiet_or_fail ${SWPACKAGE} \
        -d "$DEPOT_OUTPUT/$DEPOT_NAMEREAL-$DEPOT_VER.$MK_HOST_ARCH.depot" \
        -s "${DEPOT_BUILD}/product.psf" \
        -x target_type=tape
    
    mk_msg "wrote $DEPOT_NAMEREAL-$DEPOT_VER.$MK_HOST_ARCH.depot"
    mk_msg "end $DEPOT_NAME"
}

_transfer()
{
    mk_mkdirname "$2"
    mk_run_or_fail cp -R "$1" "$2"
    
    if [ "$MK_DEBUG" = "no" ]
    then
        case "`file -h "$2"`" in
            *"not stripped"|*"ELF-32"*|*"ELF-64"*)
                mk_get_file_mode "$2"
                _old_mode="$result"
                mk_run_or_fail chmod u+w "$2"
                mk_run_or_fail strip "$2"
                mk_run_or_fail chmod "$_old_mode" "$2"
                ;;
        esac
    fi
}

_build_depot "$@"