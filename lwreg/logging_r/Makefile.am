lib_LTLIBRARIES = libreglog_r.la

libreglog_r_la_SOURCES = \
	globals.c \
	logging.c 

libreglog_r_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
        -DLW_ENABLE_THREADS=1

libreglog_r_la_LIBADD = \
	@top_builddir@/utils/libregutils.la \
	@PTHREAD_LIBS@

noinst_HEADERS =   \
	includes.h \
	externs.h
