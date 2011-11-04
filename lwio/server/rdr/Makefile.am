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
    connect2.c            \
    globals.c             \
    session.c             \
    session2.c            \
    socket.c              \
    tree.c                \
    tree2.c               \
    driver.c              \
    dfs.c                 \
    dfs1.c                \
    dfs2.c                \
    create.c              \
    create2.c             \
    read.c                \
    read2.c               \
    write.c               \
    write2.c              \
    setinfo.c             \
    setinfo2.c            \
    queryinfo.c           \
    queryinfo2.c          \
    querydir.c            \
    querydir2.c           \
    deviceio.c            \
    fsctl.c               \
    fsctl2.c              \
    ioctl.c               \
    security.c            \
    security2.c           \
    queryfs.c             \
    queryfs2.c            \
    close.c               \
    close2.c              \
    smb2.c                \
    path.c

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
    @SOCKET_LIBS@ \
    -llwnetclientapi

librdr_sys_la_LDFLAGS = $(DRIVER_LDFLAGS)
