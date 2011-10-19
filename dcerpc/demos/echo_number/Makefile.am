## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

MY_IDLFLAGS = -I$(IDL_INCLUDE_DIR)/.. -I$(top_builddir)/include $(DCETHREADINCLUDES)

install-strip:	install

noinst_PROGRAMS=echon_server echon_client

echon_server_SOURCES=misc.c echon_server.c echon_sstub.c
echon_client_SOURCES=misc.c echon_client.c echon_cstub.c

noinst_HEADERS=misc.h echon.idl echon.acf

CLEANFILES=echon.h echon_sstub.c echon_cstub.c

libs=$(top_builddir)/dcelib/libdcerpc.la $(top_builddir)/uuid/libuuid.la

echon_server_LDADD=$(libs)
echon_client_LDADD=$(libs)

echon_server.c: echon.h
echon_client.c: echon.h

BUILT_SOURCES=echon.h echon_sstub.c echon_cstub.c

echon.h echon_sstub.c echon_cstub.c: echon.idl
	$(IDL) $(MY_IDLFLAGS) -keep c_source $(top_srcdir)/demos/echo_number/echon.idl

