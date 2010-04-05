noinst_LTLIBRARIES = libnfssvclog_r.la

libnfssvclog_r_la_SOURCES = \
    globals.c \
    logging.c

libnfssvclog_r_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include    

libnfssvclog_r_la_LIBADD = \
    @PTHREAD_LIBS@

noinst_HEADERS = \
    includes.h   \
    externs.h
