DEPENDS="path"

### section configure

option()
{
    _default_cachedir="${MK_LOCALSTATEDIR}/lib"
    _default_configdir="${MK_DATADIR}/config"

    [ "${MK_LOCALSTATEDIR}" = "/var" ] && _default_cachedir="/var/lib/likewise"
    [ "${MK_DATADIR}" = "/usr/share" ] && _default_configdir="${MK_DATADIR}/likewise/config"

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

    mk_export LW_CACHEDIR LW_CONFIGDIR
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
            ;;
    esac
}

lw_check_iconv()
{
    mk_check_header FAIL=yes HEADER=iconv.h
    mk_check_library LIB=iconv

    if [ "$HAVE_LIB_ICONV" = "internal" ] || mk_check_function \
        PROTOTYPE='size_t iconv (iconv_t, const char **, size_t *, char **, size_t*)' \
        HEADERDEPS="iconv.h stddef.h" \
        LIBDEPS="$LIB_ICONV"
    then
        mk_define ICONV_IN_TYPE 'const char**'
    elif mk_check_function \
        PROTOTYPE='size_t iconv (iconv_t, char **, size_t *, char **, size_t*)' \
        HEADERDEPS="iconv.h stddef.h" \
        LIBDEPS="$_lib_iconv"
    then
        mk_define ICONV_IN_TYPE 'char**'
    else
        mk_fail "could not find usable iconv() function"
    fi
}