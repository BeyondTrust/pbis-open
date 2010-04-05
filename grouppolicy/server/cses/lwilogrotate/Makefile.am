gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwilogrotate.la

liblwilogrotate_la_SOURCES = lwilogrotate.c

liblwilogrotate_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwilogrotate_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwilogrotate_la_LDFLAGS = \
    -module -export-dynamic

