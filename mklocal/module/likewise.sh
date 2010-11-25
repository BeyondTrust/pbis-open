DEPENDS="path"

### section configure

option()
{
    _default_cachedir="${MK_LOCALSTATEDIR}/lib"
    _default_configdir="${MK_DATADIR}/config"

    [ "${MK_LOCALSTATEDIR}" = "/var" ] && _default_cachedir="/var/lib/likewise"

    mk_option \
        OPTION=lw-tool-dir \
        PARAM="name" \
        VAR=LW_TOOL_DIRNAME \
        DEFAULT="tools" \
        HELP="Subdirectory of build root where developer tools are placed"

    mk_option \
        OPTION=lw-cachedir \
        PARAM="path" \
        VAR=LW_CACHEDIR \
        DEFAULT="${_default_cachedir}" \
        HELP="Location of cache files"

    mk_option \
        OPTION=lw-configdir \
        PARAM="path" \
        VAR=LW_CONFIGDIR \
        DEFAULT="${_default_configdir}" \
        HELP="Location of registry files"
}

configure()
{
    mk_msg "cache dir: $LW_CACHEDIR"
    mk_msg "config dir: $LW_CONFIGDIR"
    mk_msg "developer tool dir: $LW_TOOL_DIRNAME"

    mk_export LW_CACHEDIR LW_CONFIGDIR
    mk_export LW_TOOL_DIR="@$LW_TOOL_DIRNAME"

    mk_add_scrub_target "$LW_TOOL_DIR"
}

lw_define_feature_macros()
{
    case "$MK_OS" in
        linux)
            mk_define _GNU_SOURCE
            mk_define _XOPEN_SOURCE 500
            mk_define _POSIX_C_SOURCE 200112L
            ;;
        solaris)
            mk_define _XOPEN_SOURCE 500
            mk_define __EXTENSIONS__
            mk_define _POSIX_PTHREAD_SEMANTICS
            ;;
    esac
}

lw_check_iconv()
{
    mk_check_headers FAIL=yes iconv.h
    mk_check_libraries iconv

    mk_msg_checking "iconv() input type"

    if ! mk_check_cache ICONV_IN_TYPE
    then
        if [ "$HAVE_LIB_ICONV" = "internal" ] || mk_check_function \
            PROTOTYPE='size_t iconv (iconv_t, char **, size_t *, char **, size_t*)' \
            HEADERDEPS="iconv.h stddef.h" \
            LIBDEPS="$LIB_ICONV"
        then
            result="char**"
        elif mk_check_function \
            PROTOTYPE='size_t iconv (iconv_t, const char **, size_t *, char **, size_t*)' \
            HEADERDEPS="iconv.h stddef.h" \
            LIBDEPS="$LIB_ICONV"
        then
            result="const char**"
        else
            mk_fail "could not find usable iconv() function"
        fi
        mk_cache ICONV_IN_TYPE "$result"
    fi

    mk_msg_result "$ICONV_IN_TYPE"
    mk_define ICONV_IN_TYPE "$ICONV_IN_TYPE"
}