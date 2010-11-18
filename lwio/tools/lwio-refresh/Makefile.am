bin_PROGRAMS = lwio-refresh

lwio_refresh_SOURCES = \
    main.c

lwio_refresh_CPPFLAGS =  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include

lwio_refresh_LDADD = \
    @top_builddir@/client/liblwioclient.la \
    @top_builddir@/common/liblwiocommon.la \
    -llwbase_nothr

lwio_refresh_LDFLAGS = -L$(top_builddir)/common/.libs
