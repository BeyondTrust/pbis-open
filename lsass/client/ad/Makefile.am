AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblsaclient_ad.la

liblsaclient_ad_la_SOURCES = \
    cache.c     \
    groups.c    \
    users.c     \
    join.c

liblsaclient_ad_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	@LWMSG_INCLUDES@

liblsaclient_ad_la_LIBADD = \
	@top_builddir@/common/liblsacommon.la \
	@LWMSG_NOTHR_LIBS@ \
	@SOCKET_LIBS@
