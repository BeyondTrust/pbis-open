AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lw-eventlog-cli

lw_eventlog_cli_SOURCES = \
    evtparser.c \
    main.c

lw_eventlog_cli_LDADD = @top_builddir@/client/libeventlog.la

