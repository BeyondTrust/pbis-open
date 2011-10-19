AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

testlib_LTLIBRARIES = libperflsass.la

libperflsass_la_SOURCES = tests.c
libperflsass_la_LDFLAGS = -module
libperflsass_la_LIBADD = @top_builddir@/client/liblsaclient.la
