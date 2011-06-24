DEPENDS="path"

### section configure

option()
{
    _default_cachedir="${LOCALSTATEDIR}/lib"
    _default_configdir="${DATADIR}/config"

    case "$LOCALSTATEDIR" in
        "/var")
            _default_cachedir="/var/lib/likewise"
            ;;
        "/private/var")
            _default_cachedir="/private/var/lib/likewise"
            ;;
    esac

    case "$MK_HOST_OS" in
        aix)
            _default_initdir="/etc/rc.d/init.d"
            ;;
        freebsd)
            _default_initdir="/etc/rc.d"
            ;;
        hpux)
            _default_initdir="/sbin/init.d"
            ;;
        *)
            _default_initdir="/etc/init.d"
            ;;
    esac

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

    mk_option \
        OPTION=lw-initdir \
        PARAM="path" \
        VAR=LW_INITDIR \
        DEFAULT="${_default_initdir}" \
        HELP="Location where init scripts should be installed"

    mk_option \
        OPTION=lw-device-profile \
        VAR=LW_DEVICE_PROFILE \
        PARAM="profile" \
        DEFAULT="default" \
        HELP="Device profile (default, embedded)"

    mk_option \
        OPTION=lw-feature-level \
        VAR=LW_FEATURE_LEVEL \
        PARAM="level" \
        DEFAULT="full" \
        HELP="Feature level (full, auth)"
}

configure()
{
    mk_msg "cache dir: $LW_CACHEDIR"
    mk_msg "config dir: $LW_CONFIGDIR"
    mk_msg "init script dir: $LW_INITDIR"
    mk_msg "developer tool dir: $LW_TOOL_DIRNAME"

    LW_TOOL_DIR="@$LW_TOOL_DIRNAME"
    _LW_TOOL_TARGETS=""
}

lw_add_tool_target()
{
    mk_push_vars result
    mk_resolve_target "$1"
    mk_quote "$result"
    _LW_TOOL_TARGETS="$_LW_TOOL_TARGETS $result"
    mk_pop_vars
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
            if [ "$MK_ARCH" = "ia64" ]
            then
                mk_define _XOPEN_SOURCE 500
            fi
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

lw_check_libxml2()
{
    # libxml2 puts headers in non-standard locations
    # Figure out if we build it ourselves
    XML_INCDIR="${MK_INCLUDEDIR}/libxml2"

    mk_msg_checking "internal libxml2"

    if mk_check_header \
        HEADER="${XML_INCDIR}/libxml/tree.h" &&
        [ "$result" = "internal" ]
    then
        mk_msg_result "yes"
        # Look for the headers in our own staging area
        LIBXML2_INCLUDEDIRS="${XML_INCDIR}"
        LIBXML2_HEADERDEPS="${XML_INCDIR}/libxml/tree.h \
                            ${XML_INCDIR}/libxml/xpath.h \
                            ${XML_INCDIR}/libxml/parser.h \
                            ${XML_INCDIR}/libxml/xpathInternals.h"
        
        # Go through the usual checks
        mk_check_headers FAIL=yes ${LIBXML2_HEADERDEPS}
        mk_check_libraries FAIL=yes xml2

        mk_declare -i LIBXML2_INCLUDEDIRS LIBXML2_HEADERDEPS
    else
        mk_msg_result "no"

        # Let pkg-config figure it out
        if ! mk_pkg_config \
	    VARPREFIX=LIBXML2 \
            libxml-2.0
        then
            # One last try
            mk_declare -i LIBXML2_CPPFLAGS="-I/usr/include/libxml2"
            mk_check_headers \
                FAIL=yes \
                CPPFLAGS="$LIBXML2_CPPFLAGS" \
                libxml/tree.h libxml/xpath.h libxml/parser.h libxml/xpathInternals.h
            mk_check_libraries FAIL=yes xml2
        fi
    fi
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

lw_service()
{
    mk_push_vars SERVICE SOURCES GROUPS HEADERDEPS LIBDEPS INCLUDEDIRS CPPFLAGS LDFLAGS CFLAGS CXXFLAGS DEPS
    mk_parse_params

    if [ "$LW_DEVICE_PROFILE" = "embedded" ]
    then
        mk_group \
            GROUP="$SERVICE" \
            SOURCES="$SOURCES" \
            GROUPDEPS="$GROUPS" \
            HEADERDEPS="$HEADERDEPS" \
            LIBDEPS="$LIBDEPS" \
            INCLUDEDIRS="$INCLUDEDIRS" \
            CPPFLAGS="$CPPFLAGS" \
            CFLAGS="$CFLAGS" \
            CXXFLAGS="$CXXFLAGS" \
            LDFLAGS="$LDFLAGS" \
            DEPS="$DEPS"
    else
        mk_dlo \
            INSTALLDIR="$MK_LIBDIR/lw-svcm" \
            DLO="$SERVICE" \
            SOURCES="$SOURCES" \
            GROUPS="$GROUPS" \
            HEADERDEPS="$HEADERDEPS" \
            LIBDEPS="$LIBDEPS" \
            INCLUDEDIRS="$INCLUDEDIRS" \
            CPPFLAGS="$CPPFLAGS" \
            CFLAGS="$CFLAGS" \
            CXXFLAGS="$CXXFLAGS" \
            LDFLAGS="$LDFLAGS" \
            DEPS="$DEPS"
    fi

    mk_pop_vars
}

make()
{
    mk_target \
        TARGET="${LW_TOOL_DIR}" \
        DEPS="$_LW_TOOL_TARGETS"

    mk_add_phony_target "$result"
    mk_add_scrub_target "$result"
}
