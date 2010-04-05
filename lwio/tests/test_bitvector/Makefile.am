AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_bitvector

test_bitvector_SOURCES = main.c
test_bitvector_LDFLAGS = -static
test_bitvector_LDADD = @top_builddir@/utils/liblwioutils.la @PTHREAD_LIBS@
