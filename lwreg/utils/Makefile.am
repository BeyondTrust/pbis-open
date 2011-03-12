noinst_LTLIBRARIES = libregutils.la

libregutils_la_SOURCES = \
    fileutils.c          \
    globals.c            \
    regmem.c             \
    sysfuncs.c           \
    regtime.c            \
    dlinkedlist.c        \
    hashtable.c          \
    regerror.c           \
    regdatatype.c

noinst_HEADERS = \
    externs.h       \
    includes.h

libregutils_la_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@
libregutils_la_LDFLAGS = @LWBASE_LDFLAGS@
libregutils_la_LIBADD = @LWBASE_LIBS@
