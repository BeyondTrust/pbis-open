librpc_srcdir = $(top_srcdir)
idl_srcdir = $(top_srcdir)/idl

noinst_LTLIBRARIES = libnetlogon_cs.la libnetlogon.la

libnetlogon_la_SOURCES =              \
    globals.c                         \
    netr_binding.c                    \
    netr_memory.c                     \
    netr_stubmemory.c                 \
    netr_credentials.c                \
    netr_serverreqchallenge.c         \
    netr_serverauthenticate.c         \
    netr_serverauthenticate2.c        \
    netr_serverauthenticate3.c        \
    netr_schannel.c                   \
    netr_samlogon.c                   \
    netr_samlogoff.c                  \
    netr_samlogonex.c                 \
    netr_enumeratetrusteddomainsex.c  \
    netr_getdomaininfo.c              \
    dsr_enumeratedomaintrusts.c       \
    dsr_getdcname.c

libnetlogon_cs_la_SOURCES = netlogon_cstub.c

libnetlogon_cs_la_CPPFLAGS =    \
    $(AM_CPPFLAGS)              \
    $(LWBASE_INCLUDES)          \
    $(LWADVAPI_INCLUDES)        \
    $(DCERPC_INCLUDES)          \
    -I$(librpc_srcdir)/include

libnetlogon_cs_la_CFLAGS =      \
    $(AM_CFLAGS)                \
    -DDCERPC_STUB_BUILD

libnetlogon_la_CPPFLAGS =       \
    $(AM_CPPFLAGS)              \
    $(LWBASE_INCLUDES)          \
    $(LWADVAPI_INCLUDES)        \
    $(DCERPC_INCLUDES)          \
    $(LIBUNISTR_INCLUDES)       \
    -I$(librpc_srcdir)/include

libnetlogon_la_CFLAGS =         \
    $(AM_CFLAGS)                \
    -DLIBRPC_BUILD

libnetlogon_la_LDFLAGS =   \
    $(AM_LDFLAGS)          \
    $(LWBASE_LDFLAGS)      \
    $(LWADVAPI_LDFLAGS)    \
    $(DCERPC_LDFLAGS)      \
    $(LIBUNISTR_LDFLAGS)

libnetlogon_la_LIBADD =    \
    $(LWBASE_LIBS)         \
    $(LWADVAPI_LIBS)       \
    $(DCERPC_LIBS)         \
    $(LIBUNISTR_LIBS)      \
    libnetlogon_cs.la

CLEANFILES = netlogon_h.h netlogon_cstub.c netlogon_sstub.c

BUILT_SOURCES = netlogon_h.h

LIBRPC_INCLUDES = -I$(librpc_srcdir)/include

netlogon_cstub.c netlogon_h.h: $(idl_srcdir)/netlogon.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header netlogon_h.h \
		-I$(idl_srcdir) -I$(srcdir) $(LIBRPC_INCLUDES) $<
