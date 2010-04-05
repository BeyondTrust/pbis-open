bin_PROGRAMS = lwio-cli

lwio_cli_SOURCES  = \
    main.c \
    stats.c

lwio_cli_CPPFLAGS = $(AM_CPPFLAGS) -I@top_srcdir@/include

lwio_cli_LDADD    = $(top_builddir)/client/liblwioclient.la

lwio_cli_LDFLAGS  = -L$(top_builddir)/common/.libs
