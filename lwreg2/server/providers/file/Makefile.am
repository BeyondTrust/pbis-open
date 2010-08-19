lib_LTLIBRARIES = libregserver_provider_file.la

libregserver_provider_file_la_SOURCES = \
    fileapi.c \
    globals.c            

libregserver_provider_file_la_CPPFLAGS =        \
    -DLW_ENABLE_THREADS=1         \
    $(AM_CFLAGS)                  \
    -I@top_srcdir@/server/include 

libregserver_provider_file_la_LIBADD = \
    @PTHREAD_LIBS@    