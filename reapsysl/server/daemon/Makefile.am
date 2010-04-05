sbin_PROGRAMS = reapsysld

reapsysld_CPPFLAGS = \
        $(AM_CPPFLAGS) \
	-I@top_srcdir@/server/include

reapsysld_LDADD = \
	@top_builddir@/server/api/librsysserverapi.la \
	@top_builddir@/server/reader/librsysserverreader.la \
	@top_builddir@/logging/librsyslogging.la \
	@top_builddir@/logging/logging_r/librsyslogging_r.la \
	@top_builddir@/client/librsysclientapi.la \
	-llwadvapi \
	@RT_LIBS@

reapsysld_SOURCES = \
	globals.c    \
	listener.c   \
	main.c       \
	sighandler.c
