AM_CPPFLAGS = -I$(top_srcdir)/include

svcmdir = $(libdir)/lw-svcm

svcm_LTLIBRARIES = netlogon.la

netlogon_la_CPPFLAGS = \
	-I@top_srcdir@/server/include \
	$(AM_CPPFLAGS)

netlogon_la_LIBADD = \
	@top_builddir@/server/ipc/liblwnetserveripc.la \
	@top_builddir@/server/api/liblwnetserverapi.la \
	@top_builddir@/common/liblwnetcommon.la        \
	@RT_LIBS@                                      \
	@DS_FRAMEWORK_LDFLAGS@

netlogon_la_LDFLAGS = -module -avoid-version

netlogon_la_SOURCES = \
	globals.c   \
	listener.c  \
	main-svcm.c
