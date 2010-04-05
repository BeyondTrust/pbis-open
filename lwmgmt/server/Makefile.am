sbin_PROGRAMS = lwmgmtd

idl_srcdir=$(top_srcdir)/idl

lwmgmtd_SOURCES =  \
	globals.c      \
	main.c         \
	krbmgrserver.c \
	lsamgrserver.c \
	lsamgr_sstub.c \
	krbmgr_sstub.c

lwmgmtd_CPPFLAGS =       \
	@KRB5_INCLUDES@    \
	@DCERPC_INCLUDES@    \
	@LIBUNISTR_INCLUDES@ \
	@LSASS_INCLUDES@

lwmgmtd_LDADD = \
	@top_builddir@/common/liblwmgmtcommon.la \
        @LSASS_LIBS@ \
        @KRB5_LIBS@

lwmgmtd_LDFLAGS = \
        @LSASS_LDFLAGS@ \
        @KRB5_LDFLAGS@
	
CLEANFILES = \
	 lsamgr_h.h      \
	 lsamgr_cstub.c  \
	 lsamgr_sstub.c \
	 krbmgr_h.h     \
	 krbmgr_cstub.c \
	 krbmgr_sstub.c

BUILT_SOURCES = lsamgr_h.h krbmgr_h.h

lsamgr_h.h lsamgr_cstub.c: $(idl_srcdir)/lsamgr.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header lsamgr_h.h -I$(idl_srcdir) $<
	
krbmgr_h.h krbmgr_cstub.c: $(idl_srcdir)/krbmgr.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header krbmgr_h.h -I$(idl_srcdir) $<
	

