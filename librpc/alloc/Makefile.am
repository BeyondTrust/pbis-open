noinst_LTLIBRARIES = liballoc.la

liballoc_la_SOURCES = Allocate.c
liballoc_la_CPPFLAGS = -I$(top_srcdir)/include
liballoc_la_CFLAGS = $(AM_CFLAGS)

if HAVE_MOONUNIT

lib_LTLIBRARIES = alloc_mu.la

alloc_mu_la_SOURCES = test.c stub.c
alloc_mu_la_CPPFLAGS = -I$(top_srcdir)/include
alloc_mu_la_CFLAGS = -shared -Wall -Werror
alloc_mu_la_LIBADD = @LIB_moonunit@ liballoc.la
alloc_mu_la_LDFLAGS = -shared -module -export-dynamic -avoid-version

stub.c: test.c
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS) $(alloc_mu_la_CPPFLAGS)" $^

BUILT_SOURCES = stub.c

endif
