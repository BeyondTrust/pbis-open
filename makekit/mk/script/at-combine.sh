_mk_autotools_combine_darwin()
{
    stamp="$1"
    primary="$2"
    shift

    mk_quote_list "$@"
    dirs="$result"

    _IFS="$IFS"
    IFS='
'
    set -e
    set -- $(cd "$primary" && find .) || mk_fail "could not enumerate $primary"   
    set +e
    IFS="$_IFS"

    # All of the files this function installs (staged files) are marked as
    # depending on the stamp file inside of the Makefile. So the stamp file
    # must be older than the staged files. Creating the file beforehand and
    # moving it into place afterwards insures the correct time.
    mk_run_or_fail touch "$stamp.running"

    for file
    do
        case `file -h "$primary/$file"` in
            *": directory" )
                mk_mkdir "${MK_STAGE_DIR}/${file}"
                ;;
            *"Mach-O"*)
                mk_msg "${file#.}"
                mk_quote "$MK_STAGE_DIR/$file"
                command="lipo -create -output $result"
                mk_unquote_list "$dirs"
                for dir
                do
                    mk_quote "$dir/$file"
                    command="$command $result"
                done

                mk_unquote_list "$command"
                mk_run_or_fail "$@"
                ;;
            *)
                mk_msg "${file#.}"
                mk_run_or_fail cp -RpPf -- "$primary/$file" "${MK_STAGE_DIR}/${file}"
                # Update the timestamp of the staged file so the stamp file is
                # older
                mk_run_or_fail touch -- "${MK_STAGE_DIR}/${file}"
                ;;
        esac
    done

    mk_run_or_fail mv -f "$stamp.running" "$stamp"
}

mk_msg_domain combine

if [ -n "$SOURCEDIR" ]
then
    dirname="${MK_SUBDIR:+${MK_SUBDIR#/}/}$SOURCEDIR"
elif [ -n "$MK_SUBDIR" ]
then
    dirname="${MK_SUBDIR#/}"
else
    dirname="$PROJECT_NAME"
fi

__msg="$dirname ($MK_SYSTEM)"

mk_msg "begin ${__msg}"

case "$MK_OS" in
    darwin)
        _mk_autotools_combine_darwin "$@"
        ;;
    *)
        mk_fail "unsupported OS"
        ;;
esac

mk_msg "end ${__msg}"
