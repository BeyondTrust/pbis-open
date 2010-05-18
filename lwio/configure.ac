#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lwio, 5.0.0, support@likewisesoftware.com)
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
		AM_CPPFLAGS="$AM_CPPFLAGS -D_HPUX_SOURCE=1 -U_XOPEN_SOURCE_EXTENDED"
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


# krb5

AC_ARG_WITH([krb5],
        [AC_HELP_STRING([--with-krb5=<dir>], [use krb5 located in prefix <dir>])],
        [
		KRB5_INCLUDES="-I$withval/include"
                KRB5_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([krb5-includes],
        [AC_HELP_STRING([--with-krb5-includes=<dir>], [use krb5 includes located in <dir>])],
        [
		KRB5_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([krb5-libs],
        [AC_HELP_STRING([--with-krb5-libs=<dir>], [use krb5 libs located in <dir>])],
        [
		KRB5_LDFLAGS="-L$withval"
        ])

KRB5_LIBS="-lgssapi_krb5 -lkrb5 -lk5crypto"

AC_SUBST(KRB5_INCLUDES)
AC_SUBST(KRB5_LDFLAGS)
AC_SUBST(KRB5_LIBS)

# crypto

AC_ARG_WITH([crypto],
        [AC_HELP_STRING([--with-crypto=<dir>], [use crypto located in prefix <dir>])],
        [
		CRYPTO_INCLUDES="-I$withval/include"
        CRYPTO_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([crypto-includes],
        [AC_HELP_STRING([--with-crypto-includes=<dir>], [use crypto includes located in <dir>])],
        [
		CRYPTO_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([crypto-libs],
        [AC_HELP_STRING([--with-crypto-libs=<dir>], [use crypto libs located in <dir>])],
        [
		CRYPTO_LDFLAGS="-L$withval"
        ])

CRYPTO_LIBS="-lcrypto"

AC_SUBST(CRYPTO_INCLUDES)
AC_SUBST(CRYPTO_LDFLAGS)
AC_SUBST(CRYPTO_LIBS)

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

# lwmsg

AC_ARG_WITH([lwmsg],
        [AC_HELP_STRING([--with-lwmsg=<dir>], [use lwmsg located in prefix <dir>])],
        [
		LWMSG_INCLUDES="-I$withval/include"
        LWMSG_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([lwmsg-includes],
        [AC_HELP_STRING([--with-lwmsg-includes=<dir>], [use lwmsg includes located in <dir>])],
        [
		LWMSG_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([lwmsg-libs],
        [AC_HELP_STRING([--with-lwmsg-libs=<dir>], [use lwmsg libs located in <dir>])],
        [
		LWMSG_LDFLAGS="-L$withval"
        ])

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


# share repository
share_repository=registry

AC_ARG_WITH([share-repository],
        [AC_HELP_STRING([--with-share-repository=<type>], [use share repository <type>])],
        [
                share_repository="$withval"
        ])

case "$share_repository" in

    sqlite)
    
        SHARE_REPOSITORY_TYPE_DEFINES=-DLW_USE_SHARE_REPOSITORY_SQLITE
    
        ;;
        
    registry)
    
        SHARE_REPOSITORY_TYPE_DEFINES=-DLW_USE_SHARE_REPOSITORY_REGISTRY
        
        ;;
        
    *)
    
        share_repository="registry"
        SHARE_REPOSITORY_TYPE_DEFINES=-DLW_USE_SHARE_REPOSITORY_REGISTRY
        
        ;;
esac

AM_CONDITIONAL(ENABLE_SHARE_REPOSITORY_SQLITE, test x"$share_repository" = x"sqlite")
AM_CONDITIONAL(ENABLE_SHARE_REPOSITORY_REGISTRY, test x"$share_repository" = x"registry")

AC_SUBST(SHARE_REPOSITORY_TYPE_DEFINES)

AC_ARG_WITH([drivers], [AC_HELP_STRING([--with-drivers], [list of drivers to build (npfs pvfs dfs srv rdr)])],
		       [], [with_drivers="npfs pvfs dfs srv rdr"])

_IFS="${IFS}"
IFS=" ,"
for driver in $with_drivers
do
	eval "enable_${driver}=yes"
done
IFS="${_IFS}"

AM_CONDITIONAL(ENABLE_RDR, test x"$enable_rdr" = x"yes")
if test x"$enable_rdr" = x"yes"
then
	AC_DEFINE([ENABLE_RDR], [], [Define if rdr driver should be built])
fi

AM_CONDITIONAL(ENABLE_SRV, test x"$enable_srv" = x"yes")
if test x"$enable_srv" = x"yes"
then
	AC_DEFINE([ENABLE_SRV], [], [Define if srv driver should be built])
fi

AM_CONDITIONAL(ENABLE_PVFS, test x"$enable_pvfs" = x"yes")
if test x"$enable_pvfs" = x"yes"
then
	AC_DEFINE([ENABLE_PVFS], [], [Define if pvfs driver should be built])
fi

AM_CONDITIONAL(ENABLE_DFS, test x"$enable_dfs" = x"yes")
if test x"$enable_dfs" = x"yes"
then
	AC_DEFINE([ENABLE_DFS], [], [Define if dfs driver should be built])
fi

AM_CONDITIONAL(ENABLE_NPFS, test x"$enable_npfs" = x"yes")
if test x"$enable_npfs" = x"yes"
then
	AC_DEFINE([ENABLE_NPFS], [], [Define if npfs driver should be built])
fi

# Boolean settings

AC_ARG_ENABLE([static-drivers], [AC_HELP_STRING([--enable-static-drivers], [build drivers directly into lwiod])])
AM_CONDITIONAL(ENABLE_STATIC_DRIVERS, test x"$enable_static_drivers" = x"yes")
if test x"$enable_static_drivers" = x"yes"
then
	AC_DEFINE([ENABLE_STATIC_DRIVERS], [], [Define if drivers should be built directly into lwiod])
fi

AC_ARG_ENABLE([small-daemon], [AC_HELP_STRING([--enable-small-daemon], [build small lwiod])])
AM_CONDITIONAL(ENABLE_SMALL_DAEMON, test x"$enable_small_daemon" = x"yes")
if test x"$enable_small_daemon" = x"yes"
then
	AC_DEFINE([ENABLE_SMALL_DAEMON], [], [Define if lwiod should be small])
fi

ENABLE_IOTEST=true

AC_ARG_ENABLE([iotest],
        [AC_HELP_STRING([--disable-iotest], [disable iotest (default: enabled)])],
        [
                if test x"$enableval" = x"yes"
                then
                        ENABLE_IOTEST=true
                else
                        ENABLE_IOTEST=false
                fi
        ])

AM_CONDITIONAL(ENABLE_IOTEST, [$ENABLE_IOTEST])

AC_ARG_ENABLE([pidfile], [AC_HELP_STRING([--disable-pidfile], [disable pid file creation (default: enabled)])],
              [], [enable_pidfile="yes"])
if test x"$enable_pidfile" = x"yes"
then
        AC_DEFINE([ENABLE_PIDFILE], [], [Define if pid file should be created])
fi


#####
## Unit tests
#####

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

lwioconfdir="$sysconfdir"
AC_SUBST(lwioconfdir)
AS_AC_EXPAND(CONFIGDIR, $lwioconfdir)
AC_DEFINE_UNQUOTED(CONFIGDIR, "$CONFIGDIR", [Config directory])

lwioregdir="$datadir/config"
AC_SUBST(lwioregdir)

if test x"$localstatedir" = x"/var"; then
    lwiocachedir="$localstatedir/lib/likewise"
else
    lwiocachedir="$localstatedir"
fi
AC_SUBST(lwiocachedir)
AS_AC_EXPAND(CACHEDIR, $lwiocachedir)
AC_DEFINE_UNQUOTED(CACHEDIR, "$CACHEDIR", [Cache directory])

AS_AC_EXPAND(providerdir, $libdir)

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
AC_CHECK_FUNCS([stpncpy strnlen])
AC_CHECK_FUNCS([atexit localtime_r memset strchr strerror strerror_r vsyslog])
AC_CHECK_FUNCS([rpl_realloc rpl_malloc])

TRANSPORT_FOUND=false
HAVE_EPOLL=false
HAVE_KQUEUE=false
HAVE_POLL=false
HAVE_SELECT=false
TRANSPORT_METHOD_DEFINES=

#AC_CHECK_FUNC([epoll_ctl],
#              [HAVE_EPOLL=true;TRANSPORT_FOUND=true;TRANSPORT_METHOD_DEFINES=-DLW_USE_EPOLL],
#              [HAVE_EPOLL=false])
AM_CONDITIONAL(HAVE_EPOLL, [$HAVE_EPOLL])
#
#if test x"$TRANSPORT_FOUND" = x"false"
#then
#AC_CHECK_FUNC([kqueue],
#              [HAVE_KQUEUE=true;TRANSPORT_FOUND=true;TRANSPORT_METHOD_DEFINES=-DLW_USE_KQUEUE],
#              [HAVE_KQUEUE=false])
#fi
#
AM_CONDITIONAL(HAVE_KQUEUE, [$HAVE_KQUEUE])
#
#if test x"$TRANSPORT_FOUND" = x"false"
#then
#AC_CHECK_FUNC([poll],
#              [HAVE_POLL=true;TRANSPORT_FOUND=true;TRANSPORT_METHOD_DEFINES=-DLW_USE_POLL],
#              [HAVE_POLL=false])
#fi
#
AM_CONDITIONAL(HAVE_POLL, [$HAVE_POLL])
#
if test x"$TRANSPORT_FOUND" = x"false"
then
AC_CHECK_FUNC([select],
              [HAVE_SELECT=true;TRANSPORT_FOUND=true;TRANSPORT_METHOD_DEFINES=-DLW_USE_SELECT],
              [HAVE_SELECT=false])
fi

AM_CONDITIONAL(HAVE_SELECT, [$HAVE_SELECT])
AC_SUBST(TRANSPORT_METHOD_DEFINES)

AC_CHECK_HEADERS([sys/vfs.h])
AC_CHECK_FUNCS([dirfd fstatfs fdatasync fsync])

AC_ARG_WITH([libattr],
        [AC_HELP_STRING([--without-libattr], [Do not use libattr])],
        [],
        [with_libattr=yes]
)

AS_IF([test "x$with_libattr" != xno],[
AC_CHECK_HEADERS([attr/xattr.h])
AC_CHECK_FUNCS([setxattr fsetxattr getxattr fgetxattr])
AC_CHECK_LIB([attr], [getxattr], [XATTR_LIBS="-lattr"])
])
AC_SUBST(XATTR_LIBS)

AC_CHECK_LIB([uuid], [uuid_copy], [UUID_LIBS="-luuid"])
AC_SUBST(UUID_LIBS)

AC_CHECK_LIB([pthread], [pthread_self], [PTHREAD_LIBS="-lpthread"])
AC_SUBST(PTHREAD_LIBS)

AC_CHECK_LIB([socket], [bind], [SOCKET_LIBS="-lsocket"])
old_LIBS=$LIBS
AC_SEARCH_LIBS([inet_pton], [nsl], [SOCKET_LIBS="$SOCKET_LIBS $ac_cv_search_inet_pton"])
AC_SEARCH_LIBS([__xnet_connect], [xnet], [SOCKET_LIBS="$SOCKET_LIBS $ac_cv_search___xnet_connect"])
LIBS=$old_LIBS
AC_SUBST(SOCKET_LIBS)

AC_CHECK_LIB([dl], [dlopen], [DL_LIBS="-ldl"])
AC_SUBST(DL_LIBS)

AC_CHECK_LIB([rt], [sem_init], [SEM_LIBS="-lrt"])
AC_SUBST(SEM_LIBS)

AC_CHECK_LIB([crypto], [DES_set_key], [CRYPTO_LIBS="-lcrypto"], [], [$CRYPTO_LDFLAGS])

AC_CHECK_HEADER([lwmsg/lwmsg.h],,[AC_MSG_ERROR([Could not find lwmsg headers])])
AC_CHECK_LIB([lwmsg_nothr], [lwmsg_assoc_send], [LWMSG_NOTHR_LIBS="-llwmsg_nothr"], [AC_MSG_ERROR([Could not find liblwmsg_nothr])], [$LWMSG_LDFLAGS])
AC_CHECK_LIB([lwmsg], [lwmsg_peer_new], [LWMSG_LIBS="-llwmsg"], [AC_MSG_ERROR([Could not find liblwmsg])], [$LWMSG_LDFLAGS])

AC_SUBST(LWMSG_INCLUDES)
AC_SUBST(LWMSG_LDFLAGS)
AC_SUBST(LWMSG_NOTHR_LIBS)
AC_SUBST(LWMSG_LIBS)

AC_CHECK_HEADER([lw/base.h],,[AC_MSG_ERROR([Could not find lwbase headers])])
AC_CHECK_LIB([lwbase_nothr], [LwRtlMemoryAllocate], [LWBASE_NOTHR_LIBS="-llwbase_nothr"], [AC_MSG_ERROR([Could not find liblwbase_nothr])], [$LWBASE_LDFLAGS])
AC_CHECK_LIB([lwbase], [LwInterlockedIncrement], [LWBASE_LIBS="-llwbase"], [AC_MSG_ERROR([Could not find liblwbase])], [$LWBASE_LDFLAGS])

AC_SUBST(LWBASE_INCLUDES)
AC_SUBST(LWBASE_LDFLAGS)
AC_SUBST(LWBASE_NOTHR_LIBS)
AC_SUBST(LWBASE_LIBS)

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
AC_CHECK_HEADER([fuse.h], [HAVE_FUSE_H=true], [HAVE_FUSE_H=false])
CPPFLAGS="$saved_CPPFLAGS"

AC_CHECK_LIB(fuse, fuse_new, [HAVE_FUSE_LIB=true], [HAVE_FUSE_LIB=false])

if test x"$HAVE_FUSE_H" = x"true" -a x"$HAVE_FUSE_LIB" = x"true"
then
	HAVE_FUSE=true
else
	HAVE_FUSE=false
fi

AM_CONDITIONAL(ENABLE_FUSE, test x"$HAVE_FUSE" = x"true")

AC_CHECK_TYPES([socklen_t], [], [], [AC_INCLUDES_DEFAULT
#include <sys/types.h>
#include <sys/socket.h>
])

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Werror $AM_CPPFLAGS"

AC_MSG_CHECKING([if getsockname takes socklen_t*])
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],
[
int fd = -1;
struct sockaddr addr;
socklen_t addrlen;
(void) getsockname(fd, &addr, &addrlen);
], [
AC_MSG_RESULT([yes])
AC_DEFINE([GETSOCKNAME_TAKES_SOCKLEN_T], [], [Define if getsockname takes socklen_t as its third argument])],[
AC_MSG_RESULT([no])
])
AC_MSG_CHECKING([if struct msghdr has msg_control])
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],
[
struct msghdr hdr;
hdr.msg_control = 0;
],
[
AC_MSG_RESULT([yes])
AC_DEFINE([MSGHDR_HAS_MSG_CONTROL], [], [Define if struct msghdr has the msg_control field])
],
[
AC_MSG_RESULT([no])
])

CFLAGS="$saved_CFLAGS"

# ZCT splice, sendfile, etc. support
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Werror"
AC_ARG_WITH([splice],
        [AC_HELP_STRING([--without-splice], [Do not use splice])],
        [],
        [with_splice=yes]
)
AC_ARG_WITH([sendfile],
        [AC_HELP_STRING([--without-sendfile], [Do not use sendfile])],
        [],
        [with_sendfile=yes]
)
AC_ARG_WITH([sendfilev],
        [AC_HELP_STRING([--without-sendfilev], [Do not use sendfilev])],
        [],
        [with_sendfilev=yes]
)
AC_CHECK_HEADERS([sys/sendfile.h])
AS_IF([test "x$with_splice" != xno],[
  AC_MSG_CHECKING([if splice is supported])
  AC_TRY_LINK([
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <fcntl.h>
    ],
    [
      int fd_in = -1;
      loff_t* off_in = 0;
      int fd_out = -1;
      loff_t* off_out = 0;
      size_t len = 0;
      unsigned int flags = SPLICE_F_MOVE | SPLICE_F_NONBLOCK;
      long result = splice(fd_in, off_in, fd_out, off_out, len, flags);
      return (result < 0) ? 1 : 0;
    ], [
      AC_MSG_RESULT([yes])
      AC_DEFINE([HAVE_SPLICE], [], [Define if splice is supported])
    ], [
      AC_MSG_RESULT([no])
  ])
])
AS_IF([test "x$with_sendfile" != xno],[
  # TODO-check for any necessary libraries (Solaris needs -lsendfile)
  AC_MSG_CHECKING([if sendfile (simple) is supported])
  AC_TRY_LINK([
#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
    ],
    [ 
      int out_fd = -1;
      int in_fd = -1;
      off_t* off = 0;
      size_t len = 0;
      ssize_t result = sendfile(out_fd, in_fd, off, len);
      return (result < 0) ? 1 : 0;
    ], [
      AC_MSG_RESULT([yes])
      AC_DEFINE([HAVE_SENDFILE], [], [Define if sendfile is supported])
    ], [
      AC_MSG_RESULT([no])
  ])
])
AS_IF([test "x$with_sendfile" != xno],[
  AC_MSG_CHECKING([if sendfile (BSD style) is supported])
  AC_TRY_LINK([
#include <sys/types.h>
#include <sys/uio.h>
#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
    ],
    [ 
      int fd = -1;
      int s = -1;
      off_t offset = 0;
      size_t nbytes = 0;
      struct sf_hdtr hdtr = { 0 };
      off_t sbytes = 0;
      int flags = 0;
      ssize_t result = sendfile(fd, s, offset, nbytes, &hdtr, &sbytes, flags);
      return (result < 0) ? 1 : 0;
    ], [
      AC_MSG_RESULT([yes])
      AC_DEFINE([HAVE_SENDFILE], [], [Define if sendfile is supported])
    ], [
      AC_MSG_RESULT([no])
  ])
])
AS_IF([test "x$with_sendfilev" != xno],[
  # TODO-check for any necessary libraries (Solaris needs -lsendfile)
  AC_MSG_CHECKING([if sendfilev is supported])
  AC_TRY_LINK([
#include <sys/types.h>
#include <sys/uio.h>
#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
    ],
    [ 
      int fildes = -1;
      struct sendfilevec vec = { 0 };
      int sfvcnt = 0;
      size_t xferred = 0;
      ssize_t result = sendfilev(fildes, &vec, sfvcnt, &xferred);
      return (result < 0) ? 1 : 0;
    ], [
      AC_MSG_RESULT([yes])
      AC_DEFINE([HAVE_SENDFILEV], [], [Define if sendfilev is supported])
    ], [
      AC_MSG_RESULT([no])
  ])
])
CFLAGS="$saved_CFLAGS"

AC_CONFIG_FILES([Makefile
        include/Makefile
        etc/Makefile
        etc/lwiod.reg
        utils/Makefile
	common/Makefile
        logging_r/Makefile
        ipc/Makefile
        client/Makefile
        server/Makefile
        server/rdr/Makefile
        server/smbcommon/Makefile
        server/iomgr/Makefile
        server/srv/Makefile
        server/srv/utils/Makefile
        server/srv/statistics/Makefile
        server/srv/shares/Makefile
        server/srv/shares/lwshare/Makefile
        server/srv/shares/regshare/Makefile
        server/srv/shares/api/Makefile
        server/srv/elements/Makefile
        server/srv/protocols/Makefile
        server/srv/protocols/api/Makefile
        server/srv/protocols/smb1/Makefile
        server/srv/protocols/smb2/Makefile
        server/srv/transport/Makefile
        server/srv/transport/threadpool/Makefile
        server/srv/driver/Makefile
        server/npfs/Makefile
        server/pvfs/Makefile
        server/dfs/Makefile
        server/lwiod/Makefile
        drivers/Makefile
        drivers/iotest/Makefile
        tools/Makefile
        tools/lwio-refresh/Makefile
        tools/lwio-get-log-info/Makefile
        tools/lwio-set-log-level/Makefile
        tools/lwio-tool/Makefile
        tools/lwio-copy/Makefile
        tools/lwio-driver/Makefile
        tools/lwio-cli/Makefile
        tests/Makefile
        tests/moonunit/Makefile
        tests/test_bitvector/Makefile
        tests/test_npclient/Makefile
        tests/test_npclient2/Makefile
        tests/test_npserver/Makefile
        tests/test_npclient_transactnp/Makefile
        tests/test_pvfs/Makefile
	tests/test_load/Makefile
        tests/tcon/Makefile
        tests/transactnp/Makefile
        scripts/Makefile
        scripts/lwnet
        libshareinfo/Makefile
	fuse/Makefile])

AC_OUTPUT
