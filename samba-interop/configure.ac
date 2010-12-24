#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lsass, 5.0.0, support@likewisesoftware.com)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SUBDIRS([libtdb])

AM_CPPFLAGS=""
AM_CFLAGS=""
AM_LDFLAGS=""

AC_USE_SYSTEM_EXTENSIONS

case "$host_os:$host_cpu" in
        solaris*:i?86|solaris*:sparc*)
                AC_DEFINE([_XOPEN_SOURCE], [500], [Define to desired XOPEN compliance level])
                ;;
        hpux*:hppa*)
		AC_DEFINE([_XOPEN_SOURCE_EXTENDED], [1], [Define on HP-UX])
                ;;
        hpux*:ia64*)
		AC_DEFINE([_XOPEN_SOURCE_EXTENDED], [1], [Define on HP-UX])
                ;;
        aix*:*)
                AC_DEFINE([_LINUX_SOURCE_COMPAT], [], [Enable Linux source compatibility on AIX])
                ;;
esac

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

AC_ARG_ENABLE([profiling],
        [AC_HELP_STRING([--enable-profiling], [enable profiling (default: disabled)])],
        [
                if test x"$enableval" = x"yes"
                then
                        AM_CFLAGS="$AM_CFLAGS -fprofile-arcs -ftest-coverage"
                        LDFLAGS="$LDFLAGS -fprofile-arcs -ftest-coverage"
                fi
        ])

CPPFLAGS="$CPPFLAGS -D_REENTRANT"

AM_CPPFLAGS="$AM_CPPFLAGS -I\$(top_srcdir)/include"
AM_CFLAGS="$AM_CFLAGS -Wall -Werror -fno-strict-aliasing"

AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CFLAGS)

AC_C_BIGENDIAN

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MKDIR_P

LW_USE_DOXYGEN([1.5.3])
AC_SUBST([DOXYGEN],[$DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN], [$HAVE_DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN_PDF], [$HAVE_DOXYGEN_PDF])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h stdint.h stdbool.h unistd.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.

AC_CHECK_HEADER([lwmsg/lwmsg.h],,[AC_MSG_ERROR([Could not find lwmsg headers])])
AC_CHECK_LIB([lwmsg_nothr], [lwmsg_data_marshal], [], [AC_MSG_ERROR([Could not find liblwmsg_nothr])], [$LWMSG_LDFLAGS])
AC_CHECK_LIB([lwmsg], [lwmsg_peer_new], [], [AC_MSG_ERROR([Could not find liblwmsg])], [$LWMSG_LDFLAGS])

AC_CHECK_HEADER([lw/base.h],,[AC_MSG_ERROR([Could not find lwbase headers])])
AC_CHECK_LIB([lwbase_nothr], [LwRtlMemoryAllocate], [], [AC_MSG_ERROR([Could not find liblwbase_nothr])], [$LWBASE_LDFLAGS])
AC_CHECK_LIB([lwbase], [LwInterlockedIncrement], [], [AC_MSG_ERROR([Could not find liblwbase])], [$LWBASE_LDFLAGS])

AS_AC_EXPAND(LIBDIR, $libdir)
AC_DEFINE_UNQUOTED(LIBDIR, "$LIBDIR", [Library directory])

AC_CONFIG_FILES([Makefile
                 lwicompat/Makefile
                 wbclient/Makefile
		 tools/Makefile])

AC_OUTPUT
