AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES=liblwps-utils.la

liblwps_utils_la_SOURCES = \
       lwps-futils.c   \
       lwps-error.c    \
       lwps-mem.c      \
       lwps-paths.c    \
       lwps-rwlock.c   \
       lwps-sid.c      \
       lwps-stack.c    \
       lwps-str.c      \
       lwps-strerror.c \
       lwps-sysfuncs.c

liblwps_utils_la_LDFLAGS =
liblwps_utils_la_LIBADD = \
	-llwbase \
	$(top_builddir)/logging/liblwps_logging.la

noinst_HEADERS = \
	lwps-error_p.h \
	lwps-rwlock_p.h \
	lwps-sid_p.h
