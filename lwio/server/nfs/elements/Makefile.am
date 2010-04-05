noinst_LTLIBRARIES = libnfselements.la

libnfselements_la_SOURCES = \
    context.c       \
    globals.c       \
    libmain.c       \
    nfsasyncstate.c \
    nfsconnection.c \
    nfsfile.c       \
    nfsfile2.c      \
    nfsfinder.c     \
    nfsgss.c        \
    nfsioapi.c      \
    nfssession.c    \
    nfssession2.c   \
    nfstimer.c      \
    nfstree.c       \
    nfstree2.c

libnfselements_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/nfs/include

libnfselements_la_LDFLAGS = -static

