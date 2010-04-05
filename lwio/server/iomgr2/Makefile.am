if ENABLE_STATIC_DRIVERS
noinst_LTLIBRARIES = libiomgr2.la
else
lib_LTLIBRARIES = libiomgr2.la
endif

libiomgr2_la_SOURCES = \
    ioinit.c \
    ioconfig.c \
    ioroot.c \
    iodriver.c \
    iodevice.c \
    ioirp.c \
    iofile.c \
    iomem.c \
    ioapi.c \
    ioipc.c \
    iosecurity.c \
    lwthreads.c \
    iolog.c \
    lwzct.c

libiomgr2_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1 \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

libiomgr2_la_LIBADD = \
    @top_builddir@/common/liblwiocommon.la \
    @top_builddir@/logging_r/liblwiolog_r.la  \
    @LWBASE_LIBS@ \
    @LWMSG_LIBS@ \
    @UUID_LIBS@  \
    @PTHREAD_LIBS@ \
    @SEM_LIBS@ \
    @SOCKET_LIBS@ \
	@LWREG_LIBS@

libiomgr2_la_LDFLAGS = \
    -no-undefined
