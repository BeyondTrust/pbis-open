AC_INIT(lwupgrade, 5.4, support@likewisesoftware.com)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_MACRO_DIR(m4])

LWUPGRADE_CPPFLAGS=""
LWUPGRADE_CFLAGS="-Wall -Werror -fno-strict-aliasing"
LWUPGRADE_LDFLAGS=""

case "$host_os:$host_cpu" in
    linux*:*)
        AC_DEFINE([__LWI_LINUX___], [], [Defined if OS is Linux])
        ;;
    solaris*:*)
        AC_DEFINE([__LWI_SOLARIS__], [], [Define if OS is Solaris])
        ;;
    darwin*:*)
        AC_DEFINE([__LWI_DARWIN__], [], [Define if OS is Darwin])
        ;;
    freebsd*:*)
        AC_DEFINE([__LWI_FREEBSD__], [], [Define if OS is FreeBSD])
        ;;
	hpux*:hppa*)
        AC_DEFINE([__LWI_HP_UX__], [], [Define if OS is HP-UX])
        AC_DEFINE([_XOPEN_SOURCE_EXTENDED], [1], [Define on HP-UX])
		;;
	hpux*:ia64*)
        AC_DEFINE([__LWI_HP_UX__], [], [Define if OS is HP-UX])
        AC_DEFINE([_XOPEN_SOURCE_EXTENDED], [1], [Define on HP-UX])
		;;
	aix*:*)
        AC_DEFINE([__LWI_AIX__], [], [Define if OS is AIX])
        AC_DEFINE([_LINUX_SOURCE_COMPAT], [], [Enable Linux source compatibility on AIX])
        ;;
esac


# debugging

AC_ARG_ENABLE([debug],
        [AC_HELP_STRING([--enable-debug], [enable debugging (default: disabled)])],
        [
                if test x"$enableval" = x"yes"
                then
                    LWUPGRADE_CFLAGS="$LWUPGRADE_CFLAGS -g -O0"
                    LWUPGRADE_CPPFLAGS="$LWUPGRADE_CPPFLAGS -DDEBUG"
                fi
        ])

AC_ARG_WITH([lwadvapi],
            [AC_HELP_STRING([--with-lwadvapi=<dir>], [use lwadvapi located in prefix <dir>])],
            [
                LWADVAPI_INCLUDES="-I$withval/include"
                LWADVAPI_LDFLAGS="-L$withval/lib"
            ])
AC_ARG_WITH([lwadvapi-includes],
            [AC_HELP_STRING([--with-lwadvapi-includes=<dir>], [use lwadvapi includes located in <dir>])],
            [
             LWADVAPI_INCLUDES="-I$withval"
            ])
AC_ARG_WITH([lwadvapi-libs],
            [AC_HELP_STRING([--with-lwadvapi-libs=<dir>], [use lwadvapi libs located in <dir>])],
            [
             LWADVAPI_LDFLAGS="-L$withval"
             ])

LWADVAPI_LIBS="-llwadvapi"
LWADVAPI_NOTHR_LIBS="-llwadvapi_nothr"

AC_SUBST(LWADVAPI_INCLUDES)
AC_SUBST(LWADVAPI_LDFLAGS)
AC_SUBST(LWADVAPI_LIBS)
AC_SUBST(LWADVAPI_NOTHR_LIBS)
LWUPGRADE_CPPFLAGS="$LWUPGRADE_CPPFLAGS $LWADVAPI_INCLUDES"
LWUPGRADE_LDFLAGS="$LWUPGRADE_LDFLAGS $LWADVAPI_LDFLAGS $LWADVAPI_LIBS $LWADVAPI_NOTHR_LIBS"


# lwbase

