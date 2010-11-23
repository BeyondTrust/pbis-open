lib_LTLIBRARIES = lwicompat_v2.la lwicompat_v3.la lwicompat_v4.la

wbclient_include_dir = $(top_srcdir)/wbclient
wbclient_libs = $(top_builddir)/wbclient/libwbclient.la

lwicompat_v2_la_SOURCES  = lwicompat_v2.c
lwicompat_v2_la_CPPFLAGS = -I$(srcdir) -I$(wbclient_include_dir)
lwicompat_v2_la_LIBADD   = $(wbclient_libs)
lwicompat_v2_la_LDFLAGS  = -module -export-dynamic

lwicompat_v3_la_SOURCES  = lwicompat_v3.c
lwicompat_v3_la_CPPFLAGS = -I$(srcdir) -I$(wbclient_include_dir)
lwicompat_v3_la_LIBADD   = $(wbclient_libs)
lwicompat_v3_la_LDFLAGS  = -module -export-dynamic

lwicompat_v4_la_SOURCES  = lwicompat_v4.c
lwicompat_v4_la_CPPFLAGS = -I$(srcdir) -I$(wbclient_include_dir)
lwicompat_v4_la_LIBADD   = $(wbclient_libs)
lwicompat_v4_la_LDFLAGS  = -module -export-dynamic
