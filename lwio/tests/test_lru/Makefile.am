AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_lru

test_lru_SOURCES = main.c
test_lru_LDFLAGS = -static
test_lru_LDADD = @top_builddir@/utils/liblwioutils.la @PTHREAD_LIBS@
