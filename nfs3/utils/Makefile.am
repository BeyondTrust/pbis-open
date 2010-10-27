noinst_LTLIBRARIES = libnfs3utils.la

libnfs3utils_la_SOURCES = 	\
	memory.c				\
    prodcons.c				\
	queue.c

libnfs3utils_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@/include

libnfs3utils_la_LDFLAGS = -static

