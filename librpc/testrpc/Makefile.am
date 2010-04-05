librpc_srcdir = $(top_srcdir)
liballoc_builddir = $(top_builddir)/alloc
libnetapi_builddir = $(top_builddir)/netlib

noinst_PROGRAMS = testrpc

testrpc_SOURCES =        \
	testrpc.c        \
	params.c         \
	test_lsa.c       \
	test_samr.c      \
	test_netlogon.c  \
	test_netapi.c    \
        test_dssetup.c

testrpc_CPPFLAGS =             \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(LWADVAPI_INCLUDES)       \
    -I$(librpc_srcdir)/include

testrpc_CFLAGS =               \
    $(AM_CFLAGS)               \
    -D_GNU_SOURCE              \
    -DLIBRPC_BUILD

testrpc_LDFLAGS =          \
    $(AM_LDFLAGS)          \
    $(LWBASE_LDFLAGS)      \
    $(LWADVAPI_LDFLAGS)    \
    $(LIBUNISTR_LDFLAGS)   \
    $(LWNET_LDFLAGS)       \
    $(LWIOCLIENT_LDFLAGS)  \
    -rpath ${libdir}

testrpc_LDADD =                          \
    $(LWBASE_LIBS)                       \
    $(LWADVAPI_LIBS)                     \
    $(LIBUNISTR_LIBS)                    \
    $(LWNET_LIBS)                        \
    $(LWIOCLIENT_LIBS)                   \
    $(liballoc_builddir)/liballoc.la     \
    $(libnetapi_builddir)/libnetapi.la
