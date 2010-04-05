AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = test_getpwent

test_getpwent_SOURCES = main.c
test_getpwent_LDFLAGS = -static
test_getpwent_LDADD = @top_builddir@/common/liblsacommon.la