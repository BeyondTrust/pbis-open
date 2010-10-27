if ENABLE_STATIC_DRIVERS
noinst_LTLIBRARIES = libnfs3.sys.la
DRIVER_CFLAGS =
DRIVER_LDFLAGS =
else
lib_LTLIBRARIES = libnfs3.sys.la
DRIVER_CFLAGS = -shared
DRIVER_LDFLAGS = -module -shared -avoid-version
endif

libnfs3_sys_la_SOURCES = \
	dispatch.c		\
    driver.c		\
	globals.c		\
	initialize.c    \
    nfs3worker.c

libnfs3_sys_la_CPPFLAGS =           \
    -DLW_ENABLE_THREADS=1           \
    -I@top_srcdir@                  \
    -I@top_srcdir@/include          \
    -I@top_srcdir@/proto/include    \
    $(LWIO_INCLUDES)

libnfs3_sys_la_CFLAGS = $(AM_CFLAGS) $(DRIVER_CFLAGS)

libnfs3_sys_la_LIBADD =                         \
    @top_builddir@/proto/procs/libnfs3procs.la  \
    @top_builddir@/utils/libnfs3utils.la        \
    $(LWIO_LIBS)                                \
    @PTHREAD_LIBS@

#    @top_builddir@/server/iomgr/libiomgr.la 
#    @top_builddir@/libshareinfo/liblwioshareinfo.la 

libnfs3_sys_la_LDFLAGS = \
    $(DRIVER_LDFLAGS)   \
    $(LWIO_LDFLAGS)

