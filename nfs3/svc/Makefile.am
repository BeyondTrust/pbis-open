noinst_LTLIBRARIES = libnfs3svc.la

libnfs3svc_la_SOURCES = 	\
    mount3_xdr.c            \
    nfs3_procs_xdr.c        \
    nfs3_types_xdr.c        \
    nfs_xdr.c               \
    rpc2_xdr.c

libnfs3svc_la_CPPFLAGS =   \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@/include \
    -I@top_srcdir@/svc/include

if ENABLE_STATIC_DRIVERS
    libnfs3svc_la_LDFLAGS = -static
endif
