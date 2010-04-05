AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(srcdir)/../include

noinst_LTLIBRARIES = libnsscommon.la

libnsscommon_la_SOURCES = \
	nss-error.c \
	nss-user.c \
	nss-group.c \
	nss-netgrp.c

libnsscommon_la_LIBADD = \
	@top_builddir@/client/liblsaclient.la
