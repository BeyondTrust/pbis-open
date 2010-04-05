lib_LTLIBRARIES = libgpcses.la

libgpcses_la_SOURCES = \
    gpacsesutil.c \
    gpalwixml.c   \
    gpalwiocopy.c   \
    gpacseevents.c \
    gpaxfer.c

libgpcses_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/utils \
    @LWIO_INCLUDES@\
    @LWADVAPI_INCLUDES@

libgpcses_la_LIBADD = \
    @top_builddir@/utils/libgputils.la \
    @LWIO_LIBS@ \
    @EVENTLOG_LIBS@\
    @LIBXML2_LIBS@\
    @LWADVAPI_LIBS@

libgpcses_la_LDFLAGS = \
    @LIBXML2_LDFLAGS@

