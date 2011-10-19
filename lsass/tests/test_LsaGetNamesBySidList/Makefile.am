AM_CPPFLAGS = @AM_CPPFLAGS@ -DLSA_ENABLE_DEPRECATED -I$(top_srcdir)/include

noinst_PROGRAMS = test_LsaGetNamesBySidList

test_LsaGetNamesBySidList_SOURCES = main.c
test_LsaGetNamesBySidList_LDFLAGS = -static
test_LsaGetNamesBySidList_LDADD = @top_builddir@/client/liblsaclient.la