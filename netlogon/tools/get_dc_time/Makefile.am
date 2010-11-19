AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lw-get-dc-time

lw_get_dc_time_LDADD = \
	@top_builddir@/client/liblwnetclientapi.la \
	@top_builddir@/common/liblwnetcommon.la \
	-llwadvapi_nothr

lw_get_dc_time_SOURCES = main.c
