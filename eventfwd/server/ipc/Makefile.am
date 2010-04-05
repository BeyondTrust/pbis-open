
noinst_LTLIBRARIES = libevtfwdserveripc.la

libevtfwdserveripc_la_CPPFLAGS = \
        $(AM_CPPFLAGS)           \
	-I@top_srcdir@/server/include

libevtfwdserveripc_la_LIBADD = \
	@top_builddir@/ipc/libevtfwdipc.la     \
	@top_builddir@/utils/libevtfwdutils.la \
	@SOCKET_LIBS@                         \
	@NSL_LIBS@                            \
	-llwmsg

libevtfwdserveripc_la_SOURCES = \
	ipc_log.c    \
	ipc_error.c  \
	ipc_state.c  \
	globals.c

noinst_HEADERS = \
    includes.h        \
    externs_p.h       \
    ipc_log_p.h       \
    ipc_error_p.h

