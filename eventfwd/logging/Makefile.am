lib_LTLIBRARIES=libevtfwdlogging.la

libevtfwdlogging_la_SOURCES = \
	file.c \
	globals.c \
	logging.c \
	stubs.c \
	syslog.c

libevtfwdlogging_la_LDFLAGS = \
	$(LWBASE_LDFLAGS)

libevtfwdlogging_la_LIBADD = \
	$(LWBASE_LIBS)

noinst_HEADERS = \
	includes.h \
	externs.h \
	file_p.h \
	syslog_p.h

SUBDIRS=logging_r
