lib_LTLIBRARIES = libgpauthsvc.la

libgpauthsvc_la_SOURCES = \
    authsvc.c \
    globals.c

libgpauthsvc_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

libgpauthsvc_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @LSASS_LIBS@    \
    @NETLOGON_LIBS@\
    @LWADVAPI_LIBS@

libgpauthsvc_la_LDFLAGS = \
    @LSASS_LDFLAGS@    \
    @NETLOGON_LDFLAGS@

