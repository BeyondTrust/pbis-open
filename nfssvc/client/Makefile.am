AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libnfssvc_cs.la libnfssvc.la

idl_srcdir = $(top_srcdir)/idl
include_srcdir = $(top_srcdir)/include

libnfssvc_la_SOURCES =        \
    binding.c                 \
    netr_connectionenum.c     \
    netr_fileenum.c           \
    netr_filegetinfo.c        \
    netr_fileclose.c          \
    netr_sessiondel.c         \
    netr_sessionenum.c        \
    netr_shareadd.c           \
    netr_shareenum.c          \
    netr_sharegetinfo.c       \
    netr_sharesetinfo.c       \
    netr_sharedel.c           \
    netr_servergetinfo.c      \
    netr_serversetinfo.c      \
    netr_remotetod.c          \
    memory.c                  \
    stubmemory.c              \
    memptr.c                  \
    globals.c

libnfssvc_cs_la_SOURCES = nfssvc_cstub.c

libnfssvc_cs_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -DDCERPC_STUB_BUILD    \
    $(DCERPC_INCLUDES)

libnfssvc_cs_la_CPFLAGS = \
    -DDCERPC_STUB_BUILD   \
    $(DCERPC_INCLUDES)

libnfssvc_la_CFLAGS =   \
    -DnfsSVC_BUILD      \
    -DLIBRPC_BUILD      \
    $(INCLUDES)

libnfssvc_la_CPPFLAGS =   \
    $(AM_CPPFLAGS)        \
    $(DCERPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES)

libnfssvc_la_LDFLAGS = \
    $(DCERPC_LDFLAGS)   \
    $(LIBUNISTR_LDFLAGS)

libnfssvc_la_LIBADD = \
    @top_builddir@/logging_r/libnfssvclog_r.la \
    @top_builddir@/utils/libnfssvcutils.la \
    $(DCERPC_LIBS)    \
    $(LIBUNISTR_LIBS) \
    libnfssvc_cs.la

CLEANFILES = nfssvc_h.h nfssvc_cstub.c nfssvc_sstub.c

BUILT_SOURCES = nfssvc_h.h

nfssvc_h.h nfssvc_cstub.c nfssvc_sstub.c: $(idl_srcdir)/nfssvc.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header nfssvc_h.h \
		-I$(idl_srcdir) -I$(srcdir) -I$(include_srcdir) $<
