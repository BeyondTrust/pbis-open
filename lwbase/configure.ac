AC_INIT(lwbase, 0.1, support@likewisesoftware.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

LWBASE_CPPFLAGS=""
LWBASE_CFLAGS="-Wall -Werror -Wmissing-prototypes $CFLAGS"
LWBASE_LDFLAGS=""

DS_FRAMEWORK_LDFLAGS=""

CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_GNU_SOURCE"

MOD_EXT=".so"
REALLY_X64=""

case "$host_os:$host_cpu" in
        solaris*:*)
		if echo "$CC" | grep -- "-m64" >/dev/null 2>&1
		then
			REALLY_X64="yes"
		fi
		LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_XOPEN_SOURCE=500 -D_EXTENSIONS -D_REENTRANT"
		;;
	hpux*:hppa*)
		LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500"
		MOD_EXT=".sl"
		AC_DEFINE([__LWI_HP_UX__], [], [Define if OS is HP-UX])
		;;
	hpux*:ia64*)
		LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500"
		AC_DEFINE([__LWI_HP_UX__], [], [Define if OS is HP-UX])
		;;
	aix*:*)
		LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_THREAD_SAFE"
		;;	
    	darwin*:*)
		if test "`sysctl -n hw.optional.x86_64 2>/dev/null`" = "1"
		then
			REALLY_X64="yes"
		fi
		DS_FRAMEWORK_LDFLAGS="-framework DirectoryService"
		AC_DEFINE([__LWI_DARWIN__], [], [Define if OS is Darwin])
		;;	
esac
     

case "$host_cpu" in
     	i?86)
		if test -n "$REALLY_X64" && echo "$CC" | grep -v -- "-arch i386" >/dev/null 2>&1
		then
			AC_DEFINE([LWBASE_ATOMIC_X86_64], [], [x86_64 atomic operations])
		else
			AC_DEFINE([LWBASE_ATOMIC_X86], [], [x86 atomic operations])
		fi
		;;
	x86_64)
		# Check for -m32 in CC
		if echo "$CC" | grep -- "-m32" >/dev/null 2>&1
		then
		        AC_DEFINE([LWBASE_ATOMIC_X86], [], [x86 atomic operations])
		else
			AC_DEFINE([LWBASE_ATOMIC_X86_64], [], [x86_64 atomic operations])
		fi
		;;
	*)
		AC_DEFINE([LWBASE_ATOMIC_LOCK], [], [lock atomic operations])
		;;
esac

AS_AC_EXPAND(LIBDIR, $libdir)
AC_DEFINE_UNQUOTED(LIBDIR, "$LIBDIR", [Library directory])
AC_DEFINE_UNQUOTED([MOD_EXT], ["$MOD_EXT"], [Extension of shared modules])

AC_SUBST(MOD_EXT)
AC_SUBST(LWBASE_CPPFLAGS)
AC_SUBST(LWBASE_CFLAGS)
AC_SUBST(LWBASE_LDFLAGS)

AC_SUBST(DS_FRAMEWORK_LDFLAGS)

# Checks for programs.
AC_PROG_CC
AC_PROG_MKDIR_P
AC_PROG_LIBTOOL

LW_USE_DOXYGEN([1.5.3])
AC_SUBST([DOXYGEN],[$DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN], [$HAVE_DOXYGEN])
AM_CONDITIONAL([HAVE_DOXYGEN_PDF], [$HAVE_DOXYGEN_PDF])

# libuuid

