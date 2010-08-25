noinst_LTLIBRARIES = liblwtaskutils.la

liblwtaskutils_la_SOURCES = \
    consolelog.c    \
    filelog.c       \
    globals.c       \
    libmain.c       \
    logger.c        \
    lwtaskloginfo.c \
    queue.c         \
    sysfuncs.c      \
    syslog.c        \
    task.c

liblwtaskutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/include

liblwtaskutils_la_LIBADD = \
    @LWADVAPI_LIBS@ \
    @RT_LIBS@

liblwtaskutils_la_LDFLAGS = \
   $(AM_LDFLAGS) \
   @LWADVAPI_LDFLAGS@
