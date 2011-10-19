## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

MY_IDLFLAGS = -I$(IDL_INCLUDE_DIR)/.. -I$(top_builddir)/include $(DCETHREADINCLUDES) -keep c_source

install-strip:	install

noinst_PROGRAMS=access_server access_client

access_server_SOURCES=misc.c access_server.c access_sstub.c
access_client_SOURCES=misc.c access_client.c access_cstub.c

noinst_HEADERS=misc.h access.idl access.acf

CLEANFILES=access.h access_sstub.c access_cstub.c

libs=$(top_builddir)/dcelib/libdcerpc.la $(top_builddir)/uuid/libuuid.la

access_server_LDADD=$(libs)
access_client_LDADD=$(libs)

BUILT_SOURCES=access.h access_sstub.c access_cstub.c

access.h access_sstub.c access_cstub.c: access.idl
	$(IDL) $(MY_IDLFLAGS) $(top_srcdir)/demos/access/access.idl

