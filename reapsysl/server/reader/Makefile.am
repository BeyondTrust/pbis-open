
noinst_LTLIBRARIES = librsysserverreader.la

librsysserverreader_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(EVENTLOG_INCLUDES) \
	$(LSASS_INCLUDES) \
	-I@top_srcdir@/server/include

librsysserverreader_la_LIBADD = \
	$(EVENTLOG_LIBS) \
	$(LSASS_LIBS) \
	@top_builddir@/utils/librsysutils.la

librsysserverreader_la_SOURCES = \
	reader.c    \
	globals.c

librsysserverreader_la_LDFLAGS = \
	-static \
	$(EVENTLOG_LDFLAGS) \
	$(LSASS_LDFLAGS) \
	$(AM_LDFLAGS)

noinst_HEADERS = \
    includes.h        \
    externs_p.h       \
    reader_p.h

