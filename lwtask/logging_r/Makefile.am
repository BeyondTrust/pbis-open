noinst_LTLIBRARIES = liblwtasklog_r.la

liblwtasklog_r_la_SOURCES = \
    globals.c \
    logging.c

liblwtasklog_r_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include    

liblwtasklog_r_la_LIBADD = \
    @PTHREAD_LIBS@

noinst_HEADERS = \
    includes.h   \
    externs.h
