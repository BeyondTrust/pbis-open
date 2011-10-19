#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lwnetapi, 5.0.0, support@likewisesoftware.com)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([include/config.h])

AM_CPPFLAGS=""
AM_CFLAGS=""
AM_LDFLAGS=""

PKG_VERSION="5.0.0"

# Platform-specific stuff

MOD_EXT=".so"
DS_FRAMEWORK_LDFLAGS=""

AC_USE_SYSTEM_EXTENSIONS

case "$host_os:$host_cpu" in
	linux*:i?86|linux*:x86_64|linux*:s390*)
		AC_DEFINE([__LWI_LINUX__], [], [Define if OS is Linux])
		;;
	solaris*:i?86|solaris*:sparc*)
		AC_DEFINE([__LWI_SOLARIS__], [], [Define if OS is Solaris])
	    AC_DEFINE([_XOPEN_SOURCE], [500], [Define to desired XOPEN compliance level])
		;;
        darwin8.*:*)
                echo "Detected Mac OS X - Tiger"
                DS_FRAMEWORK_LDFLAGS="-framework DirectoryService"
                AC_DEFINE([__LWI_DARWIN__], [], [Define if OS is Darwin])
                ;;
        darwin9.*:*)
                echo "Detected Mac OS X - Leopard"
                DS_FRAMEWORK_LDFLAGS="-framework DirectoryService"
                AC_DEFINE([__LWI_DARWIN__], [], [Define if OS is Darwin])
                ;;
        darwin10.*:*)
                echo "Detected Mac OS X - Snow Leopard"
                DS_FRAMEWORK_LDFLAGS="-framework DirectoryService"
                AC_DEFINE([__LWI_DARWIN__], [], [Define if OS is Darwin - x64 Snow Leopard])
                AC_DEFINE([__LWI_DARWIN_X64__], [], [Define if OS is Darwin - x64 Snow Leopard])
                ;;
	freebsd*:*)
		AC_DEFINE([__LWI_FREEBSD__], [], [Define if OS is FreeBSD])
		;;
	netbsd*:*)
		AC_DEFINE([__LWI_NETBSD__], [], [Define if OS is FreeBSD])
		;;
	hpux*:hppa*)
		MOD_EXT=".sl"
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

AC_SUBST(MOD_EXT)
AC_SUBST(DS_FRAMEWORK_LDFLAGS)

AC_DEFINE_UNQUOTED([MOD_EXT], ["$MOD_EXT"], [Extension of shared modules])

# Package version

AC_ARG_WITH([version],
        [AC_HELP_STRING([--with-version=<pkg-version>], [package version <major>.<minor>.<build>])],
        [
		PKG_VERSION="$withval"
        ])
AC_DEFINE_UNQUOTED(PKG_VERSION, "$PKG_VERSION", [Package Version])


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

AC_CHECK_HEADER([reg/reg.h],,[AC_MSG_ERROR([Could not find lwreg headers])],
[[#include <stdarg.h>
]])
AC_SUBST(LWREG_INCLUDES)
AC_SUBST(LWREG_LDFLAGS)
AC_SUBST(LWREG_LIBS)


# libunistr

