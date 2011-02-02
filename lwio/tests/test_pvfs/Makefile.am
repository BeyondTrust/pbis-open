AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_pvfs

test_pvfs_SOURCES = main.c

test_pvfs_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE

test_pvfs_LDFLAGS =

test_pvfs_LDADD = @top_builddir@/client/liblwioclient.la
