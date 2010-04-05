AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = test_checkuserinlist

test_checkuserinlist_SOURCES = main.c
test_checkuserinlist_LDFLAGS = -static
test_checkuserinlist_LDADD = @top_builddir@/client/liblsaclient.la