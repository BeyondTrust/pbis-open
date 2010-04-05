srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(rpc_srcdir)/idl

if ENABLE_STATIC_RPCSERVERS
noinst_LTLIBRARIES = libdssetup_srv.la
else
rpcserver_LTLIBRARIES = libdssetup_srv.la
noinst_LTLIBRARIES = libdssetup_ss.la libdssetup.la
endif

libdssetup_la_SOURCES =             \
    globals.c                       \
    dssetup.c                       \
    dssetup_cfg.c                   \
    dssetup_srv.c                   \
    dsr_memory.c                    \
    dsr_security.c                  \
    dsr_accesstoken.c               \
    dsr_rolegetprimarydomaininfo.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(NETAPI_INCLUDES) \
    $(LWBASE_INCLUDES)

libdssetup_la_CFLAGS = \
    $(AM_CFLAGS)            \
    -DLIBRPC_BUILD          \
    -I$(srv_srcdir)/include \
    -I$(top_srcdir)/include \
    -I$(srv_srcdir)/api     \
    $(DCERPC_INCLUDES)      \
    $(LIBUNISTR_INCLUDES)   \
    $(LWBASE_INCLUDES)      \
    $(NETLOGON_INCLUDES)

libdssetup_la_LIBADD = \
    $(DCERPC_LIBS)                        \
    $(NETAPI_LIBS)                        \
    $(LIBUNISTR_LIBS)                     \
    $(LWBASE_LIBS)                        \
    $(NETLOGON_LIBS)                      \
    $(top_builddir)/server/api/liblsaserverapi.la  \
    $(top_builddir)/server/store/dsapi/libdsapi.la

libdssetup_ss_la_SOURCES = \
    dssetup_sstub.c

libdssetup_ss_la_CFLAGS = \
    $(AM_CFLAGS)          \
    -DDCERPC_STUB_BUILD   \
    $(DCERPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES) \
    $(LWBASE_INCLUDES)

libdssetup_ss_la_LIBADD = \
    libdssetup.la

libdssetup_srv_la_SOURCES =
libdssetup_srv_la_CFLAGS = \
    $(AM_CFLAGS)
libdssetup_srv_la_LIBADD = \
    libdssetup_ss.la
libdssetup_srv_la_LDFLAGS = -module -avoid-version

CLEAN_FILES = dssetup_cstub.c dssetup_sstub.c dssetup_h.h

BUILT_SOURCES = dssetup_h.h

dssetup_sstub.c dssetup_cstub.c dssetup_h.h: $(idl_srcdir)/dssetup.idl dssetup.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header dssetup_h.h -I$(srcdir) -I$(idl_srcdir) \
		$(DCEIDL_PASS_INCLUDES) $<
