lib_LTLIBRARIES = libeventcltr.la

idl_srcdir=$(top_srcdir)/idl

libeventcltr_la_SOURCES = \
    eventlogbinding.c  \
    eventlog.c \
    clientlib.c \
    stdafx.c \
    eventlog_cstub.c

libeventcltr_la_LIBADD = \
    $(top_builddir)/logging/libcltrlogging.la \
    @top_builddir@/common/libeventcltrcommon.la

libeventcltr_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/common   \
    @DCERPC_INCLUDES@   \
    $(LWRPCRT_INCLUDES)

libeventcltr_la_LDFLAGS = \
    -static \
    $(AM_LDFLAGS)

CLEANFILES = eventlog_h.h eventlog_cstub.c eventlog_sstub.c

BUILT_SOURCES = eventlog_h.h

eventlog_h.h eventlog_cstub.c: $(idl_srcdir)/eventlog.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header eventlog_h.h -I$(idl_srcdir) -I$(top_srcdir)/include $<
