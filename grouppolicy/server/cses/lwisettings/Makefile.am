gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwisettings.la

liblwisettings_la_SOURCES = \
    lwiauthdxml.c           \
    regutils.c              \
    lwievtsettings.c        \
    lwievtfwd.c             \
    lwigpsettings.c         \
    lwilsassmode.c          \
    lwisettings.c           \
    lwiwinbindmode.c        \
    lwedsplugin.c

liblwisettings_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/utils\
	@LWIO_INCLUDES@\
    @LWADVAPI_INCLUDES@

liblwisettings_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
	@LWIO_LIBS@\
	@LWREG_LIBS@\
    @LWADVAPI_LIBS@

liblwisettings_la_LDFLAGS = \
    -module -export-dynamic

