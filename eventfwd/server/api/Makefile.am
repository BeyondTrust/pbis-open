
noinst_LTLIBRARIES = libevtfwdserverapi.la

libevtfwdserverapi_la_CPPFLAGS = \
        $(AM_CPPFLAGS)                \
	-I@top_srcdir@/server/include \
	@SQLITE_INCLUDES@             \
	@PSTORE_INCLUDES@             \
	@OPENLDAP_INCLUDES@

libevtfwdserverapi_la_SOURCES = \
       config.c          \
       globals.c         \
       state.c           \
       loginfo.c

libevtfwdserverapi_la_LIBADD = \
		@top_builddir@/logging/libevtfwdlogging.la \
		@PTHREAD_LIBS@        \
		@top_builddir@/server/ipc/libevtfwdserveripc.la

libevtfwdserverapi_la_LDFLAGS = \
        @SQLITE_LDFLAGS@       \
        @PSTORE_LDFLAGS@       \
        @OPENLDAP_LDFLAGS@

noinst_HEADERS = \
	config_p.h     \
	externs_p.h    \
	includes.h     \
	loginfo_p.h    \
	state_p.h      \
	structs_p.h
