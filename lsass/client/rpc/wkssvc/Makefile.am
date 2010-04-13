rpccli_srcdir = $(top_srcdir)/client/rpc
idl_srcdir = $(top_srcdir)/client/rpc/idl
lsass_srcdir = $(top_srcdir)

noinst_LTLIBRARIES = libwkssvc_cs.la libwkssvc.la

libwkssvc_la_SOURCES =       \
    globals.c                \
    wkss_binding.c           \
    wkss_memory.c            \
    wkss_stubmemory.c        \
    netr_wkstagetinfo.c      \
    netr_joindomain2.c       \
    netr_unjoindomain2.c


DCEIDL_PASS_INCLUDES =         \
    $(CPPFLAGS)                \
    $(LWBASE_INCLUDES)         \
    -I$(lsass_srcdir)/include

libwkssvc_cs_la_SOURCES = wkssvc_cstub.c

libwkssvc_cs_la_CPPFLAGS =       \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    -I$(lsass_srcdir)/include

libwkssvc_cs_la_CFLAGS =    \
    $(AM_CFLAGS)            \
    -DDCERPC_STUB_BUILD

libwkssvc_la_CPPFLAGS =          \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    -I$(lsass_srcdir)/include

libwkssvc_la_CFLAGS =  \
    $(AM_CFLAGS)       \
    -DLIBRPC_BUILD

libwkssvc_la_LDFLAGS =  \
    $(AM_LDFLAGS)       \
    $(LWBASE_LDFLAGS)   \
    $(DCERPC_LDFLAGS)

libwkssvc_la_LIBADD =  \
    $(LWBASE_LIBS)     \
    $(DCERPC_LIBS)     \
    libwkssvc_cs.la

CLEANFILES = wkssvc_h.h wkssvc_cstub.c wkssvc_sstub.c

BUILT_SOURCES = wkssvc_h.h

wkssvc_cstub.c wkssvc_h.h: $(idl_srcdir)/wkssvc.idl wkssvc.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header wkssvc_h.h -cstub_pref cli_ -sstub_pref srv_ \
		-I$(idl_srcdir) -I$(srcdir) $(DCEIDL_PASS_INCLUDES) $<
