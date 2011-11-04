AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir) -I$(top_srcdir)/server/api

noinst_PROGRAMS = nbname1 nbname2 nbquery

nbname1_LDADD = \
	@top_builddir@/server/api/liblwnetserverapi.la \
        @top_builddir@/common/liblwnetcommon.la

nbname1_SOURCES = nbname1.c

nbname2_LDADD = \
	@top_builddir@/server/api/liblwnetserverapi.la \
        @top_builddir@/common/liblwnetcommon.la

nbquery_LDADD = \
	@top_builddir@/server/api/liblwnetserverapi.la \
        @top_builddir@/common/liblwnetcommon.la

nbquery_SOURCES = nbquery.c
