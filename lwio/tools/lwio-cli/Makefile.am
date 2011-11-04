bin_PROGRAMS = lwio-cli

lwio_cli_SOURCES  = \
    main.c \
    stats.c

lwio_cli_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/include

lwio_cli_LDADD    = \
    $(top_builddir)/client/liblwioclient.la \
    $(top_builddir)/common/liblwiocommon.la \
    $(top_builddir)/libshareinfo/liblwioshareinfo.la \
    -llwbase_nothr

lwio_cli_LDFLAGS  = -L$(top_builddir)/common/.libs \
                    -L$(top_builddir)/libshareinfo/.libs
