AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

testbin_PROGRAMS = test_perf

test_perf_SOURCES = main.c tests.c
test_perf_LDFLAGS = 
test_perf_LDADD = $(RT_LIBS) $(DL_LIBS)

SUBDIRS = lsassonly
