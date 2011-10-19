noinst_LTLIBRARIES=liblwps_logging.la

liblwps_logging_la_SOURCES = \
	globals.c \
	logging.c

liblwps_logging_la_LDFLAGS = \
	$(LWBASE_LDFLAGS)

liblwps_logging_la_LIBADD = \
	$(LWBASE_LIBS)

liblwps_logging_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/include

noinst_HEADERS = \
	includes.h \
	externs.h
