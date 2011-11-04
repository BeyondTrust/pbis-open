AM_CPPFLAGS = -I$(top_srcdir)/include

sbin_PROGRAMS = netlogond

netlogond_CPPFLAGS = \
	-I@top_srcdir@/server/include \
	$(AM_CPPFLAGS)

netlogond_LDADD = \
	@top_builddir@/server/ipc/liblwnetserveripc.la \
	@top_builddir@/server/api/liblwnetserverapi.la \
	@top_builddir@/common/liblwnetcommon.la        \
	@RT_LIBS@                                      \
	@DS_FRAMEWORK_LDFLAGS@

netlogond_SOURCES = \
	globals.c    \
	listener.c   \
	main.c       \
	sighandler.c
