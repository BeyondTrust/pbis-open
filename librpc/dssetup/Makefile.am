librpc_srcdir = $(top_srcdir)
idl_srcdir=$(top_srcdir)/idl

noinst_LTLIBRARIES = libdssetup_cs.la libdssetup.la

libdssetup_la_SOURCES =            \
    dsr_rolegetprimarydomaininfo.c \
    dsr_memory.c                   \
    dsr_stubmemory.c               \
    dsr_binding.c

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
    $(LIBUNISTR_INCLUDES)        \
    -I$(librpc_srcdir)/include

libdssetup_la_CFLAGS= \
    $(AM_CFLAGS)      \
    -DLIBRPC_BUILD

libdssetup_la_LDFLAGS = \
    $(AM_LDFLAGS)       \
    $(DCERPC_LDFLAGS)   \
    $(LIBUNISTR_LDFLAGS)

libdssetup_la_LIBADD=   \
    $(LWBASE_LIBS)      \
    $(DCERPC_LIBS)      \
    $(LIBUNISTR_LIBS)   \
    libdssetup_cs.la

CLEANFILES = dssetup_h.h dssetup_cstub.c dssetup_sstub.c

BUILT_SOURCES = dssetup_h.h

LIBRPC_INCLUDES = -I$(librpc_srcdir)/include

dssetup_cstub.c dssetup_h.h: $(idl_srcdir)/dssetup.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header dssetup_h.h \
		-I$(idl_srcdir) -I$(srcdir) $(LIBRPC_INCLUDES) $<
