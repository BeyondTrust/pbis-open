AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblsalog_r.la

liblsalog_r_la_SOURCES = \
	globals.c \
	logging.c \
	tracing.c

liblsalog_r_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(PSTORE_INCLUDES) \
        -DLW_ENABLE_THREADS=1

liblsalog_r_la_LIBADD = \
	$(top_builddir)/common/liblsacommon.la \
	$(PSTORE_LIBS) \
	$(PTHREAD_LIBS)

liblsalog_r_la_LDFLAGS = \
	$(PSTORE_INCLUDES) \
	$(PTHREAD_INCLUDES)

noinst_HEADERS =   \
	includes.h \
	externs.h
