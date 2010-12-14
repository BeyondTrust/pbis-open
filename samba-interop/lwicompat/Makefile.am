lib_LTLIBRARIES = lwicompat_v4.la

wbclient_include_dir = $(top_srcdir)/wbclient
wbclient_libs = $(top_builddir)/wbclient/libwbclient.la

lwicompat_v4_la_SOURCES  = lwicompat_v4.c
lwicompat_v4_la_CPPFLAGS = -I$(srcdir) -I$(wbclient_include_dir)
lwicompat_v4_la_LIBADD   = $(wbclient_libs)
lwicompat_v4_la_LDFLAGS  = -module -export-dynamic
