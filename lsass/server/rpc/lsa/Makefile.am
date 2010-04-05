srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(rpc_srcdir)/idl

if ENABLE_STATIC_RPCSERVERS
noinst_LTLIBRARIES = liblsa_srv.la
else
rpcserver_LTLIBRARIES = liblsa_srv.la
noinst_LTLIBRARIES = liblsa_ss.la liblsa.la
endif

liblsa_la_SOURCES = \
    globals.c                   \
    lsa.c                       \
    lsa_cfg.c                   \
    lsa_srv.c                   \
    lsa_memory.c                \
    lsa_openpolicy2.c           \
    lsa_close.c                 \
    lsa_lookupnames.c           \
    lsa_lookupsids.c            \
    lsa_lookupnames2.c          \
    lsa_lookupsids2.c           \
    lsa_lookupnames3.c          \
    lsa_queryinfopolicy.c       \
    lsa_queryinfopolicy2.c      \
    lsa_domaincache.c           \
    lsa_accounts.c              \
    lsa_accesstoken.c           \
    lsa_security.c              \
    lsa_contexthandle.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(NETAPI_INCLUDES) \
    $(LWBASE_INCLUDES)

liblsa_la_CFLAGS = \
    $(AM_CFLAGS)             \
    -DLIBRPC_BUILD           \
    $(FEATURETEST_FLAGS)     \
    -I$(srv_srcdir)/include  \
    -I$(top_srcdir)/include  \
    -I$(rpc_srcdir)/samr     \
    -I$(srv_srcdir)/api      \
    $(DCERPC_INCLUDES)       \
    $(LIBUNISTR_INCLUDES)    \
    $(LWBASE_INCLUDES)       \
    $(LWADVAPI_INCLUDES)     \
    $(NETLOGON_INCLUDES)

liblsa_la_LIBADD = \
    $(DCERPC_LIBS)                        \
    $(NETAPI_LIBS)                        \
    $(LIBUNISTR_LIBS)                     \
    $(LWBASE_LIBS)                        \
    $(LWADVAPI_LIBS)                      \
    $(NETLOGON_LIBS)                      \
    $(top_builddir)/server/api/liblsaserverapi.la  \
    $(top_builddir)/server/store/dsapi/libdsapi.la

liblsa_ss_la_SOURCES = \
    lsa_sstub.c

liblsa_ss_la_CFLAGS = \
    $(AM_CFLAGS)             \
    -DDCERPC_STUB_BUILD   \
    $(FEATURETEST_FLAGS)  \
    $(DCERPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES) \
    $(LWBASE_INCLUDES)

liblsa_ss_la_LIBADD = \
    liblsa.la

liblsa_srv_la_SOURCES =
liblsa_srv_la_CFLAGS = \
       $(AM_CFLAGS)
liblsa_srv_la_LIBADD = \
    liblsa_ss.la
liblsa_srv_la_LDFLAGS = -module -avoid-version

CLEAN_FILES = lsa_cstub.c lsa_sstub.c lsa_h.h

BUILT_SOURCES = lsa_h.h

lsa_sstub.c lsa_cstub.c lsa_h.h: $(idl_srcdir)/lsa.idl lsa.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header lsa_h.h -I$(srcdir) -I$(idl_srcdir) \
		$(DCEIDL_PASS_INCLUDES) $<
