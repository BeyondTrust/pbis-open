AM_CPPFLAGS = -I$(top_srcdir)/include

sbin_PROGRAMS = nfssvcd

lib_LTLIBRARIES = libnfssvc_ss.la

idl_srcdir = $(top_srcdir)/idl
include_srcdir = $(top_srcdir)/include

nfssvcd_SOURCES =              \
    config.c                   \
    nfssvc.c                   \
    server.c                   \
    globals.c                  \
    marshall.c                 \
    memutil.c                  \
    signalhandler.c            \
    nfssvc_netshareadd.c       \
    nfssvc_netsharedel.c       \
    nfssvc_netsharegetinfo.c   \
    nfssvc_netsharesetinfo.c   \
    nfssvc_netshareenum.c      \
    nfssvc_netservergetinfo.c  \
    nfssvc_netnamevalidate.c   \
    utils.c                    \
    main.c


libnfssvc_ss_la_SOURCES = nfssvc_sstub.c

libnfssvc_ss_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -DDCERPC_STUB_BUILD    \
    $(DCERPC_INCLUDES)

libnfssvc_ss_la_CFLAGS = \
    $(AM_CPPFLAGS)       \
    -DDCERPC_STUB_BUILD  \
    $(DCERPC_INCLUDES)

nfssvcd_CFLAGS = \
    -DNFSSVC_BUILD   \
    -DLIBRPC_BUILD

nfssvcd_CPPFLAGS = \
    $(AM_CPPFLAGS)        \
    $(DCERPC_INCLUDES)    \
    $(LIBRPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES) \
    $(LWADVAPI_INCLUDES)  \
    $(LWIO_INCLUDES)      \
    $(NETLOGON_INCLUDES)

nfssvcd_LDFLAGS = \
    $(DCERPC_LDFLAGS)    \
    $(LIBRPC_LDFLAGS)    \
    $(LIBUNISTR_LDFLAGS) \
    $(LWADVAPI_LDFLAGS)  \
    $(LWIO_LDFLAGS)      \
    $(NETLOGON_LDFLAGS)  \
    @DS_FRAMEWORK_LDFLAGS@

nfssvcd_LDADD = \
    $(top_builddir)/logging_r/libnfssvclog_r.la \
    $(top_builddir)/utils/libnfssvcutils.la \
    libnfssvc_ss.la                         \
    libwkssvc_ss.la                         \
    libwinreg_ss.la                         \
    $(DCERPC_LIBS)                          \
    $(LIBRPC_LIBS)                          \
    $(LIBUNISTR_LIBS)                       \
    $(LWADVAPI_LIBS)                        \
    $(LWIO_LIBS)                            \
    $(NETLOGON_LIBS)

CLEANFILES = nfssvc_h.h nfssvc_cstub.c nfssvc_sstub.c

BUILT_SOURCES = \
    nfssvc_h.h

nfssvc_h.h nfssvc_cstub.c nfssvc_sstub.c: $(idl_srcdir)/nfssvc.idl $(idl_srcdir)/nfssvc.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header nfssvc_h.h \
		-I$(srcdir) -I$(idl_srcdir) -I$(include_srcdir) $<

