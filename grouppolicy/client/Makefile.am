lib_LTLIBRARIES = libgpclient.la

libgpclient_la_SOURCES = gpaclient.c

libgpclient_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/utils   \
    @LWADVAPI_INCLUDES@

libgpclient_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @LWADVAPI_LIBS@
