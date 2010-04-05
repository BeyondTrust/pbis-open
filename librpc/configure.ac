AC_INIT(idl/samr.idl)
AM_INIT_AUTOMAKE(librpc,0.1)
AC_CONFIG_HEADER([include/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

DCERPC_INCLUDES=""
DCERPC_LDFLAGS=""
DCERPC_PATH=""

PSTORE_INCLUDES=""
PSTORE_LDFLAGS=""

AM_CFLAGS="$AM_CFLAGS -Wall -Werror -fno-strict-aliasing"

AC_SUBST(LTLDFLAGS)
AC_SUBST(AM_CFLAGS)

# Handle options

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

AC_SUBST(LWBASE_INCLUDES)
AC_SUBST(LWBASE_LDFLAGS)


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

# dcerpc

AC_ARG_WITH(
	[dcerpc], 
	[AC_HELP_STRING([--with-dcerpc=<dir>], [look for dcerpc headers/libraries in <dir>])],
	[
	    DCERPC_INCLUDES="-I$withval/include"
	    DCERPC_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	[dcerpc-includes], 
	[AC_HELP_STRING([--with-dcerpc-includes=<dir>], [look for dcerpc headers in <dir>])],
	[
	    DCERPC_INCLUDES="-I$withval"
	]
)

AC_ARG_WITH(
	[dcerpc-libs], 
	[AC_HELP_STRING([--with-dcerpc-libs=<dir>], [look for dcerpc libraries in <dir>])],
	[
	    DCERPC_LDFLAGS="-L$withval"
	]
)

AC_SUBST(DCERPC_INCLUDES)
AC_SUBST(DCERPC_LDFLAGS)


# ldap

AC_ARG_WITH(
	[ldap],
	[AC_HELP_STRING([--with-ldap=<dir>], [look for ldap headers/libraries in <dir>])],
	[
	    LDAP_INCLUDES="-I$withval/include"
	    LDAP_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	[ldap-includes], 
	[AC_HELP_STRING([--with-ldap-includes=<dir>], [look for ldap headers in <dir>])],
	[
	    LDAP_INCLUDES="-I$withval"
	]
)

AC_ARG_WITH(
	[ldap-libs], 
	[AC_HELP_STRING([--with-ldap-libs=<dir>], [look for ldap libraries in <dir>])],
	[
	    LDAP_LDFLAGS="-L$withval"
	]
)

AC_SUBST(LDAP_INCLUDES)
AC_SUBST(LDAP_LDFLAGS)


# pstore

AC_ARG_WITH(
	[pstore], 
	[AC_HELP_STRING([--with-pstore=<dir>], [look for pstore headers/libraries in <dir>])],
	[
	    PSTORE_INCLUDES="-I$withval/include"
	    PSTORE_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	[pstore-includes], 
	[AC_HELP_STRING([--with-pstore-includes=<dir>], [look for pstore headers in <dir>])],
	[
	    PSTORE_INCLUDES="-I$withval"
	]
)

AC_ARG_WITH(
	[pstore-libs],
	[AC_HELP_STRING([--with-pstore-libs=<dir>], [look for pstore libraries in <dir>])],
	[
	    PSTORE_LDFLAGS="-L$withval"
	]
)

AC_SUBST(PSTORE_INCLUDES)
AC_SUBST(PSTORE_LDFLAGS)


# netlogon

AC_ARG_WITH(
	[lwnet], 
	[AC_HELP_STRING([--with-lwnet=<dir>], [look for netlogon headers/libraries in <dir>])],
	[
	    LWNET_INCLUDES="-I$withval/include"
	    LWNET_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	[lwnet-includes], 
	[AC_HELP_STRING([--with-lwnet-includes=<dir>], [look for pstore headers in <dir>])],
	[
	    LWNET_INCLUDES="-I$withval"
	]
)

AC_ARG_WITH(
	[lwnet-libs],
	[AC_HELP_STRING([--with-lwnet-libs=<dir>], [look for pstore libraries in <dir>])],
	[
	    LWNET_LDFLAGS="-L$withval"
	]
)

AC_SUBST(LWNET_INCLUDES)
AC_SUBST(LWNET_LDFLAGS)


# keytab

AC_ARG_WITH(
	[keytab], 
	[AC_HELP_STRING([--with-keytab=<dir>], [look for keytab headers/libraries in <dir>])],
	[
	    KEYTAB_INCLUDES="-I$withval/include"
	    KEYTAB_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	[keytab-includes], 
	[AC_HELP_STRING([--with-keytab-includes=<dir>], [look for keytab headers in <dir>])],
	[
	    KEYTAB_INCLUDES="-I$withval"
	]
)

AC_ARG_WITH(
	[keytab-libs],
	[AC_HELP_STRING([--with-keytab-libs=<dir>], [look for keytab libraries in <dir>])],
	[
	    KEYTAB_LDFLAGS="-L$withval"
	]
)

KEYTAB_LIBS="-lktkrb5"

AC_SUBST(KEYTAB_INCLUDES)
AC_SUBST(KEYTAB_LDFLAGS)
AC_SUBST(KEYTAB_LIBS)


# lwioclient

AC_ARG_WITH(
	[lwioclient], 
	[AC_HELP_STRING([--with-lwioclient=<dir>], [look for lwio client headers/libraries in <dir>])],
	[
	    LWIOCLIENT_INCLUDES="-I$withval/include"
	    LWIOCLIENT_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	[lwioclient-includes], 
	[AC_HELP_STRING([--with-lwioclient-includes=<dir>], [look for lwio client headers in <dir>])],
	[
	    LWIOCLIENT_INCLUDES="-I$withval"
	]
)

AC_ARG_WITH(
	[lwioclient-libs],
	[AC_HELP_STRING([--with-lwioclient-libs=<dir>], [look for lwio client libraries in <dir>])],
	[
	    LWIOCLIENT_LDFLAGS="-L$withval"
	]
)

AC_SUBST(LWIOCLIENT_INCLUDES)
AC_SUBST(LWIOCLIENT_LDFLAGS)


# crypto

AC_ARG_WITH(
	[crypto], 
	[AC_HELP_STRING([--with-crypto=<dir>], [look for crypto headers/libraries in <dir>])],
	[
	    CRYPTO_INCLUDES="-I$withval/include"
	    CRYPTO_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	[crypto-includes], 
	[AC_HELP_STRING([--with-crypto-includes=<dir>], [look for crypto headers in <dir>])],
	[
	    CRYPTO_INCLUDES="-I$withval"
	]
)

AC_ARG_WITH(
	[crypto-libs],
	[AC_HELP_STRING([--with-crypto-libs=<dir>], [look for crypto libraries in <dir>])],
	[
	    CRYPTO_LDFLAGS="-L$withval"
	]
)

AC_SUBST(CRYPTO_INCLUDES)
AC_SUBST(CRYPTO_LDFLAGS)


AC_CHECK_HEADER([lw/base.h],,[AC_MSG_ERROR([Could not find lwbase headers])])
AC_CHECK_HEADER([lw/winerror.h],,[AC_MSG_ERROR([Could not find lwbase headers])])
AC_CHECK_HEADER([lw/ntstatus.h],,[AC_MSG_ERROR([Could not find lwbase headers])])
AC_CHECK_LIB([lwbase], [LwInterlockedIncrement], [LWBASE_LIBS="-llwbase"], [AC_MSG_ERROR([Could not find liblwbase])], [$LWBASE_LDFLAGS])
AC_CHECK_LIB([lwbase_nothr], [LwNtStatusToWin32Error], [LWBASE_NOTHR_LIBS="-llwbase_nothr"], [AC_MSG_ERROR([Could not find liblwbase])], [$LWBASE_LDFLAGS])
AC_SUBST(LWBASE_LIBS)
AC_SUBST(LWBASE_NOTHR_LIBS)

AC_CHECK_HEADER([lwerror.h],,[AC_MSG_ERROR([Could not find lwadvapi headers])])

AC_CHECK_LIB([ldap], [ldap_gssapi_bind_s], [LDAP_LIBS="-lldap_r"], [], [$LDAP_LDFLAGS])
AC_SUBST(LDAP_LIBS)

AC_CHECK_LIB([crypto], [DES_set_key], [CRYPTO_LIBS="-lcrypto"], [], [$CRYPTO_LDFLAGS])


for i in $CPPFLAGS ${DCERPC_INCLUDES}; do
    j=`echo $i | grep '^-I'`
    if test x != x"$j"
    then
        IDLFLAGS="$IDLFLAGS $j"
    fi
done
AC_SUBST(IDLFLAGS)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

AC_CHECK_SIZEOF([long int])

# Checks for library functions.
AC_CHECK_FUNCS(strndup)

LIBICONV_LDFLAGS=""
AC_CHECK_LIB(iconv, iconv, [LIBICONV_LDFLAGS="-liconv"])

# Checks for libraries.
CPPFLAGS="$CPPFLAGS $PSTORE_INCLUDES"
AC_CHECK_LIB([lwpsapi], [LwpsOpenPasswordStore], [PSTORE_LIBS="-llwpsapi"],
     AC_MSG_ERROR([PStore library not found]), [$PSTORE_LDFLAGS])
AC_SUBST(PSTORE_LIBS)

CPPFLAGS="$CPPFLAGS $DCERPC_INCLUDES"
AC_CHECK_LIB([dcerpc], [rpc__init], [DCERPC_LIBS="-ldcerpc"],
     AC_MSG_ERROR([DCE/RPC library not found]), [$DCERPC_LDFLAGS])
AC_SUBST(DCERPC_LIBS)

CPPFLAGS="$CPPFLAGS $LWNET_INCLUDES"
AC_CHECK_LIB([lwnetclientapi], [LWNetGetDCName], [LWNET_LIBS="-llwnetclientapi"],
     AC_MSG_ERROR([Netlogon daemon client library not found]), [$LWNET_LDFLAGS])
AC_SUBST(LWNET_LIBS)

AC_CHECK_LIB([tdb], [tdb_fetch], [
		AC_DEFINE(HAVE_LIBTDB, 1, [Set 1 one if the tdbbackend should be used])
		have_libtdb=true
		],
		[have_libtdb=false])
AM_CONDITIONAL([HAVE_LIBTDB], [test x$have_libtdb = xtrue])

AC_CHECK_LIB([lwioclient], [LwIoGetThreadCreds], [LWIOCLIENT_LIBS="-llwioclient"],
     AC_MSG_ERROR([liblwioclient not found]), [$LWIOCLIENT_LDFLAGS])
AC_SUBST(LWIOCLIENT_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([stdlib.h string.h strings.h inttypes.h sys/varargs.h])

# Check for programs
AC_PATH_PROG([IDL], [dceidl], [no], [$PATH:$DCERPC_PATH])

if test x"$IDL" = x"no"; then
   AC_MSG_ERROR([DCERPC IDL compiler not found])
fi

# Check for moonunit bits
AC_CHECK_HEADERS([moonunit/moonunit.h], [HAVE_MOONUNIT_H=true], [HAVE_MOONUNIT_H=false])
AC_CHECK_LIB([moonunit], [Mu_Interface_Result], [HAVE_MOONUNIT_LIB=true], [HAVE_MOONUNIT_LIB=false])
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

AC_OUTPUT(Makefile
          include/Makefile
          alloc/Makefile
          msrpc/Makefile
          mpr/Makefile
          samr/Makefile
          lsa/Makefile
          krb5pac/Makefile
          netlib/Makefile
          netlogon/Makefile
          dssetup/Makefile
          testrpc/Makefile)
