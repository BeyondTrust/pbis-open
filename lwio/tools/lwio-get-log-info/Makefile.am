bin_PROGRAMS = lwio-get-log-info

lwio_get_log_info_SOURCES = \
    main.c

lwio_get_log_info_CPPFLAGS =  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include

lwio_get_log_info_LDADD = \
    @top_builddir@/client/liblwioclient.la \
    @top_builddir@/common/liblwiocommon.la \
    -llwbase_nothr

lwio_get_log_info_LDFLAGS = -L$(top_builddir)/common/.libs

