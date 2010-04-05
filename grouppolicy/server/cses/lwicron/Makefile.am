gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwicron.la

liblwicron_la_SOURCES = cronext.c

liblwicron_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwicron_la_LIBADD = \
    @top_builddir@/server/gpcse/libgpcses.la \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwicron_la_LDFLAGS = \
    -module -export-dynamic

