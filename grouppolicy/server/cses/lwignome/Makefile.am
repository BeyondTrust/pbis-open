gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwignome.la

liblwignome_la_SOURCES = \
    lwignome.c       \
    lwignomecommon.c

liblwignome_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwignome_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwignome_la_LDFLAGS = \
    -module -export-dynamic

