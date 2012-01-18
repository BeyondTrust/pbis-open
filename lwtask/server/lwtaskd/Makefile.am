sbin_PROGRAMS = lwtaskd

lwtaskd_SOURCES = \
    context.c    \
    globals.c    \
    ipc.c        \
    listener.c   \
    main.c       \
    prodcons.c   \
    sighandler.c \
    task.c       \
    worker.c

lwtaskd_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -DLW_ENABLE_THREADS=1   \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/server/include
 
lwtaskd_LDADD = \
    $(top_builddir)/server/lwmigrate/liblwmigrate.la \
    $(top_builddir)/server/taskrepository/libtaskrepository.la \
    $(top_builddir)/utils/liblwtaskutils.la \
    $(top_builddir)/logging_r/liblwtasklog_r.la \
    $(top_builddir)/ipc/liblwtaskipc.la \
    @NETLOGON_LIBS@ \
    @UUID_LIBS@ \
    @KRB5_LIBS@ \
    @PTHREAD_LIBS@ \
    @DL_LIBS@ \
    @RT_LIBS@

