#
# Copyright (C) Likewise Software.  All rights reserved.
#
AC_INIT(lwautoenroll, 1.0, support@likewise.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LIBTOOL

# Checks for headers.
AC_USE_SYSTEM_EXTENSIONS

AC_HEADER_STDC

AC_C_BIGENDIAN
if test "x$ac_cv_c_bigendian" = "xyes"; then
    AC_DEFINE([LW_BIG_ENDIAN], [], [Define standard macro for Big Endian systems])
else
    AC_DEFINE([LW_LITTLE_ENDIAN], [], [Define standard macro for Little Endian systems])
fi

LWAUTOENROLL_CPPFLAGS=
LWAUTOENROLL_CFLAGS='-Wall -Werror -Wmissing-prototypes'
AC_SUBST(LWAUTOENROLL_CPPFLAGS)
AC_SUBST(LWAUTOENROLL_CFLAGS)

AS_AC_EXPAND(SBINDIR, ["$sbindir"])
AC_DEFINE_UNQUOTED(SBINDIR, ["$SBINDIR"], [sbin directory])

if test x"$localstatedir" = x"/var"; then
    cachedir="$localstatedir/lib/likewise"
else
    cachedir="$localstatedir"
fi
AC_SUBST(cachedir)
AS_AC_EXPAND(CACHEDIR, $cachedir)
AC_DEFINE_UNQUOTED(CACHEDIR, "$CACHEDIR", [Cache directory])

LW_USE_DOXYGEN([1.4.0])
AC_SUBST([DOXYGEN],[$DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN], [$HAVE_DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN_PDF], [$HAVE_DOXYGEN_PDF])

AC_CONFIG_FILES([
    Makefile
    doc/Doxyfile
    config/lwcertd.reg
])

AC_OUTPUT
