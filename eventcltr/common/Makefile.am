
lib_LTLIBRARIES=libeventcltrcommon.la

idl_srcdir=$(top_srcdir)/idl

libeventcltrcommon_la_SOURCES = \
	cltr-cfg.c \
	cltr-futils.c \
	cltr-mem.c \
	cltr-parser.c \
	cltr-rpcmem.c \
	cltr-stack.c \
	cltr-str.c

noinst_HEADERS = \
	cltr-base.h \
	cltr-cfg_p.h \
	cltr-futils.h \
	cltr-mem.h \
	cltr-parser.h \
	cltr-rpcmem.h \
	cltr-str.h \
	Resource.h

libeventcltrcommon_la_LIBADD =  \
	$(LIBUNISTR_LIBS)

libeventcltrcommon_la_LDFLAGS =  \
	-static \
	$(AM_LDFLAGS)

CLEANFILES = eventlog_h.h eventlog_cstub.c eventlog_sstub.c

BUILT_SOURCES = eventlog_h.h

eventlog_h.h eventlog_cstub.c: $(idl_srcdir)/eventlog.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header eventlog_h.h -I$(idl_srcdir) -I$(top_srcdir)/include $<
