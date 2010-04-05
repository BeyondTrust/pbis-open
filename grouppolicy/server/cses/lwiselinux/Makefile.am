gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwiselinux.la

liblwiselinux_la_SOURCES = lwiselinux.c

liblwiselinux_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils/\
    @LWADVAPI_INCLUDES@

liblwiselinux_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_INCLUDES@

liblwiselinux_la_LDFLAGS = \
    -module -export-dynamic

