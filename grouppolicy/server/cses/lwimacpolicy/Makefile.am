gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwimacpolicy.la

liblwimacpolicy_la_SOURCES = lwimacpolicy.c

liblwimacpolicy_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils/\
    @LWADVAPI_INCLUDES@

liblwimacpolicy_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwimacpolicy_la_LDFLAGS = \
    -module -export-dynamic

