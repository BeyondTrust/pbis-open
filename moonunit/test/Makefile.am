MOONUNIT_LT = $(top_builddir)/src/moonunit/moonunit-lt
MOONUNIT_STUB = $(top_builddir)/src/moonunit/moonunit-stub

check_LTLIBRARIES = example.la

EXAMPLE_SOURCES = $(srcdir)/example.c

example_la_SOURCES = $(EXAMPLE_SOURCES)
nodist_example_la_SOURCES = example_stub.c
example_la_CPPFLAGS = $(AM_CPPFLAGS)
example_la_LDFLAGS = -module -avoid-version -export-dynamic -rpath $(libdir)

if CPLUSPLUS_ENABLED
EXAMPLE_SOURCES += $(srcdir)/example_cpp.cpp
example_la_LINK = $(CXXLINK) $(example_la_LDFLAGS)
else
example_la_LINK = $(LINK) $(example_la_LDFLAGS)
endif

CLEANFILES = example_stub.c
BUILT_SOURCES = example_stub.c

example_stub.c: $(EXAMPLE_SOURCES) $(MOONUNIT_STUB)
	$(MOONUNIT_STUB) $(EXAMPLE_SOURCES) -o $@ \
		CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" \
		CXXCPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" \
		CPP="$(CPP)" CXXCPP="$(CXXCPP)"

AM_CPPFLAGS = -I$(top_srcdir)/include

CHECK_PLUGINS= \
	--plugin $(top_builddir)/src/plugins/c/c.la \
	--plugin $(top_builddir)/src/plugins/console/console.la \
	--plugin $(top_builddir)/src/plugins/xml/xml.la \
	--plugin $(top_builddir)/src/plugins/shell/sh.la \
	--loader-option "sh:helper=$(top_builddir)/src/plugins/shell/mu.sh"

check-local: $(MOONUNIT_LT) example.la example.sh example.res
	$(MOONUNIT_LT) $(CHECK_ARGS) $(CHECK_PLUGINS) -r $(srcdir)/example.res $(top_builddir)/test/example.la $(srcdir)/example.sh

EXTRA_DIST = example.sh example.res