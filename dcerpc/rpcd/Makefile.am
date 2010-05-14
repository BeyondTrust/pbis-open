## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk
C_FLAGS=-Drpcd_c_database_name_prefix1=\"$(localstatedir)/run/\" -I$(top_srcdir)/ncklib -I$(top_srcdir)/ncklib/include/$(target_os)
AM_CFLAGS+=$(C_FLAGS) -Wall -DDCETHREAD_ENFORCE_API

MY_IDLFLAGS=-keep c_source -v -no_mepv -cepv -I. -I$(IDL_INCLUDE_DIR)/.. @DCETHREADINCLUDES@ -I$(top_builddir)/include

dcom_sources=objex_sstub.c

sbin_PROGRAMS=dcerpcd

dcerpcd_SOURCES=dsm.c dsm_unix.c rpcd.c rpcddb.c rpcdep.c rpcdepdb.c rpcdsliv.c rpcdutil.c ep_sstub.c $(rpcd_dcom_sources)

noinst_HEADERS=dsm_p.h rpcddb.h rpcdepdb.h rpcdepdbp.h rpcdp.h rpcdutil.h dsm.idl 

dcerpcd_LDADD=../dcelib/libdcerpc.la ../uuid/libuuid.la ../libdcethread/libdcethread.la
dcerpcd_LDFLAGS=@LTLDFLAGS@

BUILT_SOURCES=ep_sstub.c dsm.h
CLEANFILES=objex.h object_sstub.c ep_sstub.c dsm.h

if ENABLE_DCOM
objex_sstub.c: $(IDL_INCLUDE_DIR)/objex.idl
	$(IDL) $(MY_IDLFLAGS) -client none $<
endif

ep_sstub.c: $(IDL_INCLUDE_DIR)/ep.idl
	$(IDL) $(MY_IDLFLAGS) -client none $<

dsm.h:	dsm.idl
	$(IDL) $(MY_IDLFLAGS) -client none -server none $<

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/rpc

regdir = $(datadir)/config
reg_DATA = dcerpcd.reg
