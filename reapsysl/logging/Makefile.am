lib_LTLIBRARIES=librsyslogging.la

librsyslogging_la_SOURCES = \
	file.c \
	globals.c \
	logging.c \
	syslog.c

librsyslogging_la_LDFLAGS = \
	$(LWBASE_LDFLAGS)

librsyslogging_la_LIBADD = \
	$(LWBASE_LIBS)

noinst_HEADERS = \
	includes.h \
	externs.h \
	file_p.h \
	syslog_p.h

SUBDIRS=logging_r
