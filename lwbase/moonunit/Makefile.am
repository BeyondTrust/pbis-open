AM_CFLAGS = $(LWBASE_CFLAGS)
AM_CPPFLAGS = $(LWBASE_CPPFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(LWBASE_LDFLAGS)

TEST_SOURCES = test-security.c test-atomic.c test-threadpool.c test-compare.c test-convert.c test-printf.c

noinst_LTLIBRARIES = libbenchmark.la

libbenchmark_la_SOURCES = benchmark.c
libbenchmark_la_LIBADD = $(top_builddir)/src/liblwbase.la @LIB_XNET@

noinst_PROGRAMS = benchmark
benchmark_SOURCES = benchmark-main.c
benchmark_LDADD = libbenchmark.la


lib_LTLIBRARIES =

if HAVE_MOONUNIT
lib_LTLIBRARIES += lwbase_mu.la
endif

lwbase_mu_la_SOURCES = \
	test-stub.c \
	$(TEST_SOURCES)

lwbase_mu_la_LIBADD = \
	libbenchmark.la \
	$(top_builddir)/src/liblwbase.la \
	$(LIB_MOONUNIT)

lwbase_mu_la_LDFLAGS = -module -avoid-version

test-stub.c: $(TEST_SOURCES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" $^
