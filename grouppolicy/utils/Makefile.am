lib_LTLIBRARIES = libgputils.la

libgputils_la_SOURCES = \
    gpadns.c      \
    gpafilexfer.c \
    gpaio.c       \
    gpalogger.c   \
    gpamessage.c  \
    gpxnet.c      \
    gpacfg.c      \
    gpaarray.c    \
    gpashell.c    \
    gpaexec.c    \
    gpacfgparser.c    \
    gpaprocutils.c    \
    gpautils.c    \
    gpamissing.c    \
    gpalist.c    \
    gpahashtable.c    \
    gpahash.c    \
    sysfuncs.c

libgputils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    @PSTORE_INCLUDES@\
    @LSASS_INCLUDES@\
    @LWREG_INCLUDES@\
    @LWADVAPI_INCLUDES@

libgputils_la_LIBADD = \
    @CENTUTILS_LIBS@ \
    @PSTORE_LIBS@ \
    @RESOLV_LIBS@    \
    @NSL_LIBS@ \
    @RT_LIBS@ \
    @LWREG_LIBS@\
    @LSASS_LIBS@\
    @LWADVAPI_LIBS@

libgputils_la_LDFLAGS = \
    @CENTUTILS_LDFLAGS@ \
    @LSASS_LDFLAGS@\
    @PSTORE_LDFLAGS@

