AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_pvfs

test_pvfs_SOURCES = main.c

test_pvfs_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE

test_pvfs_LDFLAGS = \
    -L$(top_builddir)/common/.libs \
    -L$(top_builddir)/libshareinfo/.libs

test_pvfs_LDADD = \
    @top_builddir@/client/liblwioclient.la \
    @top_builddir@/common/liblwiocommon.la \
    -llwbase -llwbase_nothr
