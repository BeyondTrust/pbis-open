
lib_LTLIBRARIES=libevtfwdutils.la

libevtfwdutils_la_SOURCES = \
	evtfwd-regcfg.c \
	evtfwd-cfg.c \
	evtfwd-dlinkedlist.c \
	evtfwd-error.c \
	evtfwd-futils.c \
	evtfwd-mem.c \
	evtfwd-paths.c \
	evtfwd-stack.c \
	evtfwd-str.c \
	evtfwd-strerror.c \
	evtfwd-sysfuncs.c

noinst_HEADERS = \
        includes.h      \
	evtfwd-cfg_p.h   \
	evtfwd-sysfuncs_p.h   \
	evtfwd-error_p.h

libevtfwdutils_la_LIBADD = \
	@LWREG_LIBS@

libevtfwdutils_la_LDFLAGS =  \
	@top_builddir@/logging/libevtfwdlogging.la \
	@LWREG_LIBS@
