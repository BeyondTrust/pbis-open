AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_createfile

test_createfile_SOURCES = main.c
test_createfile_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE
test_createfile_LDADD = @top_builddir@/client/liblwioclient.la
# Hack around broken HP-UX linker
test_createfile_LDFLAGS = -L@top_builddir@/common/.libs
