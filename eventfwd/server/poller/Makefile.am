
noinst_LTLIBRARIES = libevtfwdserverpoller.la

libevtfwdserverpoller_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
	$(PSTORE_INCLUDES) \
	$(KRB5_INCLUDES) \
	$(LWIO_INCLUDES) \
	-I@top_srcdir@/server/include

libevtfwdserverpoller_la_LIBADD = \
	$(top_builddir)/utils/libevtfwdutils.la \
	$(PSTORE_LIBS) \
	$(KRB5_LIBS) \
        $(EVENTLOG_LIBS) \
        $(EVENTCLTR_LIBS) \
        $(LWIO_LIBS)

libevtfwdserverpoller_la_SOURCES = \
	poller.c    \
	krb5.c	    \
	globals.c

libevtfwdserverpoller_la_LDFLAGS = \
	-static \
	$(PSTORE_LDFLAGS) \
	$(KRB5_LDFLAGS) \
	$(LWIO_LDFLAGS) \
	$(AM_LDFLAGS)

noinst_HEADERS = \
    includes.h        \
    externs_p.h       \
    poller_p.h        \
    krb5_p.h

