noinst_LTLIBRARIES = libregserverapi.la

libregserverapi_la_SOURCES = \
    main.c                \
    globals.c             \
    ipc_registry.c        \
    regserver.c           \
    regsecurity.c                 

libregserverapi_la_CPPFLAGS =        \
	-DLW_ENABLE_THREADS=1         \
	$(AM_CPPFLAGS)                \
	-I@top_srcdir@/server/include \
	@LWBASE_INCLUDES@ \
	@REG_BACKEND_DEFINES@

libregserverapi_la_LDFLAGS = @LWBASE_LDFLAGS@

if USE_SQLITE_BACKEND

BACKEND_LIB = \
    @top_builddir@/server/providers/sqlite/libregserver_provider_sqlite.la

endif

if USE_FILE_BACKEND

BACKEND_LIB = \
    @top_builddir@/server/providers/file/libregserver_provider_file.la

endif

libregserverapi_la_LIBADD = \
    $(BACKEND_LIB) \
    -llwbase \
    -llwmsg_nothr \
    -luuid \
    @PTHREAD_LIBS@
