AC_INIT(fserv, 0.1, bademail@example.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])

AC_SUBST(FSERV_CPPFLAGS)
AC_SUBST(FSERV_CFLAGS)
AC_SUBST(FSERV_LDFLAGS)

AC_SUBST(LWMSG_CPPFLAGS)
AC_SUBST(LWMSG_LDFLAGS)

AC_SUBST(LIB_LWMSG)
AC_SUBST(LIB_LWMSG_NOTHR)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_HEADER_STDC

AC_CHECK_HEADER([lwmsg/lwmsg.h],,[AC_MSG_ERROR([Could not find lwmsg headers])])
AC_CHECK_LIB([lwmsg], [lwmsg_peer_new], [LIB_LWMSG="-llwmsg"], [AC_MSG_ERROR([Could not find liblwmsg])])

AC_CONFIG_FILES([Makefile include/Makefile src/Makefile])

AC_OUTPUT
