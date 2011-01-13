netapi_builddir = $(top_builddir)/src

bin_PROGRAMS = netapitest

netapitest_SOURCES =    \
    netapitest.c        \
    params.c            \
    common.c            \
    test_netapi.c       \
    test_samr.c         \
    test_lsa.c          \
    test_netlogon.c     \
    test_dssetup.c      \
    test_wkssvc.c

netapitest_CPPFLAGS =          \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(LWADVAPI_INCLUDES)       \
    $(LASSS_INCLUDES)          \
    -I$(top_srcdir)/include

netapitest_CFLAGS =            \
    $(AM_CFLAGS)               \
    -D_GNU_SOURCE


netapitest_LDFLAGS =           \
    $(AM_LDFLAGS)              \
    $(LWBASE_LDFLAGS)          \
    $(LWADVAPI_LDFLAGS)        \
    $(LASSS_INCLUDES)

netapitest_LDADD =                            \
    $(LWBASE_LIBS)                            \
    $(LWADVAPI_LIBS)                          \
    $(LSASS_LIBS)                             \
    $(netapi_builddir)/liblwnetapi.la
