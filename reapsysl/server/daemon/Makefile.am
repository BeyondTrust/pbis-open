svcmdir = $(libdir)/lw-svcm

svcm_LTLIBRARIES = reapsysl.la

reapsysl_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
	-I$(top_srcdir)/server/include

reapsysl_la_LIBADD = \
	$(top_builddir)/server/api/librsysserverapi.la \
	$(top_builddir)/server/reader/librsysserverreader.la \
	-llwadvapi \
	$(RT_LIBS)

reapsysl_la_LDFLAGS = -module -avoid-version

reapsysl_la_SOURCES = \
	globals.c    \
	main-svcm.c  \
	sighandler.c
