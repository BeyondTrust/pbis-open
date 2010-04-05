gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwisudo.la

liblwisudo_la_SOURCES = \
    lwisudo.c

liblwisudo_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwisudo_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @top_builddir@/server/gpcse/libgpcses.la \
    @LWREG_LIBS@\
    @LWADVAPI_LIBS@

liblwisudo_la_LDFLAGS = \
    -module -export-dynamic

