AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = test_getpwnam

test_getpwnam_SOURCES = main.c
test_getpwnam_LDFLAGS = -static
test_getpwnam_LDADD = @top_builddir@/common/liblsacommon.la