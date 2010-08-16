bin_PROGRAMS = lwtask-cli

lwtask_cli_SOURCES  = \
    log.c  \
    main.c \
    task.c

lwtask_cli_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1   \
    -I@top_srcdir@/include

lwtask_cli_LDADD = $(top_builddir)/client/liblwtaskclient.la

