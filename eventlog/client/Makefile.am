AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libeventlog.la libeventlog-mac.la

idl_srcdir=$(top_srcdir)/idl

libeventlog_la_SOURCES = \
    binding.c  \
    eventlog.c \
    legacy.c \
    eventlog_cstub.c

libeventlog_la_LIBADD = \
    @top_builddir@/utils/libeventlogutils.la

libeventlog_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    @DCERPC_INCLUDES@

libeventlog_mac_la_SOURCES = \
    $(libeventlog_la_SOURCES)

libeventlog_mac_la_LIBADD = \
    $(libeventlog_la_LIBADD)

libeventlog_mac_la_LDFLAGS = \
    -module -avoid-version

libeventlog_mac_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    $(libeventlog_la_CPPFLAGS)

CLEANFILES = eventlog_h.h eventlog_cstub.c eventlog_sstub.c

BUILT_SOURCES = eventlog_h.h

eventlog_h.h eventlog_cstub.c: $(idl_srcdir)/eventlog.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header eventlog_h.h -I$(idl_srcdir) -I$(top_srcdir)/include $<

