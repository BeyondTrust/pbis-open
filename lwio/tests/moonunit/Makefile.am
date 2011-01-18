TEST_SOURCES = test-zct.c test-util.c

lib_LTLIBRARIES =

if HAVE_MOONUNIT
lib_LTLIBRARIES += lwio_mu.la
endif

lwio_mu_la_SOURCES = \
	test-stub.c \
	$(TEST_SOURCES)

lwio_mu_la_CPPFLAGS =               \
    $(AM_CPPFLAGS)                  \
    -I$(top_srcdir)/include         \
    -I$(top_builddir)/include       \
    -I$(top_srcdir)/server/include

lwio_mu_la_LIBADD = \
	$(top_builddir)/server/iomgr/libiomgr.la \
	$(LIB_MOONUNIT)

lwio_mu_la_LDFLAGS = -module -avoid-version

test-stub.c: $(TEST_SOURCES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(lwio_mu_la_CPPFLAGS)" $^
