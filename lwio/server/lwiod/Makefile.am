MAIN_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

MAIN_LIBS = \
    @top_builddir@/server/iomgr/libiomgr.la \
    @NETLOGON_LIBS@ \
    @LWMSG_LIBS@ \
    @LWREG_LIBS@

svcmdir = $(libdir)/lw-svcm

svcm_LTLIBRARIES = lwio.la

lwio_la_LDFLAGS = -L$(top_builddir)/common/.libs -module -avoid-version

lwio_la_SOURCES = \
    globals.c   \
    main-svcm.c   \
    cfg.c    \
    ipc.c

lwio_la_CPPFLAGS = $(MAIN_CPPFLAGS)

lwio_la_LIBADD = $(MAIN_LIBS)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lwiocachedir)

