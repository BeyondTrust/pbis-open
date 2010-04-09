rpccli_builddir = $(top_builddir)/client/rpc

bin_PROGRAMS = rpctest

rpctest_SOURCES =    \
    testrpc.c        \
    params.c         \
    test_lsa.c       \
    test_samr.c      \
    test_netlogon.c  \
    test_dssetup.c

rpctest_CPPFLAGS =             \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(LWADVAPI_INCLUDES)       \
    -I$(top_srcdir)/include

rpctest_CFLAGS =               \
    $(AM_CFLAGS)               \
    -D_GNU_SOURCE              \
    -DLIBRPC_BUILD

rpctest_LDFLAGS =          \
    $(AM_LDFLAGS)          \
    $(LWBASE_LDFLAGS)      \
    $(LWADVAPI_LDFLAGS)    \
    $(LWNET_LDFLAGS)       \
    $(LWIOCLIENT_LDFLAGS)  \
    -rpath ${libdir}

rpctest_LDADD =                               \
    $(LWBASE_LIBS)                            \
    $(LWADVAPI_LIBS)                          \
    $(LWNET_LIBS)                             \
    $(LWIOCLIENT_LIBS)                        \
    $(rpccli_builddir)/liblsassrpc.la
