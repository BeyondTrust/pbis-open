nsslib_LTLIBRARIES = nss_lsass.la

nss_lsass_la_SOURCES = \
	nss-group.c \
	nss-main.c \
	nss-user.c \
        nss-netgrp.c

nss_lsass_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I@srcdir@/../include

nss_lsass_la_LIBADD = \
	@builddir@/../common/libnsscommon.la \
	@top_builddir@/client/liblsaclient.la

nss_lsass_la_LDFLAGS = -module -version-number 1 -export-dynamic

noinst_HEADERS = \
	nss-group.h \
	nss-user.h \
        nss-netgrp.h
