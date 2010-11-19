AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lw-get-dc-list

lw_get_dc_list_LDADD = \
	@top_builddir@/client/liblwnetclientapi.la \
	@top_builddir@/common/liblwnetcommon.la \
	-llwadvapi_nothr

lw_get_dc_list_SOURCES = main.c
