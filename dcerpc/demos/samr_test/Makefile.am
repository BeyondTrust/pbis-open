## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

MY_IDLFLAGS = -I$(IDL_INCLUDE_DIR)/.. -I$(top_builddir)/include $(DCETHREADINCLUDES)

install-strip:	install

noinst_PROGRAMS=samrt_server samrt_client

samrt_server_SOURCES=misc.c samrt_server.c samrt_sstub.c
samrt_client_SOURCES=misc.c samrt_client.c samrt_cstub.c

noinst_HEADERS=misc.h samrt.idl samrt.acf

CLEANFILES=samrt.h samrt_sstub.c samrt_cstub.c

libs=$(top_builddir)/dcelib/libdcerpc.la \
	$(top_builddir)/uuid/libuuid.la

samrt_server_LDADD=$(libs)
samrt_client_LDADD=$(libs)

samrt_server.c: samrt.h
samrt_client.c: samrt.h

BUILT_SOURCES=samrt.h samrt_sstub.c samrt_cstub.c

samrt.h samrt_sstub.c samrt_cstub.c: samrt.idl
	$(IDL) $(MY_IDLFLAGS) -keep c_source $(top_srcdir)/demos/samr_test/samrt.idl