AC_ARG_WITH([libunistr],
        [AC_HELP_STRING([--with-libunistr=<dir>], [use libunistr located in prefix <dir>])],
        [
		LIBUNISTR_INCLUDES="-I$withval/include"
                LIBUNISTR_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([libunistr-includes],
        [AC_HELP_STRING([--with-libunistr-includes=<dir>], [use libunistr includes located in <dir>])],
        [
		LIBUNISTR_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([libunistr-libs],
        [AC_HELP_STRING([--with-libunistr-libs=<dir>], [use libunistr libs located in <dir>])],
        [
		LIBUNISTR_LDFLAGS="-L$withval"
        ])

LIBUNISTR_LIBS="-lunistr"

AC_SUBST(LIBUNISTR_INCLUDES)
AC_SUBST(LIBUNISTR_LDFLAGS)
AC_SUBST(LIBUNISTR_LIBS)

# netlogon

AC_ARG_WITH([netlogon],
        [AC_HELP_STRING([--with-netlogon=<dir>], [use netlogon located in prefix <dir>])],
        [
                NETLOGON_INCLUDES="-I$withval/include"
                NETLOGON_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([netlogon-includes],
        [AC_HELP_STRING([--with-netlogon-includes=<dir>], [use netlogon includes located in <dir>])],
        [
                NETLOGON_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([netlogon-libs],
        [AC_HELP_STRING([--with-netlogon-libs=<dir>], [use netlogon libs located in <dir>])],
        [
                NETLOGON_LDFLAGS="-L$withval"
        ])

NETLOGON_LIBS="-llwnetclientapi"

AC_SUBST(NETLOGON_INCLUDES)
AC_SUBST(NETLOGON_LDFLAGS)
AC_SUBST(NETLOGON_LIBS)

# lwadvapi

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

LSASS_LIBS=-llsarpc

AC_SUBST(LSASS_INCLUDES)
AC_SUBST(LSASS_LDFLAGS)
AC_SUBST(LSASS_LIBS)

# srvsvc

AC_ARG_WITH([srvsvc],
        [AC_HELP_STRING([--with-srvsvc=<dir>], [use srvsvc located in prefix <dir>])],
        [
        SRVSVC_INCLUDES="-I$withval/include"
        SRVSVC_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([srvsvc-includes],
        [AC_HELP_STRING([--with-srvsvc-includes=<dir>], [use srvsvc includes located in <dir>])],
        [
        SRVSVC_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([srvsvc-libs],
        [AC_HELP_STRING([--with-srvsvc-libs=<dir>], [use srvsvc libs located in <dir>])],
        [
        SRVSVC_LDFLAGS="-L$withval"
        ])

SRVSVC_LIBS=-lsrvsvc

AC_SUBST(SRVSVC_INCLUDES)
AC_SUBST(SRVSVC_LDFLAGS)
AC_SUBST(SRVSVC_LIBS)

# dcerpc

AC_ARG_WITH([dcerpc],
        [AC_HELP_STRING([--with-dcerpc=<dir>], [use dcerpc located in prefix <dir>])],
        [
        DCERPC_INCLUDES="-I$withval/include"
        DCERPC_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([dcerpc-includes],
        [AC_HELP_STRING([--with-dcerpc-includes=<dir>], [use dcerpc includes located in <dir>])],
        [
        DCERPC_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([dcerpc-libs],
        [AC_HELP_STRING([--with-dcerpc-libs=<dir>], [use dcerpc libs located in <dir>])],
        [
        DCERPC_LDFLAGS="-L$withval"
        ])

DCERPC_LIBS=-ldcerpc

AC_SUBST(DCERPC_INCLUDES)
AC_SUBST(DCERPC_LDFLAGS)
AC_SUBST(DCERPC_LIBS)

# dcerpc

AC_ARG_WITH([netlogon],
        [AC_HELP_STRING([--with-netlogon=<dir>], [use dcerpc located in prefix <dir>])],
        [
        NETLOGON_INCLUDES="-I$withval/include"
        NETLOGON_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([netlogon-includes],
        [AC_HELP_STRING([--with-netlogon-includes=<dir>], [use netlogon includes located in <dir>])],
        [
        NETLOGON_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([netlogon-libs],
        [AC_HELP_STRING([--with-netlogon-libs=<dir>], [use netlogon libs located in <dir>])],
        [
        NETLOGON_LDFLAGS="-L$withval"
        ])

NETLOGON_LIBS=-llwnetclientapi

AC_SUBST(NETLOGON_INCLUDES)
AC_SUBST(NETLOGON_LDFLAGS)
AC_SUBST(NETLOGON_LIBS)

# unicode

AC_ARG_ENABLE([unicode],
               [AC_HELP_STRING([--disable-unicode], [disable unicode support (default: enabled)])],[use_unicode=$enableval],[use_unicode=yes])

if test x"$use_unicode" = x"yes"
then
    AM_CFLAGS="$AM_CFLAGS -DUNICODE"
fi

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

CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_GNU_SOURCE"

AM_CPPFLAGS="$AM_CPPFLAGS -I${top_srcdir}/include"
AM_CFLAGS="$AM_CFLAGS -Wall -Werror -fno-strict-aliasing"

AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CFLAGS)

AS_AC_EXPAND(PREFIXDIR, $prefix)
AC_DEFINE_UNQUOTED(PREFIXDIR, "$PREFIXDIR", [Prefix directory])

AS_AC_EXPAND(LIBDIR, $libdir)
AC_DEFINE_UNQUOTED(LIBDIR, "$LIBDIR", [Library directory])

AS_AC_EXPAND(SBINDIR, ["${sbindir}"])

AC_C_BIGENDIAN

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.

# Check for basic types
AC_CHECK_TYPES([int8, int16, int32, int64], [], [],
[#include <sys/types.h>
 #if __ia64
 #include <model.h>
 #endif])
AC_CHECK_TYPES([uint8, uint16, uint32, uint64])

AC_CHECK_SIZEOF([long long int])
AC_CHECK_SIZEOF([long int])

AC_CHECK_DECLS([isblank], [], [], [#include <ctype.h>])

AC_CHECK_HEADERS([inttypes.h stdlib.h sys/varargs.h dlfcn.h])
AC_CHECK_HEADERS([time.h sys/time.h strings.h stdbool.h sys/socket.h netinet/tcp.h])

AC_CHECK_HEADERS([winnttypes.h wc16str.h wc16printf.h])

AC_CHECK_TYPES([wchar16_t], [], [], [AC_INCLUDES_DEFAULT
#ifdef HAVE_WC16STR_H
# include <wc16str.h>
#endif
])

AC_CHECK_TYPES([blksize_t])

AC_CHECK_HEADERS([sys/param.h sys/pstat.h procfs.h sys/procfs.h sys/sysctl.h sys/user.h])

AC_CHECK_DECLS([KERN_PROC_PATHNAME, KERN_PROCARGS], [], [], [
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#if HAVE_SYS_PSTAT_H
#include <sys/pstat.h>
#endif])

AC_CHECK_TYPES([struct psinfo], , , [
#if HAVE_PROCFS_H
#include <procfs.h>
#elif HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif
])

AC_CHECK_FUNCS([pstat_getproc])

# Checks for library functions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_FUNC_STRERROR_R

AC_CHECK_FUNCS([strtoll __strtoll strtoull __strtoull strtol strtoul])
AC_CHECK_FUNCS([stpncpy strnlen strndup])
AC_CHECK_FUNCS([atexit localtime_r memset strchr strerror strerror_r vsyslog])
AC_CHECK_FUNCS([rpl_realloc rpl_malloc])

AC_CHECK_LIB([pthread], [pthread_self], [PTHREAD_LIBS="-lpthread"])
AC_SUBST(PTHREAD_LIBS)

AC_CHECK_LIB([rt], [sem_init], [SEM_LIBS="-lrt"])
AC_SUBST(SEM_LIBS)

AC_CHECK_LIB([unistr], [mbstowc16s], [LIBUNISTR_LIBS="-lunistr"], [], [$LIBUNISTR_LDFLAGS])

AC_CHECK_HEADER([lw/base.h],,[AC_MSG_ERROR([Could not find lwbase headers])])
AC_CHECK_LIB([lwbase_nothr], [LwRtlMemoryAllocate], [LWBASE_NOTHR_LIBS="-llwbase_nothr"], [AC_MSG_ERROR([Could not find liblwbase_nothr])], [$LWBASE_LDFLAGS])
AC_CHECK_LIB([lwbase], [LwInterlockedIncrement], [LWBASE_LIBS="-llwbase"], [AC_MSG_ERROR([Could not find liblwbase])], [$LWBASE_LDFLAGS])

AC_SUBST(LWBASE_INCLUDES)
AC_SUBST(LWBASE_LDFLAGS)
AC_SUBST(LWBASE_NOTHR_LIBS)
AC_SUBST(LWBASE_LIBS)

AC_CONFIG_FILES([
        Makefile
        include/Makefile
        src/Makefile
        test/Makefile])

AC_OUTPUT
