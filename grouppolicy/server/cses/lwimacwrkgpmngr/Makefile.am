gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwimacwrkgpmngr.la

liblwimacwrkgpmngr_la_SOURCES = lwimacwrkgpmngr.c

liblwimacwrkgpmngr_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwimacwrkgpmngr_la_LIBADD = \
    @top_builddir@/server/gpcse/libgpcses.la \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwimacwrkgpmngr_la_LDFLAGS = \
    -module -export-dynamic

