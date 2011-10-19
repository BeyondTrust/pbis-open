lib_LTLIBRARIES = centutils_mu.la

TEST_FILES = test_ctshell.c test_ctfileutils.c

AM_CPPFLAGS += -I@top_srcdir@/include

centutils_mu_la_SOURCES = $(TEST_FILES) test_stub.c
centutils_mu_la_LIBADD = @top_builddir@/libcentutils/libcentutils.la $(LIB_moonunit)
centutils_mu_la_LDFLAGS = -module -avoid-version -export-dynamic

BUILT_SOURCES = test_stub.c

test_stub.c: $(TEST_FILES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS) " $^


