AC_INIT(lwmsg, 0.1, support@likewisesoftware.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

LWMSG_CPPFLAGS="-DLWMSG_DISABLE_DEPRECATED"
LWMSG_CFLAGS="-Wall -Werror -Wmissing-prototypes"
LWMSG_LDFLAGS=""
MOD_EXT=".so"

case "$host_os:$host_cpu" in
        linux*:*)
		LWMSG_CPPFLAGS="$LWMSG_CPPFLAGS -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -D_GNU_SOURCE"
		;;
        solaris*:*)
                LWMSG_CPPFLAGS="$LWMSG_CPPFLAGS -D_XOPEN_SOURCE=500 -D_EXTENSIONS -D_REENTRANT"
		;;
	hpux*:hppa*)
		LWMSG_CPPFLAGS="$LWMSG_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500 -D_REENTRANT"
		MOD_EXT=".sl"
		;;
	hpux*:ia64*)
		LWMSG_CPPFLAGS="$LWMSG_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500 -D_REENTRANT"
		MOD_EXT=".so"
		;;
	aix*:*)
		LWMSG_CPPFLAGS="$LWMSG_CPPFLAGS -D_THREAD_SAFE"
		;;	
esac

AC_SUBST(LWMSG_CPPFLAGS)
AC_SUBST(LWMSG_CFLAGS)
AC_SUBST(LWMSG_LDFLAGS)
AC_SUBST(LIB_DL)
AC_SUBST(LIB_XNET)
AC_SUBST(LIB_PTHREAD)
AC_SUBST(LIB_RT)
AC_SUBST(LIB_ICONV)
AC_SUBST(MOD_EXT)
AC_DEFINE_UNQUOTED([MOD_EXT], ["$MOD_EXT"], [Extension of dynamic shared modules])

AC_ARG_ENABLE([internal-docs], [AC_HELP_STRING([--enable-internal-docs], [enable documentation of LWMsg internals])])
if test x"$enable_internal_docs" = x"yes"
then
	ENABLE_INTERNAL_DOCS="YES"
else
	ENABLE_INTERNAL_DOCS="NO"
fi

AC_SUBST(ENABLE_INTERNAL_DOCS)

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $LWMSG_CPPFLAGS"

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

# Check if pthread_sigmask is available without libpthread
AC_CHECK_FUNC(pthread_sigmask, [AC_DEFINE([HAVE_PTHREAD_SIGMASK_IN_LIBC],[],[Define if pthread_sigmask is available in libc])],[])

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [LIB_PTHREAD="-lpthread"], [LIB_PTHREAD=""])
AC_CHECK_LIB(rt, nanosleep, [LIB_RT="-lrt"], [LIB_RT=""])
AC_CHECK_LIB(dl, dlopen, [LIB_DL="-ldl"], [LIB_DL=""])
AC_CHECK_LIB(xnet, __xnet_connect, [LIB_XNET="-lxnet"], [LIB_XNET=""])
AC_CHECK_LIB(iconv, iconv, [LIB_ICONV="-liconv"],
             AC_CHECK_LIB(iconv, libiconv, [LIB_ICONV="-liconv"], [LIB_ICONV=""]))

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/socket.h sys/varargs.h sys/select.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INTMAX_T
AC_TYPE_UINTMAX_T
AC_TYPE_SIZE_T
AC_C_BIGENDIAN

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Werror"

AC_MSG_CHECKING([whether iconv() takes const char** as its second parameter])

AC_TRY_COMPILE(
[#include <iconv.h>
 #include <stddef.h>
 ],
		[iconv_t h = iconv_open(NULL, NULL);
		 char* in;
		 (void) iconv(h, (const char**) &in, NULL, NULL, NULL);
		],
		[AC_MSG_RESULT([yes]); AC_DEFINE_UNQUOTED(ICONV_IN_TYPE, [const char**], [iconv in type])],
		[AC_MSG_RESULT([no]); AC_DEFINE_UNQUOTED(ICONV_IN_TYPE, [char**], [iconv in type])])

CFLAGS="$saved_CFLAGS"

# Checks for library functions.
AC_HEADER_STDC
AC_CHECK_FUNCS([memset])
AC_FUNC_STRERROR_R

# Check for eid method

AC_CHECK_DECLS([SO_PEERCRED, getpeereid],,,
[
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
])

AC_CHECK_FUNCS([getpeereid])

# Check for moonunit bits
AC_CHECK_HEADERS([moonunit/moonunit.h], [HAVE_MOONUNIT_H=true], [HAVE_MOONUNIT_H=false])
AC_CHECK_LIB(moonunit, Mu_Interface_Result, [HAVE_MOONUNIT_LIB=true], [HAVE_MOONUNIT_LIB=false])
AC_CHECK_PROG(MOONUNIT_STUB, moonunit-stub, moonunit-stub, false)

if test "${MOONUNIT_STUB}" = "moonunit-stub" && $HAVE_MOONUNIT_H && $HAVE_MOONUNIT_LIB
then
	HAVE_MOONUNIT="true"
	LIB_moonunit="-lmoonunit"
	AC_DEFINE([HAVE_MOONUNIT], [], [Have MoonUnit])
else
	HAVE_MOONUNIT="false"
fi

AC_SUBST(LIB_moonunit)
AM_CONDITIONAL(HAVE_MOONUNIT, $HAVE_MOONUNIT)

CPPFLAGS="$saved_CPPFLAGS"

AC_CONFIG_FILES([Makefile doc/Doxyfile])

AC_OUTPUT
