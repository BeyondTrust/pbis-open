nsslib_LTLIBRARIES = libnss_lsass.la

libnss_lsass_la_SOURCES = \
	nss-auth.c \
	nss-group.c \
	nss-main.c \
	nss-user.c \
	globals.c

libnss_lsass_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/../include \
	-I$(srcdir)/../../auth/include

libnss_lsass_la_LIBADD = \
	../common/libnsscommon.la \
	$(top_builddir)/client/liblsaclient.la

libnss_lsass_la_LDFLAGS = -module -version-number 2 -export-dynamic

noinst_HEADERS = \
	nss-auth.h \
	nss-group.h \
	nss-user.h
