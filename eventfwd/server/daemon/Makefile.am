sbin_PROGRAMS = eventfwdd

eventfwdd_CPPFLAGS = \
        $(AM_CPPFLAGS) \
	-I@top_srcdir@/server/include

eventfwdd_LDADD = \
	@top_builddir@/server/api/libevtfwdserverapi.la \
	@top_builddir@/server/poller/libevtfwdserverpoller.la \
	@top_builddir@/logging/libevtfwdlogging.la \
	@top_builddir@/logging/logging_r/libevtfwdlogging_r.la \
	@top_builddir@/client/libevtfwdclientapi.la \
	-llwadvapi \
	@RT_LIBS@

eventfwdd_SOURCES = \
	globals.c    \
	listener.c   \
	main.c       \
	sighandler.c
