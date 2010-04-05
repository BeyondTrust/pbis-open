## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

MY_IDLFLAGS = -I$(IDL_INCLUDE_DIR)/.. -I$(top_builddir)/include $(DCETHREADINCLUDES) -keep c_source

install-strip:	install

noinst_PROGRAMS=echou_server echou_client

echou_server_SOURCES=misc.c echou_server.c echou_sstub.c
echou_client_SOURCES=misc.c echou_client.c echou_cstub.c

noinst_HEADERS=misc.h echou.idl echou.acf

CLEANFILES=echou.h echou_sstub.o echou_cstub.o

libs=$(top_builddir)/dcelib/libdcerpc.la $(top_builddir)/uuid/libuuid.la

echou_server_LDADD=$(libs)
echou_client_LDADD=$(libs)

BUILT_SOURCES=echou.h echou_sstub.c echou_cstub.c

echou.h echou_sstub.c echou_cstub.c: echou.idl
	$(IDL) $(MY_IDLFLAGS) -midl $(top_srcdir)/demos/echo_union/echou.idl
