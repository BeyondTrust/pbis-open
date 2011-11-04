rpccli_srcdir = $(top_srcdir)/client/rpc
lsass_srcdir = $(top_srcdir)

noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES =         \
    binding.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(LWBASE_INCLUDES) \
    -I$(lsass_srcdir)/include

libcommon_la_CPPFLAGS =          \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    -I$(lsass_srcdir)/include

libcommon_la_LDFLAGS =       \
    $(AM_LDFLAGS)         \
    $(LWBASE_LDFLAGS)     \
    $(DCERPC_LDFLAGS)
