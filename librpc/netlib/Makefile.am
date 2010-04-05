librpc_srcdir = $(top_srcdir)

lib_LTLIBRARIES = libnetapi.la

libnetapi_la_SOURCES =            \
    globals.c                     \
    net_connection.c              \
    net_memory.c                  \
    net_userinfo.c                \
    net_groupinfo.c               \
    net_memberinfo.c              \
    net_crypto.c                  \
    net_userenum.c                \
    net_useradd.c                 \
    net_userdel.c                 \
    net_user.c                    \
    net_usergetinfo.c             \
    net_usersetinfo.c             \
    net_usergetlocalgroups.c      \
    net_localgroupenum.c          \
    net_localgroupadd.c           \
    net_localgroupdel.c           \
    net_localgroupgetinfo.c       \
    net_localgroupsetinfo.c       \
    net_localgroupmembers.c       \
    net_localgroupgetmembers.c    \
    net_joindomain.c              \
    joinlocal.c                   \
    net_unjoindomain.c            \
    unjoinlocal.c                 \
    net_bufferfree.c              \
    net_userchangepassword.c      \
    net_getdomainname.c           \
    net_machinechangepassword.c   \
    ldaputil.c                    \
    net_debug.c                   \
    machinepassword.c             \
    net_getdcname.c               \
    net_hostinfo.c                \
    net_credentials.c

libnetapi_la_CPPFLAGS =         \
    $(AM_CPPFLAGS)              \
    $(LWBASE_INCLUDES)          \
    $(LWADVAPI_INCLUDES)        \
    $(LWNET_INCLUDES)           \
    $(DCERPC_INCLUDES)          \
    $(LIBUNISTR_INCLUDES)       \
    $(PSTORE_INCLUDES)          \
    $(KEYTAB_INCLUDES)          \
    -I$(librpc_srcdir)/include  \
    -I$(libtdb_srcdir)/include


libnetapi_la_LIBADD = \
    $(LWBASE_LIBS)                            \
    $(LWADVAPI_LIBS)                          \
    $(LIBUNISTR_LIBS)                         \
    $(LWNET_LIBS)                             \
    $(LDAP_LIBS)                              \
    $(KEYTAB_LIBS)                            \
    $(LWIOCLIENT_LIBS)                        \
    $(CRYPTO_LIBS)                            \
    $(top_builddir)/alloc/liballoc.la         \
    $(top_builddir)/samr/libsamr.la           \
    $(top_builddir)/lsa/liblsa.la             \
    $(top_builddir)/netlogon/libnetlogon.la   \
    $(top_builddir)/msrpc/libmsrpc.la         \
    $(top_builddir)/krb5pac/libkrb5pac.la     \
    $(top_builddir)/dssetup/libdssetup.la

if HAVE_LIBTDB
libnetapi_la_LIBADD += -ltdb
endif

libnetapi_la_CFLAGS =      \
    $(AM_CFLAGS)           \
    -DLIBRPC_BUILD

libnetapi_la_LDFLAGS =     \
    $(AM_LDLAGS)           \
    $(LWBASE_LDFLAGS)      \
    $(LWADVAPI_LDFLAGS)    \
    $(LIBUNISTR_LDFLAGS)   \
    $(LWNET_LDFLAGS)       \
    $(LDAP_LDFLAGS)        \
    $(KEYTAB_LDFLAGS)      \
    $(LWIOCLIENT_LDFLAGS)  \
    $(CRYPTO_LDFLAGS)

