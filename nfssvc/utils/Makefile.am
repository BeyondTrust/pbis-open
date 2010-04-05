AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libnfssvcutils.la

libnfssvcutils_la_SOURCES = \
    consolelog.c \
    filelog.c    \
    fileutils.c  \
    globals.c    \
    hostinfo.c   \
    logger.c     \
    loginfo.c    \
    memory.c     \
    strings.c    \
    sysfuncs.c   \
    syslog.c

libnfssvcutils_la_LIBADD = \
    $(DCERPC_LIBS)         \
    $(LIBUNISTR_LIBS)

libnfssvcutils_la_CFLAGS = \
    -DNFSSVC_BUILD         \
    -DLIBRPC_BUILD

libnfssvcutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS)           \
    -DNFSSVC_BUILD           \
    -DLIBRPC_BUILD           \
    $(DCERPC_INCLUDES)       \
    $(LIBUNISTR_INCLUDES)    \
    $(LWADVAPI_INCLUDES)

libnfssvcutils_la_LDFLAGS = \
    $(DCERPC_LDFLAGS)       \
    $(LIBUNISTR_LDFLAGS)    \
    $(LWADVAPI_LDFLAGS)
