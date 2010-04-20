sbin_PROGRAMS = lwregd

lwregd_SOURCES =    \
    globals.c       \
    listener.c      \
    main.c          \
    sighandler.c

lwregd_CPPFLAGS =                 \
    -DLW_ENABLE_THREADS=1         \
    $(AM_CPPFLAGS)                \
    -I@top_srcdir@/server/include \
    @LWBASE_INCLUDES@

lwregd_LDADD =                             \
    @top_builddir@/server/api/libregserverapi.la \
    @top_builddir@/common/libregcommon.la     \
    @top_builddir@/logging_r/libreglog_r.la   \
    @DS_FRAMEWORK_LDFLAGS@

sysconf_DATA = lwreg.reg

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(registrycachedir)
