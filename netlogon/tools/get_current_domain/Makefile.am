AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lw-get-current-domain

lw_get_current_domain_LDADD = \
	@top_builddir@/client/liblwnetclientapi.la \
	@top_builddir@/common/liblwnetcommon.la \
	-llwadvapi_nothr

lw_get_current_domain_SOURCES = main.c
