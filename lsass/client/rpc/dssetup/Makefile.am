rpccli_srcdir = $(top_srcdir)/client/rpc
idl_srcdir = $(top_srcdir)/idl
lsass_srcdir = $(top_srcdir)

noinst_LTLIBRARIES = libdssetup_cs.la libdssetup.la

libdssetup_la_SOURCES =            \
    dsr_rolegetprimarydomaininfo.c \
    dsr_memory.c                   \
    dsr_stubmemory.c               \
    dsr_binding.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(LWBASE_INCLUDES) \
    -I$(lsass_srcdir)/include

libdssetup_cs_la_SOURCES = dssetup_cstub.c

libdssetup_cs_la_CPPFLAGS =      \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    -I$(librpc_srcdir)/include

libdssetup_cs_la_CFLAGS = \
    -DDCERPC_STUB_BUILD

libdssetup_la_CPPFLAGS = \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(LWADVAPI_INCLUDES)         \
    $(DCERPC_INCLUDES)           \
    -I$(librpc_srcdir)/include

libdssetup_la_CFLAGS= \
    $(AM_CFLAGS)

libdssetup_la_LDFLAGS = \
    $(AM_LDFLAGS)       \
    $(DCERPC_LDFLAGS)

libdssetup_la_LIBADD=   \
    $(LWBASE_LIBS)      \
    $(DCERPC_LIBS)      \
    libdssetup_cs.la

CLEANFILES = dssetup_h.h dssetup_cstub.c dssetup_sstub.c

BUILT_SOURCES = dssetup_h.h

dssetup_cstub.c dssetup_h.h: $(idl_srcdir)/dssetup.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header dssetup_h.h -cstub_pref cli_ -sstub_pref srv_ \
		-I$(idl_srcdir) -I$(srcdir) $(DCEIDL_PASS_INCLUDES) $<
