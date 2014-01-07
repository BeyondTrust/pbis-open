AC_PREREQ(2.59)
AC_INIT(lwtest, 0.1, support@likewisesoftware.com)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_MACRO_DIR(m4])

AM_CPPFLAGS=""
AM_CFLAGS=""
AM_LDFLAGS=""

case "$host_os:$host_cpu" in
    linux*:*)
        AC_DEFINE([__LWI_LINUX___], [], [Defined if OS is Linux])
        ;;
    solaris*:*)
        AC_DEFINE([__LWI_SOLARIS__], [], [Define if OS is Solaris])
        AC_DEFINE([_XOPEN_SOURCE], [500], [Define to desired XOPEN compliance level]
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


# lsass

AC_ARG_WITH([lsass],
        [AC_HELP_STRING([--with-lsass=<dir>], [use lsass located in prefix <dir>])],
        [
        LSASS_INCLUDES="-I$withval/include"
        LSASS_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([lsass-includes],
        [AC_HELP_STRING([--with-lsass-includes=<dir>], [use lsass includes located in <dir>])],
        [
        LSASS_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([lsass-libs],
        [AC_HELP_STRING([--with-lsass-libs=<dir>], [use lsass libs located in <dir>])],
        [
        LSASS_LDFLAGS="-L$withval"
        ])

AC_SUBST(LSASS_INCLUDES)
AC_SUBST(LSASS_LDFLAGS)

# debugging

AC_ARG_ENABLE([debug],
        [AC_HELP_STRING([--enable-debug], [enable debugging (default: disabled)])],
        [
                if test x"$enableval" = x"yes"
                then
                    AM_CFLAGS="$AM_CFLAGS -g -O0"
                    AM_CPPFLAGS="$AM_CPPFLAGS -DDEBUG"
                fi
        ])

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [PTHREAD_LIBS="-lpthread"])
AC_CHECK_LIB([lsaclient], [LsaOpenServer], [LSASS_LIBS="-llsaclient"])

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(LSASS_LIBS)

# Checks for library functions
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strndup strncasecmp strtoul])

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/socket.h sys/varargs.h pthread.h])
AC_CHECK_HEADERS([time.h sys/time.h])
AC_CHECK_HEADERS([libgen.h])

CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_GNU_SOURCE"

AM_CPPFLAGS="$AM_CPPFLAGS -I\$(top_srcdir)/include -I\$(top_builddir)/include"
AM_CFLAGS="$AM_CFLAGS -Wall -Werror -fno-strict-aliasing"

AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CFLAGS)


# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile
                 scripts/Makefile
                 src/Makefile
                 src/lsa/Makefile
                 src/utility/Makefile])

AC_OUTPUT
