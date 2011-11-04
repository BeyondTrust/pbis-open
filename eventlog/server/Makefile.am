AM_CPPFLAGS = -I$(top_srcdir)/include

sbin_PROGRAMS = eventlogd

idl_srcdir=$(top_srcdir)/idl

eventlogd_SOURCES = \
    access.c         \
    db.c             \
    eventlog_sstub.c \
    globals.c        \
    main.c           \
    security.c       \
    server.c

eventlogd_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    @GSSAPI_INCLUDES@ \
    @DCERPC_INCLUDES@

eventlogd_LDADD = \
    @top_builddir@/utils/libeventlogutils.la \
    @GSSAPI_LIBS@ \
    @SQLITE_LIBS@ \
    @PTHREAD_LIBS@ \
    @DL_LIBS@ \
    -ldcerpc \
    -llwbase \
    -llwbase_nothr \
    -llwadvapi \
    -llwadvapi_nothr

eventlogd_LDFLAGS = \
    @GSSAPI_LDFLAGS@ \
    @SQLITE_LDFLAGS@

CLEANFILES = \
    eventlog_h.h \
    eventlog_cstub.c \
    eventlog_sstub.c

BUILT_SOURCES = eventlog_h.h

eventlog_h.h eventlog_sstub.c: $(idl_srcdir)/eventlog.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header eventlog_h.h -I$(idl_srcdir) $<

