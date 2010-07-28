nsslib_LTLIBRARIES = nss_lsass.la

nss_lsass_la_SOURCES = \
	nss-group.c \
	nss-main.c \
	nss-user.c \
	globals.c

nss_lsass_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/../../auth/include

nss_lsass_la_LIBADD = \
	../common/libnsscommon.la \
	$(top_builddir)/client/liblsaclient.la

nss_lsass_la_LDFLAGS = -module -version-number 1 -export-dynamic
