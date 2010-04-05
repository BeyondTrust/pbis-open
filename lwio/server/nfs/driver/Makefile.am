if ENABLE_STATIC_DRIVERS
noinst_LTLIBRARIES = libnfs.sys.la
DRIVER_CFLAGS =
DRIVER_LDFLAGS =
else
lib_LTLIBRARIES = libnfs.sys.la
DRIVER_CFLAGS = -shared
DRIVER_LDFLAGS = -module -shared -avoid-version
endif

libnfs_sys_la_SOURCES = \
    ccb.c          \
    config.c       \
    device.c       \
    deviceclose.c  \
    devicecreate.c \
    deviceio.c     \
    driver.c       \
    globals.c      \
    nfsexports.c   \
    nfsstats.c     \
    nfsworker.c

libnfs_sys_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/nfs/include

libnfs_sys_la_CFLAGS = $(AM_CFLAGS) $(DRIVER_CFLAGS)

libnfs_sys_la_LIBADD = \
    @top_builddir@/server/nfs/protocols/api/libnfs.protocols.api.la \
    @top_builddir@/server/nfs/transport/threadpool/libnfs.transport.threadpool.la \
    @top_builddir@/server/nfs/elements/libnfselements.la \
    @top_builddir@/server/nfs/exports/api/libshareapi.la \
    @top_builddir@/server/nfs/utils/libnfsutils.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @top_builddir@/server/smbcommon/liblwiosmbcommon.la \
    @top_builddir@/libshareinfo/liblwioshareinfo.la \
    @KRB5_LIBS@ \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@

libnfs_sys_la_LDFLAGS = $(DRIVER_LDFLAGS)

