ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
    @top_builddir@/server/iomgr/libiomgr.la

if ENABLE_STATIC_DRIVERS
noinst_LTLIBRARIES = librdr.sys.la
DRIVER_CFLAGS =
DRIVER_LDFLAGS =
if !ENABLE_SMALL_DAEMON
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif
else
lib_LTLIBRARIES = librdr.sys.la
DRIVER_CFLAGS = -shared
DRIVER_LDFLAGS = -module -shared -avoid-version -export-symbols $(srcdir)/rdr.sym
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif

librdr_sys_la_SOURCES =  \
    connect.c             \
    getsesskey.c          \
    globals.c             \
    session.c             \
    socket.c              \
    tree.c                \
    driver.c              \
    create.c              \
    read.c                \
    write.c               \
    setinfo.c             \
    queryinfo.c           \
    querydir.c            \
    deviceio.c            \
    fsctl.c               \
    security.c            \
    queryfs.c             \
    close.c

librdr_sys_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@ \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

librdr_sys_la_CFLAGS = $(AM_CFLAGS) $(DRIVER_CFLAGS) -Wmissing-prototypes

librdr_sys_la_LIBADD = \
    @top_builddir@/server/smbcommon/liblwiosmbcommon.la \
    $(ADDITIONAL_LIBS) \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@ \
    @SOCKET_LIBS@

librdr_sys_la_LDFLAGS = $(DRIVER_LDFLAGS)
