gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwiprompt.la

liblwiprompt_la_SOURCES = lwiprompt.c

liblwiprompt_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils/\
    @LWADVAPI_INCLUDES@

liblwiprompt_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwiprompt_la_LDFLAGS = \
    -module -export-dynamic

