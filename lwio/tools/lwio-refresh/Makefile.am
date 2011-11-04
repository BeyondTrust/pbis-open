bin_PROGRAMS = lwio-refresh

lwio_refresh_SOURCES = \
    main.c

lwio_refresh_CPPFLAGS =  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include

lwio_refresh_LDADD = \
    @top_builddir@/client/liblwioclient.la \
    @top_builddir@/common/liblwiocommon.la \
    $(top_builddir)/libshareinfo/liblwioshareinfo.la \
    -llwbase_nothr

lwio_refresh_LDFLAGS = -L$(top_builddir)/common/.libs \
                       -L$(top_builddir)/libshareinfo/.libs
