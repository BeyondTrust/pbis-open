#!gmake
#
# Makefile for tdb directory
#

lib_LTLIBRARIES = libtdb.la
bin_PROGRAMS = tdbtool tdbtorture
noinst_PROGRAMS = tdbdump tdbbackup
AM_CPPFLAGS += -DSTANDALONE=1 -I$(srcdir)/include -Iinclude -I../include -I$(top_srcdir)/replace

libtdb_la_SOURCES = common/tdb.c common/dump.c common/transaction.c common/error.c common/traverse.c common/freelist.c common/freelistcheck.c common/io.c common/lock.c common/open.c common/tdbback.c

tdbtool_SOURCES = tools/tdbtool.c
tdbtool_LDADD = libtdb.la

tdbtorture_SOURCES = tools/tdbtorture.c
tdbtorture_LDADD = libtdb.la

tdbdump_SOURCES = tools/tdbdump.c
tdbdump_LDADD = libtdb.la

tdbbackup_SOURCES = tools/tdbbackup.c
tdbbackup_LDADD = libtdb.la
