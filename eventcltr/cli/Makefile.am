bin_PROGRAMS = lw-eventcltr-cli

lw_eventcltr_cli_CPPFLAGS = \
	-I$(top_srcdir)/clientlib \
	-I$(top_srcdir)/common \
	$(AM_CPPFLAGS)

lw_eventcltr_cli_SOURCES = main.c

noinst_HEADERS = 
	client.h \
	stdafx.h

lw_eventcltr_cli_LDADD = \
	$(top_builddir)/clientlib/libeventcltr.la \
	$(top_builddir)/logging/libcltrlogging.la \
	$(DCERPC_LIBS) \
	$(LWRPCRT_LIBS)
