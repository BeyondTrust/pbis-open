ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
    @top_builddir@/server/iomgr2/libiomgr2.la

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
DRIVER_LDFLAGS = -module -shared -avoid-version
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif

librdr_sys_la_SOURCES =   \
    client_session.c      \
    client_socket.c       \
    client_tree.c         \
    createfile.c          \
    closefile.c           \
    getsesskey.c          \
    globals.c             \
    readfile.c            \
    queryinfofile.c       \
    setinfofile.c         \
    renamefile.c          \
    querysecurity.c       \
    queryfsinfo.c         \
    findfirst2.c          \
    findnext2.c           \
    response.c            \
    session.c             \
    smb_logoff.c          \
    smb_negotiate.c       \
    smb_nptransact.c      \
    smb_session_setup.c   \
    smb_tree_connect.c    \
    smb_tree_disconnect.c \
    smb_write.c           \
    socket.c              \
    tree.c                \
    writefile.c           \
    reaper.c              \
    lwio-semaphore.c      \
    rdrdriver.c           \
    rdrcreate.c           \
    rdrread.c             \
    rdrwrite.c            \
    rdrsetinfo.c          \
    rdrqueryinfo.c        \
    rdrquerydir.c         \
    rdrdeviceio.c         \
    rdrfsctl.c            \
    rdrsecurity.c         \
    rdrqueryfs.c          \
    rdrclose.c

librdr_sys_la_CPPFLAGS = \
    -I@top_srcdir@  	   \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

librdr_sys_la_CFLAGS = $(AM_CFLAGS) $(DRIVER_CFLAGS)

librdr_sys_la_LIBADD = \
    @top_builddir@/server/smbcommon/liblwiosmbcommon.la \
    $(ADDITIONAL_LIBS) \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@ \
    @SOCKET_LIBS@

librdr_sys_la_LDFLAGS = $(DRIVER_LDFLAGS)
