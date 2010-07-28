AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(srcdir)/../include -I$(srcdir)/../../auth/include

noinst_LTLIBRARIES = libnsscommon.la

libnsscommon_la_SOURCES = \
	nss-error.c \
	nss-handle.c \
	nss-user.c \
	nss-group.c \
	nss-netgrp.c

libnsscommon_la_LIBADD = \
	$(top_builddir)/client/liblsaclient.la \
	$(top_builddir)/interop/auth/common/liblsaauth.la
