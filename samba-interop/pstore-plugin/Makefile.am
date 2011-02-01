AM_CPPFLAGS += -I$(top_srcdir)/include -I$(top_srcdir)/libtdb/include

lib_LTLIBRARIES = libsamba-pstore-plugin.la

libsamba_pstore_plugin_la_SOURCES = \
	samba-pstore-plugin.c

libsamba_pstore_plugin_la_LIBADD = $(top_builddir)/libtdb/libtdb.la -llwadvapi -lrsutils -llwbase_nothr

noinst_HEADERS = samba-pstore-plugin.h
