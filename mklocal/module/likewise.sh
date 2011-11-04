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
            mk_define __LWI_LINUX__
            mk_define _GNU_SOURCE
            mk_define _XOPEN_SOURCE 500
            mk_define _POSIX_C_SOURCE 200112L
            mk_define _REENTRANT
            ;;
        solaris)
            mk_define __LWI_SOLARIS__
            mk_define _XOPEN_SOURCE 500
            mk_define __EXTENSIONS__
            mk_define _POSIX_PTHREAD_SEMANTICS
            mk_define _REENTRANT
            ;;
        freebsd)
            mk_define __LWI_FREEBSD__
            ;;
        darwin)
            mk_define __LWI_DARWIN__
            ;;
        aix)
            mk_define __LWI_AIX__
            mk_define _THREAD_SAFE
            ;;
        hpux)
            mk_define __LWI_HP_UX__
            mk_define _HPUX_SOURCE 1
            mk_define _REENTRANT
            mk_define _XOPEN_SOURCE_EXTENDED 1
            # HACK HACK HACK
            mk_write_config_header "union mpinfou {};"
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
        # GNU libiconv attempts to mimic the prototype
        # of the system iconv() function, or falls back
        # on char** if it could not find one.  Even if
        # we build libiconv ourselves, we still need to
        # perform these checks so we know what prototype it
        # will use.  This is why we use LDFLAGS to attempt
        # linking to the system libiconv
        if [ "$HAVE_LIB_ICONV" = "internal" ]
        then
            if mk_check_function \
                FUNCTION="iconv_open" \
                HEADERDEPS="iconv.h stddef.h" \
                LDFLAGS="-liconv"
            then
                _ICONV_LDFLAGS="-liconv"
            else
                _ICONV_LDFLAGS=""
            fi
        fi

        if mk_check_function \
            PROTOTYPE='size_t iconv (iconv_t, char **, size_t *, char **, size_t*)' \
            HEADERDEPS="iconv.h stddef.h" \
            LDFLAGS="$_ICONV_LDFLAGS"
        then
            result="char**"
        elif mk_check_function \
            PROTOTYPE='size_t iconv (iconv_t, const char **, size_t *, char **, size_t*)' \
            HEADERDEPS="iconv.h stddef.h" \
            LDFLAGS="$_ICONV_LDFLAGS"
        then
            result="const char**"
        elif [ "$HAVE_LIB_ICONV" = "internal" ]
        then
            # We didn't find either function, but
            # we build libiconv ourselves, so it
            # should fall back on char**
            result="char**"
        else
            mk_fail "could not find usable iconv() function"
        fi
        mk_cache ICONV_IN_TYPE "$result"
    fi

    mk_msg_result "$ICONV_IN_TYPE"
    mk_define ICONV_IN_TYPE "$ICONV_IN_TYPE"
}

lw_check_strerror_r()
{
    mk_msg_checking "return type of strerror_r"

    if ! mk_check_cache STRERROR_R_RETURN_TYPE
    then
        # Check which version of strerror_r we have
        if mk_check_function \
            PROTOTYPE="char* strerror_r(int,char*,size_t)" \
            HEADERDEPS="string.h"
        then
            result="char*"
        elif mk_check_function \
            PROTOTYPE="int strerror_r(int,char*,size_t)" \
            HEADERDEPS="string.h"
        then
            result="int"
        else
            result="none"
        fi
        mk_cache STRERROR_R_RETURN_TYPE "$result"
    fi

    mk_msg_result "$STRERROR_R_RETURN_TYPE"

    case "$STRERROR_R_RETURN_TYPE" in
        "char*")
            mk_define STRERROR_R_CHAR_P 1
            ;;
        "int")
            mk_define STRERROR_R_INT 1
            ;;
    esac
}

lw_check_pthread_once_init()
{
    mk_msg_checking "broken PTHREAD_ONCE_INIT"

    if ! mk_check_cache HAVE_BROKEN_ONCE_INIT
    then
        if mk_try_compile \
            CODE="pthread_once_t once = PTHREAD_ONCE_INIT; *(&once) = once; return 0;" \
            CFLAGS="-Wall -Werror" \
            HEADERDEPS="pthread.h"
        then
            result="no"
        else
            result="yes"
        fi
        
        mk_cache HAVE_BROKEN_ONCE_INIT "$result"
    fi

    mk_msg_result "$HAVE_BROKEN_ONCE_INIT"

    if [ "$HAVE_BROKEN_ONCE_INIT" = "yes" ]
    then
        mk_define BROKEN_ONCE_INIT 1
    else
        mk_define BROKEN_ONCE_INIT 0
    fi

    result="$HAVE_BROKEN_ONCE_INIT"
}