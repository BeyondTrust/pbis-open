AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lwnet-set-log-level

lwnet_set_log_level_LDADD = \
    @top_builddir@/client/liblwnetclientapi.la \
    @top_builddir@/common/liblwnetcommon.la \
    -llwadvapi

lwnet_set_log_level_SOURCES = \
    main.c
