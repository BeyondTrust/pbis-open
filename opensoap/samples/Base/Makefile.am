# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
##
##AUTOMAKE_OPTIONS=no-dependencies

noinst_PROGRAMS = xmliconv sendreq

INCLUDES = -I$(top_builddir) -I$(top_builddir)/include

xmliconv_SOURCES = xmliconv.c
xmliconv_LDADD = $(top_builddir)/src/api/libOpenSOAPClient.la

sendreq_SOURCES = sendreq.c
sendreq_LDADD = $(top_builddir)/src/api/libOpenSOAPClient.la

EXTRA_DIST = \
	READMESJ.TXT \
	README.TXT
