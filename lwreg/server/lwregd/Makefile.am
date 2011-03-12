svcmdir = $(libdir)/lw-svcm

svcm_LTLIBRARIES = lwreg.la

lwreg_la_SOURCES =  \
    globals.c       \
    listener.c      \
    main-svcm.c

lwreg_la_CPPFLAGS =               \
    -DLW_ENABLE_THREADS=1         \
    $(AM_CPPFLAGS)                \
    -I@top_srcdir@/server/include \
    @LWBASE_INCLUDES@

lwreg_la_LIBADD =                             \
    @top_builddir@/server/api/libregserverapi.la \
    @top_builddir@/common/libregcommon.la     \
    @top_builddir@/logging_r/libreglog_r.la   \
    @DS_FRAMEWORK_LDFLAGS@ \
    -llwmsg

lwreg_la_LDFLAGS = -module -avoid-version

lwregreg_DATA = lwreg.reg

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(registrycachedir)
