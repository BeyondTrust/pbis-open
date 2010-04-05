gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwiapparmor.la

liblwiapparmor_la_SOURCES = lwiapparmor.c

liblwiapparmor_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwiapparmor_la_LIBADD = \
    @top_builddir@/server/gpcse/libgpcses.la \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwiapparmor_la_LDFLAGS = \
    -module -export-dynamic

