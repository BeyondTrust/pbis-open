gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwimotd.la

liblwimotd_la_SOURCES = lwimotd.c

liblwimotd_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwimotd_la_LIBADD = \
    @top_builddir@/server/gpcse/libgpcses.la \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwimotd_la_LDFLAGS = \
    -module -export-dynamic

