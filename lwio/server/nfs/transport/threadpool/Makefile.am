noinst_LTLIBRARIES = libnfs.transport.threadpool.la

libnfs_transport_threadpool_la_SOURCES = \
    libmain.c       \
    listener.c      \
    nfssocket.c

libnfs_transport_threadpool_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/nfs/include \
    -I@top_srcdir@/server/nfs/transport/include

libnfs_transport_threadpool_la_LDFLAGS =
