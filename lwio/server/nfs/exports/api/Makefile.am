noinst_LTLIBRARIES = libshareapi.la

libshareapi_la_SOURCES = \
    globals.c  \
    libmain.c  \
    mapping.c  \
    exportapi.c \
    utils.c

libshareapi_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    @SHARE_REPOSITORY_TYPE_DEFINES@ \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/nfs/include \
    -I@top_srcdir@/server/nfs/exports/include
    
if ENABLE_SHARE_REPOSITORY_REGISTRY

libshareapi_la_LIBADD = \
    @top_builddir@/server/nfs/exports/regexports/liblwregexports.la

endif

libshareapi_la_LDFLAGS = -static

