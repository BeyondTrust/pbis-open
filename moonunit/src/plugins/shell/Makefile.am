pkglibdir=@plugindir@
pkglib_LTLIBRARIES = sh.la
libexec_SCRIPTS = mu.sh

AM_CPPFLAGS = -I$(top_srcdir)/include -DLIBEXEC_PATH="\"$(libexecdir)\""
MODULE_LDFLAGS = -shared -module -avoid-version -export-dynamic

sh_la_SOURCES = sh-exec.c sh-load.c process.c
sh_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
sh_la_LDFLAGS = $(MODULE_LDFLAGS)

noinst_HEADERS = process.h sh-exec.h sh-types.h

