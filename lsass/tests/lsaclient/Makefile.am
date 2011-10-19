AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = lsaclient

lsaclient_SOURCES = main.c
lsaclient_LDFLAGS = -static
lsaclient_LDADD = @top_builddir@/client/liblsaclient.la