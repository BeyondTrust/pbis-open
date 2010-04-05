AC_INIT([moonunit],[0.5],[bkoropoff __at__ gmail_com])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_CXX

case "$host_os" in
    solaris2.*)
	STDFLAGS="-std=c99"
	CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200112L -D__EXTENSIONS__"
	CC="$CC $STDFLAGS"
	moduleext=".so"
	;;
    aix5.*|aix6.*)
	moduleext=".so"
	LDFLAGS="$LDFLAGS -Wl,-brtl"
	;;
    linux-gnu)
	STDFLAGS="-std=c99"
	CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=500"
	CC="$CC $STDFLAGS"
        moduleext=".so"
	;;
    hpux11.*)
	case "$host_cpu" in
	    hppa2.0*)
	        moduleext=".sl"
	        LDFLAGS="$LDFLAGS -Wl,+s"
	        ;;
            ia64)
                moduleext=".so"
                LDFLAGS="$LDFLAGS -Wl,+s"
		;;
	    *)
	        AC_MSG_ERROR([Unsupported architecture: $host_cpu])
		;;
        esac
	;;
    darwin8.*|darwin9.*|darwin10.*)
	moduleext=".so"
	;;
    freebsd6.*|freebsd7.*|freebsd8.*)
	STDFLAGS="-std=c99"
	CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L -D__BSD_VISIBLE=1"
	CC="$CC $STDFLAGS"
	moduleext=".so"
	;;
    *)
	AC_MSG_ERROR([Unsupported OS: $host_os])
	;;
esac

plugindir="${libdir}/moonunit"
AC_SUBST(plugindir)
AC_SUBST(moduleext)
AC_SUBST(memberext)
AC_DEFINE_UNQUOTED([DSO_EXT], ["$moduleext"], [Full extension of dynamic shared objects])
AM_CONDITIONAL([HAVE_MEMBEREXT], [test -n "$memberext"])

WANT_CPLUSPLUS="true"
WANT_LIBELF="true"
WANT_DOC="true"

AC_ARG_ENABLE([cpp],
	[AC_HELP_STRING([--disable-cpp], [disable C++ support (default: enabled)])],
	[
		if test x"$enableval" = x"no"
		then
			WANT_CPLUSPLUS="false"
		fi
	])

AC_ARG_ENABLE([debug],
	[AC_HELP_STRING([--enable-debug], [enable debugging])],
	[
		if test x"$enableval" = x"yes"
		then
			AC_DEFINE([DEBUG], [], [Debugging enabled])
			CFLAGS="$CFLAGS -O0 -g"
			CXXFLAGS="$CXXFLAGS -O0 -g"
		fi
	])

AC_ARG_ENABLE([doc],
	[AC_HELP_STRING([--disable-doc], [disable documentation])],
	[
		if test x"$enableval" = x"no"
		then
			WANT_DOC="false"
		fi
	])

AC_ARG_WITH([libelf],
	[AC_HELP_STRING([--with-libelf=<dir>],
		[use libelf located in <dir> to scan libraries for unit tests, or 'no' to disable])],
	[
		if test x"$withval" = x"no"
		then
			WANT_LIBELF=false
			LIBELF_INCLUDES=
	 		LIBELF_LDFLAGS=
	 	elif test x"$withval" = x"yes"
	 	then
	 		WANT_LIBELF=true
	 		LIBELF_INCLUDES=
	 		LIBELF_LDFLAGS=
	 	else
			LIBELF_INCLUDES="-I$withval/include"
	 		LIBELF_LDFLAGS="-L$withval/lib",
	 	fi
	])

AC_ARG_WITH([docbook-xsl],
	[AC_HELP_STRING([--with-docbook-xsl=<dir>],
		[use DocBook XML stylesheets located in <dir>])],
	[DOCBOOK_SEARCH_PATH="$withval"])

AC_CONFIG_HEADERS(include/config.h)

if $WANT_CPLUSPLUS
then
   AC_DEFINE(CPLUSPLUS_ENABLED, [], [Define if you want C++ support])
fi
AM_CONDITIONAL([CPLUSPLUS_ENABLED], [$WANT_CPLUSPLUS])
AM_CONDITIONAL([DOC_ENABLED], [$WANT_DOC])

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MKDIR_P
AC_PATH_PROG(BASH_PATH, bash)

AC_SUBST(MKDIR_P)

# Begin C checks
AC_LANG_C

# Types
AC_CHECK_SIZEOF(void*)

