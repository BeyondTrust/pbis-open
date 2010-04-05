librpc_srcdir = $(top_srcdir)
idl_srcdir = $(top_srcdir)/idl

noinst_LTLIBRARIES = liblsa_cs.la liblsa.la

liblsa_la_SOURCES =         \
    globals.c               \
    lsa_binding.c           \
    lsa_memory.c            \
    lsa_stubmemory.c        \
    lsa_openpolicy2.c       \
    lsa_close.c             \
    lsa_lookupnames.c       \
    lsa_lookupnames2.c      \
    lsa_lookupnames3.c      \
    lsa_lookupsids.c        \
    lsa_queryinfopolicy.c   \
    lsa_queryinfopolicy2.c

liblsa_cs_la_SOURCES = lsa_cstub.c

liblsa_cs_la_CPPFLAGS =          \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    -I$(librpc_srcdir)/include

liblsa_cs_la_CFLAGS =    \
    $(AM_CFLAGS)         \
    -DDCERPC_STUB_BUILD

liblsa_la_CPPFLAGS =             \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    $(LIBUNISTR_INCLUDES)        \
    -I$(librpc_srcdir)/include

liblsa_la_CFLAGS =  \
    $(AM_CFLAGS)    \
    -DLIBRPC_BUILD

liblsa_la_LDFLAGS =       \
    $(AM_LDFLAGS)         \
    $(LWBASE_LDFLAGS)     \
    $(DCERPC_LDFLAGS)     \
    $(LIBUNISTR_LDFLAGS)

liblsa_la_LIBADD =     \
    $(LWBASE_LIBS)     \
    $(DCERPC_LIBS)     \
    $(LIBUNISTR_LIBS)  \
    liblsa_cs.la

CLEANFILES = lsa_h.h lsa_cstub.c lsa_sstub.c

BUILT_SOURCES = lsa_h.h

LIBRPC_INCLUDES = -I$(librpc_srcdir)/include

lsa_cstub.c lsa_h.h: $(idl_srcdir)/lsa.idl lsa.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header lsa_h.h \
		-I$(idl_srcdir) -I$(srcdir) $(LIBRPC_INCLUDES) $<
