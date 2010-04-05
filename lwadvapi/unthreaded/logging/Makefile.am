noinst_LTLIBRARIES=liblwadvapi_logging.la

liblwadvapi_logging_la_SOURCES = \
	globals.c \
	logging.c

liblwadvapi_logging_la_LDFLAGS = \
	$(LWBASE_LDFLAGS)

liblwadvapi_logging_la_LIBADD = \
	$(LWBASE_LIBS)

liblwadvapi_logging_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/include/public \
	-I$(top_srcdir)/include

noinst_HEADERS = \
	includes.h \
	externs.h
