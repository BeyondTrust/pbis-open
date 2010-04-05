AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_load

test_load_SOURCES = main.c
test_load_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE
test_load_LDADD = @top_builddir@/client/liblwioclient.la
# Hack around broken HP-UX linker
test_load_LDFLAGS = -L@top_builddir@/common/.libs
