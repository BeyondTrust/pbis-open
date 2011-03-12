AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblsaclient_api.la liblsaclient_api_thr.la

liblsaclient_api_la_SOURCES = \
    artefacts.c \
    auth.c      \
    clientipc.c \
    config.c    \
    groups.c    \
    metrics.c   \
    provider.c  \
    session.c   \
    status.c    \
    tracing.c   \
    users.c     \
    marshal.c   \
    api2.c

liblsaclient_api_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	@LWMSG_INCLUDES@

liblsaclient_api_la_LIBADD = \
	@top_builddir@/common/liblsacommon.la \
	@LWMSG_NOTHR_LIBS@ \
	@SOCKET_LIBS@

liblsaclient_api_thr_la_SOURCES = \
	clientipc_thr.c

liblsaclient_api_thr_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	@LWMSG_INCLUDES@

liblsaclient_api_thr_la_LIBADD = \
	-llwmsg -llwbase
