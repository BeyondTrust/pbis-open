if ENABLE_SMALL_DAEMON
lib_LTLIBRARIES = liblwiomain.la
else
noinst_LTLIBRARIES = liblwiomain.la
endif

MAIN_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include

MAIN_LIBS = \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @NETLOGON_LIBS@ \
    @LWMSG_LIBS@ \
	@LWREG_LIBS@

lwiod_LDFLAGS = -L$(top_builddir)/common/.libs \
    @DS_FRAMEWORK_LDFLAGS@

if ENABLE_STATIC_DRIVERS

if ENABLE_RDR
MAIN_LIBS += $(top_builddir)/server/rdr/librdr.sys.la
endif

if ENABLE_SRV
MAIN_LIBS += $(top_builddir)/server/srv/libsrv.sys.la
endif

if ENABLE_PVFS
MAIN_LIBS += $(top_builddir)/server/pvfs/libpvfs.sys.la
endif

if ENABLE_NPFS
MAIN_LIBS += $(top_builddir)/server/npfs/libnpfs.sys.la
endif

endif

liblwiomain_la_SOURCES = \
    globals.c   \
    libmain.c   \
    cfg.c    \
    ipc.c       \
    proc.c

liblwiomain_la_CPPFLAGS = $(MAIN_CPPFLAGS)

liblwiomain_la_LIBADD = $(MAIN_LIBS)

sbin_PROGRAMS = lwiod

lwiod_SOURCES = main.c

lwiod_CPPFLAGS = $(MAIN_CPPFLAGS)

lwiod_LDADD = liblwiomain.la

if !ENABLE_SMALL_DAEMON
lwiod_LDADD += $(MAIN_LIBS)
endif

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(lwiocachedir)

