pkglibdir=@plugindir@
pkglib_LTLIBRARIES = xml.la
AM_CPPFLAGS = -I$(top_srcdir)/include
MODULE_LDFLAGS = -shared -module -avoid-version -export-dynamic

xml_la_SOURCES = xml.c
xml_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
xml_la_LDFLAGS = $(MODULE_LDFLAGS)
