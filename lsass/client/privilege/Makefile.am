AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblsaclient_privilege.la

liblsaclient_privilege_la_SOURCES = \
    account.c

liblsaclient_privilege_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    $(LWMSG_INCLUDES)

liblsaclient_privilege_la_LIBADD = \
    $(top_builddir)/common/liblsacommon.la \
    $(LWMSG_NOTHR_LIBS) \
    $(SOCKET_LIBS)
