noinst_LTLIBRARIES = liblwioutils.la

liblwioutils_la_SOURCES = \
    assert.c      \
    bitvector.c   \
    ecp.c         \
    dlinkedlist.c \
    fileutils.c   \
    globals.c     \
    hashtable.c   \
    hostinfo.c    \
    libmain.c     \
    lwlist.c      \
    memory.c      \
    multistring.c \
    queue.c       \
    path.c        \
    stack.c       \
    strings.c     \
    sysfuncs.c    \
    unistr.c      \
    regcfg.c      \
    lru.c

liblwioutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/include

liblwioutils_la_LIBADD = \
    @LWBASE_LIBS@ \
    @LWREG_LIBS@  \
    @DL_LIBS@     \
    @RT_LIBS@

liblwioutils_la_LDFLAGS = \
   $(AM_LDFLAGS) \
   @LWBASE_LDFLAGS@
