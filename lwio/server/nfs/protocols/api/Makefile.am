noinst_LTLIBRARIES = libnfs.protocols.api.la

libnfs_protocols_api_la_SOURCES = \
    config.c    \
    globals.c   \
    libmain.c   \
    negotiate.c \
    transport.c

libnfs_protocols_api_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/nfs/include \
    -I@top_srcdir@/server/nfs/protocols/include
    
libnfs_protocols_api_la_LIBADD = \
    @top_builddir@/server/nfs/protocols/smb2/libnfs.protocols.smb2.la

libnfs_protocols_api_la_LDFLAGS = -static

