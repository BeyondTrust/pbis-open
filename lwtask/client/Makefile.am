lib_LTLIBRARIES = liblwtaskclient.la

liblwtaskclient_la_SOURCES = \
    connect.c \
    logging.c \
    pid.c     \
    task.c

liblwtaskclient_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLW_ENABLE_THREADS=1   \
    -I@top_srcdir@/include \
    @LWMSG_INCLUDES@

liblwtaskclient_la_CFLAGS = \
    $(AM_CFLAGS) \
    -Wmissing-prototypes

liblwtaskclient_la_LIBADD = \
    $(top_builddir)/utils/liblwtaskutils.la \
    $(top_builddir)/logging_r/liblwtasklog_r.la \
    $(top_builddir)/ipc/liblwtaskipc.la \
    @LWMSG_LIBS@ \
    @KRB5_LIBS@
