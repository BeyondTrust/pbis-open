noinst_LTLIBRARIES = liblwregexports.la

liblwregexports_la_SOURCES = \
    globals.c   \
    libmain.c   \
    regshare.c

liblwregexports_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/nfs/include \
    -I@top_srcdir@/server/nfs/exports/include
    
liblwregexports_la_LIBADD = \
    @SQLITE_LIBS@

liblwregexports_la_LDFLAGS = -static

