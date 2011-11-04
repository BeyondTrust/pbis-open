AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = test_memcache

test_memcache_SOURCES = main.c
test_memcache_LDFLAGS = -static
test_memcache_LDADD = @top_builddir@/common/liblsacommon.la
