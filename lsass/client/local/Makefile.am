AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblsaclient_local.la

liblsaclient_local_la_SOURCES = \
    privileges.c

liblsaclient_local_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(LWMSG_INCLUDES)

liblsaclient_local_la_LIBADD = \
	$(top_builddir)/common/liblsacommon.la \
	$(LWMSG_NOTHR_LIBS) \
	$(SOCKET_LIBS)