AC_ARG_WITH([libuuid],
        [AC_HELP_STRING([--with-libuuid=<dir>], [use libuuid located in prefix <dir>])],
        [
                LIBUUID_INCLUDES="-I$withval/include"
                LIBUUID_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([libuuid-includes],
        [AC_HELP_STRING([--with-libuuid-includes=<dir>], [use libuuid includes located in <dir>])],
        [
                LIBUUID_INCLUDES="-I$withval/include"
        ])

AC_ARG_WITH([libuuid-libs],
        [AC_HELP_STRING([--with-libuuid-libs=<dir>], [use libuuid libs located in <dir>])],
        [
                LIBUUID_LDFLAGS="-L$withval/lib"
        ])

LIBUUID_LIBS="-luuid"

AC_SUBST(LIBUUID_INCLUDES)
AC_SUBST(LIBUUID_LDFLAGS)
AC_SUBST(LIBUUID_LIBS)


# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [LIB_PTHREAD="-lpthread"], [LIB_PTHREAD=""])
AC_CHECK_LIB([uuid], [uuid_generate_time], [LIBUUID_LIBS="-luuid"], [], [$LIBUUID_LDFLAGS])
AC_CHECK_LIB(xnet, __xnet_connect, [LIB_XNET="-lxnet"], [LIB_XNET=""])
AC_CHECK_LIB([dl], [dlopen], [LIB_DL="-ldl"], [LIB_DL=""])
AC_CHECK_LIB([rt], [sigwaitinfo], [LIB_RT="-lrt"], [LIB_RT=""])

AC_SUBST(LIB_DL)
AC_SUBST(LIB_XNET)
AC_SUBST(LIB_PTHREAD)
AC_SUBST(LIB_RT)

AC_CHECK_FUNCS([strndup strncasecmp])

saved_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $LIB_RT"
AC_CHECK_FUNCS([sigwaitinfo])
LDFLAGS="$saved_LDFLAGS"

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/socket.h sys/varargs.h sys/time.h sys/param.h pthread.h pthread_np.h unistd.h uuid/uuid.h])
AC_CHECK_HEADERS([sys/cpuset.h],,,[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(wchar_t)

# Check for cpu set type
AC_CHECK_TYPE(
	[cpu_set_t],
	[AC_DEFINE_UNQUOTED(CPU_SET_TYPE, [cpu_set_t], [CPU set type])],
	[],
	[
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
])
AC_CHECK_TYPE(
	[cpuset_t],
	[AC_DEFINE_UNQUOTED(CPU_SET_TYPE, [cpuset_t], [CPU set type])],
	[],
	[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_CPUSET_H
#include <sys/cpuset.h>
#endif
])

# Checks for library functions.
AC_HEADER_STDC

AC_CHECK_HEADERS([wctype.h])

AC_CHECK_LIB(iconv, iconv, [LIB_ICONV="-liconv"],
    AC_CHECK_LIB(iconv, libiconv, [LIB_ICONV="-liconv"], [LIB_ICONV=""]))

AC_SUBST(LIB_ICONV)

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

AC_CHECK_FUNCS([wcscasecmp])


# Check for epoll

AC_CHECK_HEADERS([sys/epoll.h], [HAVE_EPOLL_H=true], [HAVE_EPOLL_H=false])
AC_CHECK_FUNC([epoll_create], [HAVE_EPOLL_CREATE=true], [HAVE_EPOLL_CREATE=false])

# Check for kqueue

AC_CHECK_FUNC([kqueue], [HAVE_KQUEUE=true], [HAVE_KQUEUE=false])

# Check for CPU affinity functions
saved_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $LIB_PTHREAD"
AC_CHECK_FUNCS([pthread_setaffinity_np pthread_attr_setaffinity_np])
LDFLAGS="$saved_LDFLAGS"

THREADPOOL_BACKEND=poll

if $HAVE_EPOLL_H && $HAVE_EPOLL_CREATE
then
	THREADPOOL_BACKEND=epoll
fi

if $HAVE_KQUEUE
then
	THREADPOOL_BACKEND=kqueue
fi

if test "$THREADPOOL_BACKEND" = "select"
then
     	USE_SELECT=true
	AC_DEFINE([USE_SELECT], [], [Define to use select backend])
else
	USE_SELECT=false
fi

if test "$THREADPOOL_BACKEND" = "poll"
then
     	USE_POLL=true
	AC_DEFINE([USE_POLL], [], [Define to use poll backend])
else
	USE_POLL=false
fi

if test "$THREADPOOL_BACKEND" = "kqueue"
then
     	USE_KQUEUE=true
	AC_DEFINE([USE_KQUEUE], [], [Define to use kqueue backend])
else
	USE_KQUEUE=false
fi

if test "$THREADPOOL_BACKEND" = "epoll"
then
     	USE_EPOLL=true
	AC_DEFINE([USE_EPOLL], [], [Define to use epoll backend])
else
	USE_EPOLL=false
fi

AM_CONDITIONAL(USE_SELECT, $USE_SELECT)
AM_CONDITIONAL(USE_POLL, $USE_POLL)
AM_CONDITIONAL(USE_EPOLL, $USE_EPOLL)
AM_CONDITIONAL(USE_KQUEUE, $USE_KQUEUE)

# Check for moonunit bits
AC_CHECK_HEADERS([moonunit/moonunit.h], [HAVE_MOONUNIT_H=true], [HAVE_MOONUNIT_H=false])
AC_CHECK_LIB(moonunit, Mu_Interface_Result, [HAVE_MOONUNIT_LIB=true], [HAVE_MOONUNIT_LIB=false])
AC_CHECK_PROG(MOONUNIT_STUB, moonunit-stub, moonunit-stub, false)

if test "${MOONUNIT_STUB}" = "moonunit-stub" && $HAVE_MOONUNIT_H && $HAVE_MOONUNIT_LIB
then
	HAVE_MOONUNIT="true"
	LIB_MOONUNIT="-lmoonunit"
	AC_DEFINE([HAVE_MOONUNIT], [], [Have MoonUnit])
else
	HAVE_MOONUNIT="false"
fi

AC_SUBST(LIB_MOONUNIT)
AM_CONDITIONAL(HAVE_MOONUNIT, $HAVE_MOONUNIT)

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 include/lw/Makefile
                 src/Makefile
                 moonunit/Makefile
		 doc/Makefile
		 doc/Doxyfile
		 ])

AC_OUTPUT

echo ""
echo "Likewise Base Runtime Library configuration:"
echo ""
echo "Thread pool event backend: $THREADPOOL_BACKEND"
echo ""
