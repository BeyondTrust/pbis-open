noinst_LTLIBRARIES = liblwioutils.la

liblwioutils_la_SOURCES = \
    assert.c      \
    bitvector.c   \
    consolelog.c  \
    ecp.c         \
    dlinkedlist.c \
    filelog.c     \
    fileutils.c   \
    globals.c     \
    handlemgr.c   \
    hashtable.c   \
    hostinfo.c    \
    libmain.c     \
    logger.c      \
    lwlist.c      \
    memory.c      \
    queue.c       \
    lwiologinfo.c \
    path.c        \
    stack.c       \
    strings.c     \
    sysfuncs.c    \
    syslog.c      \
    unistr.c      \
    regcfg.c

liblwioutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/include

liblwioutils_la_LIBADD = \
    @LIBUNISTR_LIBS@ \
    @LWBASE_LIBS@ \
    @DL_LIBS@  \
	@LWREG_LIBS@

liblwioutils_la_LDFLAGS = \
   $(AM_LDFLAGS) \
   @LWBASE_LDFLAGS@
