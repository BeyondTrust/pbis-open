
noinst_LTLIBRARIES = librsysserveripc.la

librsysserveripc_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I@top_srcdir@/server/include

librsysserveripc_la_LIBADD = \
	@top_builddir@/ipc/librsysipc.la     \
	@top_builddir@/utils/librsysutils.la \
	@SOCKET_LIBS@                         \
	@NSL_LIBS@                            \
	-llwmsg

librsysserveripc_la_SOURCES = \
	ipc_log.c    \
	ipc_error.c  \
	ipc_state.c  \
	globals.c

noinst_HEADERS = \
    includes.h        \
    externs_p.h       \
    ipc_log_p.h       \
    ipc_error_p.h

