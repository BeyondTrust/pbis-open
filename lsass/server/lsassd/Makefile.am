AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include \
        -DLW_ENABLE_THREADS=1         \
        -I@top_srcdir@/server/include

MAIN_LDFLAGS = \
	@NETLOGON_LDFLAGS@ \
	@LWIO_LDFLAGS@     \
	@DS_FRAMEWORK_LDFLAGS@

MAIN_LIBS = \
    $(top_builddir)/server/api/liblsaserverapi.la \
    $(top_builddir)/server/ntlm/libntlmserver.la \
    $(SOCKET_LIBS) \
    $(NETLOGON_LIBS) \
    $(LWIO_LIBS) \
    $(top_builddir)/client/ntlm/liblsaclient_ntlm.la

svcmdir = $(libdir)/lw-svcm

svcm_LTLIBRARIES = lsass.la

lsass_la_SOURCES = \
        globals.c    \
        listener.c   \
	main-svcm.c

lsass_la_LIBADD = $(MAIN_LIBS)

lsass_la_LDFLAGS = $(MAIN_LDFLAGS) -module -avoid-version

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lsasscachedir)
