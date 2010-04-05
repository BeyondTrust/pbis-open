gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwifstab.la

liblwifstab_la_SOURCES = lwifstab.c

liblwifstab_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwifstab_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwifstab_la_LDFLAGS = \
    -module -export-dynamic

