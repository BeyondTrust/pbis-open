lib_LTLIBRARIES = liblwmgmtclient.la

idl_srcdir=$(top_srcdir)/idl

liblwmgmtclient_la_SOURCES = \
	binding.c      \
	client.c       \
        lsamgr.c       \
	lsamgr_cstub.c \
        krbmgr.c       \
	krbmgr_cstub.c
	
liblwmgmtclient_la_LIBADD = \
	@top_builddir@/common/liblwmgmtcommon.la

liblwmgmtclient_la_CPPFLAGS = \
    @DCERPC_INCLUDES@    \
    @LIBUNISTR_INCLUDES@
	
CLEANFILES = lsamgr_h.h lsamgr_cstub.c lsamgr_sstub.c krbmgr_h.h krbmgr_cstub.c krbmgr_sstub.c

BUILT_SOURCES = lsamgr_h.h krbmgr_h.h

lsamgr_h.h lsamgr_cstub.c: $(idl_srcdir)/lsamgr.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header lsamgr_h.h -I$(idl_srcdir) $<
	
krbmgr_h.h krbmgr_cstub.c: $(idl_srcdir)/krbmgr.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header krbmgr_h.h -I$(idl_srcdir) $<

