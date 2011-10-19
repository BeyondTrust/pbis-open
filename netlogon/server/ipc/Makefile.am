AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblwnetserveripc.la

liblwnetserveripc_la_CPPFLAGS = \
	-I@top_srcdir@/server/include \
	$(AM_CPPFLAGS)

liblwnetserveripc_la_SOURCES = \
	ipc_dcinfo.c    \
	globals.c

liblwnetserveripc_la_LIBADD = \
	-llwmsg

noinst_HEADERS = \
    includes.h        \
    externs_p.h       \
    ipc_dcinfo_p.h
