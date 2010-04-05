noinst_LTLIBRARIES = libnfsutils.la

libnfsutils_la_SOURCES = \
    hostinfo.c           \
    memory.c             \
    path.c               \
    prodcons.c           \
    string.c

libnfsutils_la_CPPFLAGS =  \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/nfs/include

libnfsutils_la_LDFLAGS = -static

