gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwifiles.la

liblwifiles_la_SOURCES = lwifiles.c

liblwifiles_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwifiles_la_LIBADD = \
    @top_builddir@/server/gpcse/libgpcses.la \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwifiles_la_LDFLAGS = \
    -module -export-dynamic

