## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

MY_IDLFLAGS = -no_mepv -I$(IDL_INCLUDE_DIR)/.. -I$(top_builddir)/include $(DCETHREADINCLUDES)
libs=$(top_builddir)/dcelib/libdcerpc.la \
	$(top_builddir)/threads/libdcepthreads.la \
	$(top_builddir)/uuid/libuuid.la -lstdc++ \
	$(top_builddir)/ncklib/transport/cn/libprot_ncacn.la \
	$(top_builddir)/ncklib/transport/dg/libprot_ncadg.la 


EXTRA_PROGRAMS=client server
TESTS=run_tests.sh
EXTRA_DIST=run_tests.sh

run_tests.sh:	$(EXTRA_PROGRAMS)

client_SOURCES=client.c util.c perfg.c perfb.c
server_SOURCES=server.c util.c perf.c perfg.c perfb.c
client_LDADD=perf_cstub.o perfb_cstub.o perfc_sstub.o perfg_cstub.o $(libs)
server_LDADD=perf_sstub.o perfb_sstub.o perfc_cstub.o perfg_sstub.o $(libs)

noinst_HEADERS=perf_c.h perf_p.h vax.h perf.idl perfb.idl perfc.idl perfg.idl

#BUILT_SOURCES=perf.h perfb.h perfc.h perfg.h
CLEANFILES=perf.h perfb.h perfc.h perfg.h server client

client.c: perf.h perfb.h perfc.h perfg.h
server.c: perf.h perfb.h perfc.h perfg.h

perf_cstub.o perf_sstub.o perf.h:	perf.idl
	$(IDL) $(MY_IDLFLAGS) perf.idl

perfb_cstub.o perfb_sstub.o perfb.h: perfb.idl
	$(IDL) $(MY_IDLFLAGS) perfb.idl

perfc_cstub.o perfc_sstub.o perfc.h: perfc.idl
	$(IDL) $(MY_IDLFLAGS) perfc.idl

perfg_cstub.o perfg_sstub.o perfg.h: perfg.idl
	$(IDL) $(MY_IDLFLAGS) perfg.idl

