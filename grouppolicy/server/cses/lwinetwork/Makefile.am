gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwinetwork.la

liblwinetwork_la_SOURCES = lwinetwork.c

liblwinetwork_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwinetwork_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwinetwork_la_LDFLAGS = \
    -module -export-dynamic

