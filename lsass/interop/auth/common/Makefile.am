AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(srcdir)/../include

if PLATFORM_AIX
lib_LTLIBRARIES = liblsaauth.la
else
noinst_LTLIBRARIES = liblsaauth.la
endif

liblsaauth_la_SOURCES = \
    globals.c    \
    libmain.c    \
    pam-config.c \
    pam-logging.c

liblsaauth_la_LIBADD = @top_builddir@/client/liblsaclient.la