AC_ARG_WITH([lwbase],
        [AC_HELP_STRING([--with-lwbase=<dir>], [use lwbase located in prefix <dir>])],
        [
		LWBASE_INCLUDES="-I$withval/include"
        LWBASE_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([lwbase-includes],
        [AC_HELP_STRING([--with-lwbase-includes=<dir>], [use lwbase includes located in <dir>])],
        [
		LWBASE_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([lwbase-libs],
        [AC_HELP_STRING([--with-lwbase-libs=<dir>], [use lwbase libs located in <dir>])],
        [
		LWBASE_LDFLAGS="-L$withval"
        ])
LWBASE_LIBS=-llwbase

AC_SUBST(LWBASE_INCLUDES)
AC_SUBST(LWBASE_LDFLAGS)
AC_SUBST(LWBASE_LIBS)
LWUPGRADE_CPPFLAGS="$LWUPGRADE_CPPFLAGS $LWBASE_INCLUDES"
LWUPGRADE_LDFLAGS="$LWUPGRADE_LDFLAGS $LWBASE_LDFLAGS $LWBASE_LIBS"


# sqlite

AC_ARG_WITH([sqlite],
        [AC_HELP_STRING([--with-sqlite=<dir>], [use sqlite located in prefix <dir>])],
        [
            SQLITE_INCLUDES="-I$withval/include"
            SQLITE_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([sqlite-includes],
        [AC_HELP_STRING([--with-sqlite-includes=<dir>], [use sqlite includes located in <dir>])],
        [
            SQLITE_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([sqlite-libs],
        [AC_HELP_STRING([--with-sqlite-libs=<dir>], [use sqlite libs located in <dir>])],
        [
            SQLITE_LDFLAGS="-L$withval"
        ])

SQLITE_LIBS="-lsqlite3"

AC_SUBST(SQLITE_INCLUDES)
AC_SUBST(SQLITE_LDFLAGS)
AC_SUBST(SQLITE_LIBS)

# pstore

AC_ARG_WITH([pstore],
        [AC_HELP_STRING([--with-pstore=<dir>], [use pstore located in prefix <dir>])],
        [
        PSTORE_INCLUDES="-I$withval/include"
        PSTORE_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([pstore-includes],
        [AC_HELP_STRING([--with-pstore-includes=<dir>], [use pstore includes located in <dir>])],
        [
        PSTORE_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([pstore-libs],
        [AC_HELP_STRING([--with-pstore-libs=<dir>], [use pstore libs located in <dir>])],
        [
        PSTORE_LDFLAGS="-L$withval"
        ])

PSTORE_LIBS="-llwpsapi"

AC_SUBST(PSTORE_INCLUDES)
AC_SUBST(PSTORE_LDFLAGS)
AC_SUBST(PSTORE_LIBS)
LWUPGRADE_CPPFLAGS="$LWUPGRADE_CPPFLAGS $PSTORE_INCLUDES"
LWUPGRADE_LDFLAGS="$LWUPGRADE_LDFLAGS $PSTORE_LDFLAGS $PSTORE_LIBS"



# lwreg

AC_ARG_WITH([lwreg],
        [AC_HELP_STRING([--with-lwreg=<dir>], [use lwreg located in prefix <dir>])],
        [
        LWREG_INCLUDES="-I$withval/include"
        LWREG_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([lwreg-includes],
        [AC_HELP_STRING([--with-lwreg-includes=<dir>], [use lwreg includes located in <dir>])],
        [
        LWREG_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([lwreg-libs],
        [AC_HELP_STRING([--with-lwreg-libs=<dir>], [use lwreg libs located in <dir>])],
        [
        LWREG_LDFLAGS="-L$withval"
        ])

LWREG_LIBS=-lregclient

AC_SUBST(LWREG_INCLUDES)
AC_SUBST(LWREG_LDFLAGS)
AC_SUBST(LWREG_LIBS)
LWUPGRADE_CPPFLAGS="$LWUPGRADE_CPPFLAGS $LWREG_INCLUDES"
LWUPGRADE_LDFLAGS="$LWUPGRADE_LDFLAGS $LWREG_LDFLAGS $LWREG_LIBS"

AC_SUBST(LWUPGRADE_CPPFLAGS)
AC_SUBST(LWUPGRADE_CFLAGS)
AC_SUBST(LWUPGRADE_LDFLAGS)

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $LWUPGRADE_CPPFLAGS"

# Checks for library functions
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strndup strncasecmp strtoul strtoll __strtoll])

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/socket.h sys/varargs.h pthread.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.
AC_HEADER_STDC

CPPFLAGS="$saved_CPPFLAGS"

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
