bin_PROGRAMS = lw-eventlog-cli

lw_eventlog_cli_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(LWADVAPI_INCLUDES) \
	$(POPT_INCLUDES)

lw_eventlog_cli_SOURCES = \
    evtparser.c \
    main.c

lw_eventlog_cli_LDADD = \
	$(top_builddir)/client/libeventlog.la \
	$(LWADVAPI_LIBS) \
	$(POPT_LIBS)

lw_eventlog_cli_LDFLAGS = \
	$(LWADVAPI_LDFLAGS) \
	$(POPT_LDFLAGS)
