AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libeventlogutils.la

libeventlogutils_la_SOURCES = \
    evtcfg.c    \
    evtfutils.c \
    evthost.c   \
    evtlogger.c \
    evtmem.c    \
    evtrpcmem.c \
    evtstack.c  \
    evtstr.c    \
    globals.c   \
    sysfuncs.c

libeventlogutils_la_LIBADD = \
    @DCERPC_LIBS@    \
    @LWREG_LIBS@

libeventlogutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    @DCERPC_INCLUDES@

libeventlogutils_la_LDFLAGS = \
    @DCERPC_LDFLAGS@
