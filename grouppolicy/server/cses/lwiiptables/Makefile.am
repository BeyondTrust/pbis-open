gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwiiptables.la

liblwiiptables_la_SOURCES = lwiiptables.c

liblwiiptables_la_CPPFLAGS = \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

liblwiiptables_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @top_builddir@/server/gpcse/libgpcses.la\
    @LWADVAPI_LIBS@

liblwiiptables_la_LDFLAGS = \
    -module -export-dynamic

