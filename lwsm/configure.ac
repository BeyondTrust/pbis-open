AC_INIT(lwsm, 0.1, support@likewise.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

LWSM_CPPFLAGS="-DLWSM_DISABLE_DEPRECATED -DLWSM_BUILD"
LWSM_CFLAGS="-Wall -Werror -Wmissing-prototypes"
LWSM_LDFLAGS=""
MOD_EXT=".so"

case "$host_os:$host_cpu" in
        linux*:*)
		LWSM_CPPFLAGS="$LWSM_CPPFLAGS -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -D_GNU_SOURCE"
		;;
        solaris*:*)
                LWSM_CPPFLAGS="$LWSM_CPPFLAGS -D_XOPEN_SOURCE=500 -D_EXTENSIONS -D_REENTRANT"
		;;
	hpux*:hppa*)
		LWSM_CPPFLAGS="$LWSM_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500 -D_REENTRANT"
		MOD_EXT=".sl"
		;;
	hpux*:ia64*)
		LWSM_CPPFLAGS="$LWSM_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500 -D_REENTRANT"
		MOD_EXT=".so"
		;;
	aix*:*)
		LWSM_CPPFLAGS="$LWSM_CPPFLAGS -D_THREAD_SAFE"
		;;	
esac

AS_AC_EXPAND(SBINDIR, $sbindir)
AC_DEFINE_UNQUOTED(SBINDIR, "$SBINDIR", [sbin directory])

if test x"$localstatedir" = x"/var"; then
    cachedir="$localstatedir/lib/likewise"
else
    cachedir="$localstatedir"
fi

AS_AC_EXPAND(CACHEDIR, $cachedir)
AC_DEFINE_UNQUOTED(CACHEDIR, "$CACHEDIR", [Cache directory])

loaderdir="$libdir/lwsm-loader"
AS_AC_EXPAND(LOADERDIR, $loaderdir)
AC_DEFINE_UNQUOTED(LOADERDIR, "$LOADERDIR", [Loader directory])

AC_SUBST(loaderdir)
AC_SUBST(LWSM_CPPFLAGS)
AC_SUBST(LWSM_CFLAGS)
AC_SUBST(LWSM_LDFLAGS)
AC_SUBST(LIB_PTHREAD)
AC_SUBST(LIB_LWMSG)
AC_SUBST(LIB_LWADVAPI)
AC_SUBST(LIB_LWBASE)
AC_SUBST(LIB_REGCLIENT)
AC_SUBST(LIB_LWIOCLIENT)
AC_SUBST(LIB_DL)
AC_SUBST(MOD_EXT)
AC_DEFINE_UNQUOTED([MOD_EXT], ["$MOD_EXT"], [Extension of dynamic shared modules])

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $LWSM_CPPFLAGS"

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LIBTOOL

LW_USE_DOXYGEN([1.5.3])
AC_SUBST([DOXYGEN],[$DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN], [$HAVE_DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN_PDF], [$HAVE_DOXYGEN_PDF])

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [LIB_PTHREAD="-lpthread"], [LIB_PTHREAD=""])
AC_CHECK_LIB(lwmsg, lwmsg_peer_new, [LIB_LWMSG="-llwmsg"], [AC_MSG_ERROR([Could not find liblwmsg_nothr])])
AC_CHECK_LIB(lwadvapi, LwBuildSIDString, [LIB_LWADVAPI="-llwadvapi"], [AC_MSG_ERROR([Could not find liblwadvapi])])
AC_CHECK_LIB(lwbase, RtlAccessCheck, [LIB_LWBASE="-llwbase"], [AC_MSG_ERROR([Could not find liblwbase])])
AC_CHECK_LIB(regclient, LwRegOpenKeyExA, [LIB_REGCLIENT="-lregclient"], [AC_MSG_ERROR([Could not find libregclient])])
AC_CHECK_LIB(lwioclient, LwIoQueryStateDriver, [LIB_LWIOCLIENT="-llwioclient"], [AC_MSG_ERROR([Could not find liblwioclient])])
AC_CHECK_LIB(dl, dlopen, [LIB_DL="-ldl"], [LIB_DL=""])

# Checks for header files.
AC_CHECK_HEADERS([lwmsg/lwmsg.h lwdef.h lwerror.h lw/base.h pthread.h strings.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN

# Checks for library functions.
AC_HEADER_STDC
AC_CHECK_FUNCS([memset])

CPPFLAGS="$saved_CPPFLAGS"

AC_CONFIG_FILES([Makefile doc/Doxyfile])

AC_OUTPUT
