AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(srcdir)/../include

lib_LTLIBRARIES = liblsaauth.la

liblsaauth_la_SOURCES = \
    globals.c    \
    libmain.c    \
    pam-config.c \
    pam-logging.c

liblsaauth_la_LIBADD = @top_builddir@/client/liblsaclient.la

