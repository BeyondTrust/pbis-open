AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/server/include

noinst_LTLIBRARIES = liblsaserver_privilege.la

liblsaserver_privilege_la_SOURCES = \
    ipc_dispatch.c \
    globals.c \
    privilege.c \
    account.c \
    privilegedb.c

liblsaserver_privilege_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    $(LWMSG_INCLUDES)

liblsaserver_privilege_la_LIBADD = \
    $(top_builddir)/common/liblsacommon.la \
    $(LWMSG_NOTHR_LIBS) \
    $(SOCKET_LIBS)
