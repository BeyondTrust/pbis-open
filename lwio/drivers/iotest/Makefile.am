
lib_LTLIBRARIES = libiotest.sys.la

libiotest_sys_la_SOURCES = \
    driver.c \
    create.c \
    readwrite.c \
    flush.c \
    iocontrol.c \
    fscontrol.c \
    qsinfo.c \
    namedpipe.c \
    ccb.c \
    test.c \
    workqueue.c \
    irpcontext.c \
    async.c

libiotest_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libiotest_sys_la_LIBADD = \
    @top_builddir@/common/liblwiocommon.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libiotest_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic
