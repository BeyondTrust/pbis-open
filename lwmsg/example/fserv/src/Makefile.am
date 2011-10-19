lib_LTLIBRARIES = libfservcommon.la libfservclient.la
sbin_PROGRAMS = fserver
bin_PROGRAMS = fcat fsink

AM_CPPFLAGS = @FSERV_CPPFLAGS@
AM_CFLAGS = @FSERV_CFLAGS@ -Wall -Werror
AM_LDFLAGS = @FSERV_LDFLAGS@ -static

libfservcommon_la_SOURCES = protocol.c
libfservcommon_la_CPPFLAGS = $(AM_CPPFLAGS) $(LWMSG_CPPFLAGS)
libfservcommon_la_LIBADD = $(LIB_LWMSG)

libfservclient_la_SOURCES = fserv-client.c
libfservclient_la_CPPFLAGS = $(AM_CPPFLAGS) $(LWMSG_CPPFLAGS)
libfservclient_la_LIBADD = libfservcommon.la

fserver_SOURCES = fserver.c fserver-dispatch.c
fserver_LDADD = libfservcommon.la

fcat_SOURCES = fcat.c
fcat_LDADD = libfservclient.la

fsink_SOURCES = fsink.c
fsink_LDADD = libfservclient.la