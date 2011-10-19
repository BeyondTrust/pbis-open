AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include \
        -DLW_ENABLE_THREADS=1         \
        -I@top_srcdir@/server/include

ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
        $(top_builddir)/logging_r/liblsalog_r.la \
        @top_builddir@/server/store/dsapi/libdsapi.la

if ENABLE_SMALL_DAEMON
lib_LTLIBRARIES = liblsamain.la
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
else
noinst_LTLIBRARIES = liblsamain.la
endif

MAIN_LDFLAGS = \
	@NETLOGON_LDFLAGS@ \
	@LWIO_LDFLAGS@     \
	@DS_FRAMEWORK_LDFLAGS@

MAIN_LIBS = \
    $(top_builddir)/server/api/liblsaserverapi.la \
    $(top_builddir)/server/ntlm/libntlmserver.la \
	$(ADDITIONAL_LIBS) \
    $(SOCKET_LIBS) \
    $(NETLOGON_LIBS) \
    $(LWIO_LIBS) \
    $(top_builddir)/client/ntlm/liblsaclient_ntlm.la

if ENABLE_STATIC_PROVIDERS
if ENABLE_AD
MAIN_LIBS += $(top_builddir)/server/auth-providers/ad-provider/liblsass_auth_provider_ad.la
endif
if ENABLE_LOCAL
MAIN_LIBS += $(top_builddir)/server/auth-providers/local-provider/liblsass_auth_provider_local.la
endif
endif

if ENABLE_STATIC_RPCSERVERS
if ENABLE_RPCSERVER
MAIN_LIBS += $(top_builddir)/server/rpc/lsa/liblsa_srv.la
MAIN_LIBS += $(top_builddir)/server/rpc/samr/libsamr_srv.la
MAIN_LIBS += $(top_builddir)/server/rpc/dssetup/libdssetup_srv.la
endif
endif

liblsamain_la_SOURCES = \
        globals.c    \
        listener.c   \
        libmain.c    \
        backtrace.c  \
        sighandler.c

liblsamain_la_LIBADD = $(MAIN_LIBS)
liblsamain_la_LDFLAGS = $(MAIN_LDFLAGS)

sbin_PROGRAMS = lsassd

lsassd_SOURCES = main.c

lsassd_LDADD = liblsamain.la

if !ENABLE_SMALL_DAEMON
lsassd_LDADD += $(MAIN_LIBS)
lsassd_LDFLAGS = $(MAIN_LDFLAGS)
endif


install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lsasscachedir)
