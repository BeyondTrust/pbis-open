sbin_PROGRAMS = lwtaskd

lwtaskd_SOURCES = \
    globals.c    \
    ipc.c        \
    listener.c   \
    main.c       \
    sighandler.c

lwtaskd_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1   \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/server/include
 
lwtaskd_LDADD = \
    $(top_builddir)/server/lwmigrate/liblwmigrate.la \
    $(top_builddir)/server/taskrepository/libtaskrepository.la \
    $(top_builddir)/utils/liblwtaskutils.la \
    $(top_builddir)/logging_r/liblwtasklog_r.la \
    $(top_builddir)/ipc/liblwtaskipc.la

