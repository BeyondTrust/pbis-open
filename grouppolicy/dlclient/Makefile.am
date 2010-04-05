lib_LTLIBRARIES = libgpapi.la

libgpapi_la_SOURCES = gppamsupp.c

libgpapi_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/utils

libgpapi_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @top_builddir@/client/libgpclient.la\
    @LWADVAPI_LIBS@

libgpapi_la_LDFLAGS = \
    -module -avoid-version -export-dynamic
