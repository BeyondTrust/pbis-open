bin_PROGRAMS = lwio-set-log-level

lwio_set_log_level_SOURCES = \
    main.c

lwio_set_log_level_CPPFLAGS =  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include

lwio_set_log_level_LDADD = \
    @top_builddir@/client/liblwioclient.la \
    @top_builddir@/common/liblwiocommon.la \
    -llwbase_nothr

lwio_set_log_level_LDFLAGS = -L$(top_builddir)/common/.libs
