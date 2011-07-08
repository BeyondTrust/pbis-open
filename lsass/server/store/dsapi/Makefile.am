AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
        $(top_builddir)/logging_r/liblsalog_r.la

if ENABLE_MINIMAL
noinst_LTLIBRARIES = libdsapi.la
if !ENABLE_SMALL_DAEMON
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif
else
provider_LTLIBRARIES = libdsapi.la
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif

libdsapi_la_SOURCES = \
        diradd.c      \
        dirattr.c     \
        dirbind.c     \
        dirclose.c    \
        dirdel.c      \
        dirglobals.c  \
        dirgroup.c    \
        dirmem.c      \
        dirmodify.c   \
        diropen.c     \
        dirprovider.c \
        dirsearch.c   \
        diruser.c

libdsapi_la_CPPFLAGS =                        \
	$(AM_CPPFLAGS)                        \
        -DLW_ENABLE_THREADS=1                 \
        $(LWBASE_INCLUDES)                    \
        -I$(top_srcdir)/server/include        \
        -I$(top_srcdir)/server/store/include

libdsapi_la_LIBADD =                             \
        $(LWBASE_LIBS)                           \
        $(ADDITIONAL_LIBS)                       \
        $(DL_LIBS)                               \
        $(top_builddir)/common/liblsacommon.la   \
        -lsqlite3

libdsapi_la_LDFLAGS =                            \
        $(LWBASE_LDFLAGS)