AC_CHECK_TYPE(suseconds_t, [USEC_T="suseconds_t"], [USEC_T="long int"],
			   [#include <sys/time.h>])
AC_DEFINE_UNQUOTED([USEC_T], [$USEC_T], [Type of tv_usec field of struct timeval])

AC_CHECK_HEADERS([signal.h select.h sys/select.h execinfo.h sys/ioctl.h])

AC_CHECK_LIB(socket, send, [LIB_socket="-lsocket"])
AC_CHECK_LIB(dl, dlopen, [LIB_dl="-ldl"])
AC_CHECK_LIB(pthread, pthread_self, [LIB_pthread="-lpthread"])

AC_SUBST(LIB_socket)
AC_SUBST(LIB_dl)
AC_SUBST(LIB_pthread)

AC_CHECK_FUNCS([strsignal backtrace backtrace_symbols setpgid setpgrp tcgetpgrp tcsetpgrp sigtimedwait])


AC_MSG_CHECKING([symbol layout order])
AC_TRY_RUN(
	[
		struct s { int x, y; };

		struct s a = {0,1};
		struct s b = {2,3};

		int main (int argc, char** argv)
		{
			if (&b > &a)
			   return 0;
			else
			   return 1;
		}
	],
	[
		AC_MSG_RESULT([forward])
	],
	[
		AC_MSG_RESULT([reverse])
		AC_DEFINE([SYMBOL_LAYOUT_REVERSE], [], [Define if symbols are laid out in reverse textual order in object file])
	])

saved_LDFLAGS=$LDFLAGS
saved_CPPFLAGS=$CPPFLAGS

LDFLAGS="$LDFLAGS $LIBELF_LDFLAGS"
CPPFLAGS="$CPPFLAGS $LIBELF_INCLUDES"

if $WANT_LIBELF
then
	AC_CHECK_HEADER([libelf.h],[FOUND_LIBELF_H=true],[FOUND_LIBELF_H=false])
	AC_CHECK_LIB(elf, elf_begin, [FOUND_LIBELF_LIB=true], [FOUND_LIBELF_LIB=false])

	if $FOUND_LIBELF_H && $FOUND_LIBELF_LIB
	then
		LIBELF_LIBADD="$LIBELF_LDFLAGS -lelf"
		AC_DEFINE(HAVE_LIBELF, [], [Define if libelf is available])
		FOUND_LIBELF=true
		BACKEND=libelf
	else
		FOUND_LIBELF=false
		BACKEND=none
	fi
else
	FOUND_LIBELF=false
	BACKEND=none
fi

AM_CONDITIONAL([HAVE_LIBELF], [$FOUND_LIBELF])

AC_SUBST(LIBELF_INCLUDES)
AC_SUBST(LIBELF_LIBADD)

LDFLAGS=$saved_LDFLAGS
CPPFLAGS=$saved_CPPFLAGS

CFLAGS="$CFLAGS -Wall -Werror -Wmissing-prototypes"

# Begin C++ checks
if $WANT_CPLUSPLUS
then
	AC_LANG_CPLUSPLUS
	AC_CHECK_HEADERS([cxxabi.h])

	CXXFLAGS="$CXXFLAGS -Wall -Werror"
fi
			
# Begin program/misc checks
 
AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen], [false])
if test x"$DOXYGEN" != x"false"
then
	AC_MSG_CHECKING([doxygen version])
	doxy_triple="`doxygen --version | sed 's/\./ /g'`"
	doxy_major="`echo $doxy_triple | awk '{print $1;}'`"
	doxy_minor="`echo $doxy_triple | awk '{print $2;}'`"
	doxy_micro="`echo $doxy_triple | awk '{print $3;}'`"
	doxy_version="`expr $doxy_major '*' 10000 + $doxy_minor '*' 100 + $doxy_micro`"

	if test "$doxy_version" -ge 10503
	then
		AC_MSG_RESULT([supported])
		HAVE_DOXYGEN=true
	else
		AC_MSG_RESULT([unsupported])
		HAVE_DOXYGEN=false
	fi
else
	HAVE_DOXYGEN=false
fi

AM_CONDITIONAL([HAVE_DOXYGEN], [$HAVE_DOXYGEN])
  
AC_CHECK_PROG([XSLTPROC], [xsltproc], [xsltproc], [false])
if test x"$XSLTPROC" != x"false"
then
	HAVE_XSLTPROC=true
else
	HAVE_XSLTPROC=false
fi

AC_MSG_CHECKING([DocBook XSL stylesheet directory])

if test -z "${DOCBOOK_SEARCH_PATH}"
then
	DOCBOOK_SEARCH_PATH="\
		/usr/share/xml/docbook/stylesheet/nwalsh \
		/usr/share/xml/docbook/stylesheet/nwalsh/current \
		/usr/share/sgml/docbook/xsl-stylesheets \
		/usr/local/share/xsl/docbook
		"
fi

for path in ${DOCBOOK_SEARCH_PATH}
do
	if test -d "$path" -a -d "$path/xhtml" -a -d "$path/manpages"
	then
		DOCBOOK_XSL_DIR="$path"
	fi
done

if test x"${DOCBOOK_XSL_DIR}" = x
then
	HAVE_DOCBOOK_XSL=false
	AC_MSG_RESULT(not found)
else
	HAVE_DOCBOOK_XSL=true
	AC_MSG_RESULT($DOCBOOK_XSL_DIR)
fi

if $HAVE_XSLTPROC && $HAVE_DOCBOOK_XSL
then
	HAVE_DOCBOOK=true
else
	HAVE_DOCBOOK=false
fi

AC_SUBST(DOCBOOK_XSL_DIR)
AM_CONDITIONAL([HAVE_DOCBOOK], [$HAVE_DOCBOOK])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/libmoonunit/Makefile
	src/moonunit/Makefile
	src/moonunit/moonunit-lt.sh
	src/moonunit/moonunit-stub.sh
	include/Makefile
	include/moonunit/Makefile
	include/moonunit/private/Makefile
	include/moonunit/internal/Makefile
	include/uipc/Makefile
	src/libuipc/Makefile
	src/plugins/Makefile
	src/plugins/c/Makefile
	src/plugins/shell/Makefile
	src/plugins/shell/mu.sh
	src/plugins/console/Makefile
	src/plugins/xml/Makefile
	test/Makefile
	doc/Makefile
	doc/Doxyfile
	doc/docbook-html.xsl
	doc/docbook-man.xsl
	])

AC_OUTPUT

echo ""
echo ""
echo "Configuration summary"
echo
echo "Reflection backend:    $BACKEND"
if test x"$BACKEND" = x"none"
then
	echo "    Warning: reflective test loading is unavailable."
	echo "    Unit test modules must include a stub generated by"
	echo "    moonunit-stub to be loadable."
fi
echo "C++ support:           `$WANT_CPLUSPLUS && echo yes || echo no`"
echo ""
