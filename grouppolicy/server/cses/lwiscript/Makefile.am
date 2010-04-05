gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwiscript.la

liblwiscript_la_SOURCES = scriptext.c

liblwiscript_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils/\
    @LWADVAPI_INCLUDES@

liblwiscript_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @top_builddir@/server/gpcse/libgpcses.la\
    @LWADVAPI_LIBS@

liblwiscript_la_LDFLAGS = \
    -module -export-dynamic

