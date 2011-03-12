lib_LTLIBRARIES = liblwioclient.la

liblwioclient_la_SOURCES = \
    access.c               \
    connect.c              \
    driver.c               \
    globals.c              \
    libmain.c              \
    ntfileapictx.c         \
    smbfileapictx.c        \
    srvshareapi.c          \
    thread.c               \
    win32.c

liblwioclient_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLWIO_NO_THREADS \
    -I@top_srcdir@/include \
    @LWMSG_INCLUDES@

liblwioclient_la_CFLAGS = \
    $(AM_CFLAGS) \
    -Wmissing-prototypes

liblwioclient_la_LIBADD = \
    @top_builddir@/common/liblwiocommon.la \
    @top_builddir@/libshareinfo/liblwioshareinfo.la \
    @LWMSG_LIBS@ \
    -lkrb5
