AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lwnet-get-log-info

lwnet_get_log_info_LDADD = \
    @top_builddir@/client/liblwnetclientapi.la \
    @top_builddir@/common/liblwnetcommon.la \
    -llwadvapi_nothr

lwnet_get_log_info_SOURCES = \
    main.c
