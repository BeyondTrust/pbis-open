bin_PROGRAMS = lwio-driver

lwio_driver_SOURCES = main.c
lwio_driver_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/include
lwio_driver_LDADD = \
    $(top_builddir)/client/liblwioclient.la \
    $(top_builddir)/common/liblwiocommon.la \
    -llwbase_nothr

lwio_driver_LDFLAGS = -L$(top_builddir)/common/.libs
