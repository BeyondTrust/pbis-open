noinst_LTLIBRARIES = libnfs3procs.la

libnfs3procs_la_SOURCES =   \
	exec_context.c

libnfs3procs_la_CPPFLAGS = 	    \
    $(AM_CPPFLAGS)         	    \
    -DLW_ENABLE_THREADS=1  	    \
    -I@top_srcdir@/include 	    \
	-I@top_srcdir@/proto/include

libnfs3procs_la_LDFLAGS = -static

