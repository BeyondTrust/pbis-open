lib_LTLIBRARIES=libcltrlogging.la

libcltrlogging_la_SOURCES = \
	file.c \
	globals.c \
	logging.c \
	stubs.c \
	syslog.c

 libcltrlogging_la_LDFLAGS = \
    	-static \
	$(LWBASE_LDFLAGS) \
    	$(AM_LDFLAGS)

libcltrlogging_la_LIBADD = \
	logging_r/libcltrlogging_r.la \
	$(LWBASE_LIBS)

noinst_HEADERS = \
	includes.h \
	defines.h \
	prototypes.h \
	structs.h \
	externs.h

SUBDIRS=logging_r
