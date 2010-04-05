pkglibdir=@plugindir@
pkglib_LTLIBRARIES = console.la
MODULE_LDFLAGS = -shared -module -avoid-version -export-dynamic
AM_CPPFLAGS = -I$(top_srcdir)/include

console_la_SOURCES = console.c
console_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
console_la_LDFLAGS = $(MODULE_LDFLAGS)


