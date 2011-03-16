
noinst_LTLIBRARIES = librsysserverapi.la

librsysserverapi_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I@top_srcdir@/server/include

librsysserverapi_la_SOURCES = \
       config.c          \
       globals.c

librsysserverapi_la_LIBADD = \
        @LWREG_LIBS@\
        @LWADVAPI_LIBS@\
	@PTHREAD_LIBS@

noinst_HEADERS = \
	config_p.h     \
	externs_p.h    \
	includes.h     \
	structs_p.h
