netapi_builddir = $(top_builddir)/src

noinst_PROGRAMS = netapitest

netapitest_SOURCES =    \
    netapitest.c     \
    params.c         \
    test_netapi.c

netapitest_CPPFLAGS =          \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(LWADVAPI_INCLUDES)       \
    -I$(top_srcdir)/include

netapitest_CFLAGS =            \
    $(AM_CFLAGS)               \
    -D_GNU_SOURCE


netapitest_LDFLAGS =                    \
    $(AM_LDFLAGS)                       \
    $(LWBASE_LDFLAGS)                   \
    $(LWADVAPI_LDFLAGS)

netapitest_LDADD =                            \
    $(LWBASE_LIBS)                            \
    $(LWADVAPI_LIBS)                          \
    $(netapi_builddir)/liblwnetapi.la
