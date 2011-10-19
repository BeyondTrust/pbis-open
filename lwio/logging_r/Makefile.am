noinst_LTLIBRARIES = liblwiolog_r.la

liblwiolog_r_la_SOURCES = \
    globals.c \
    logging.c

liblwiolog_r_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include    

liblwiolog_r_la_LIBADD = \
    @PTHREAD_LIBS@

noinst_HEADERS = \
    includes.h   \
    externs.h
