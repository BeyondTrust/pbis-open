lib_LTLIBRARIES = liblwmgmtcommon.la

liblwmgmtcommon_la_SOURCES = \
    cfg.c       \
    fileutils.c \
    globals.c   \
    logger.c    \
    mem.c       \
    rpcerror.c  \
    rpcmem.c    \
    stack.c     \
    str.c       \
    sysfuncs.c

liblwmgmtcommon_la_LIBADD = \
    @DCERPC_LIBS@           \
    @LIBUNISTR_LIBS@

liblwmgmtcommon_la_CPPFLAGS = \
    @DCERPC_INCLUDES@         \
    @LIBUNISTR_INCLUDES@

liblwmgmtcommon_la_LDFLAGS = \
    @DCERPC_LDFLAGS@         \
    @LIBUNISTR_LDFLAGS@

