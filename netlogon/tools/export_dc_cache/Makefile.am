AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lw-export-dc-cache

lw_export_dc_cache_CPPFLAGS = \
	-I@top_srcdir@/server/include \
	-I@top_srcdir@/server/api \
	$(AM_CPPFLAGS)

lw_export_dc_cache_LDADD = \
	@top_builddir@/server/api/liblwnetserverapi.la \
	@top_builddir@/utils/liblwnetutils.la \
	-llwadvapi_nothr

lw_export_dc_cache_SOURCES = main.c
