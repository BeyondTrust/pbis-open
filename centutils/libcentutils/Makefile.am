lib_LTLIBRARIES = libcentutils.la

AM_CPPFLAGS += -I$(top_srcdir)/include \
	       $(LWADVAPI_INCLUDES)

libcentutils_la_SOURCES = \
    ctarray.c      \
    ctexec.c       \
    ctfileutils.c  \
    cthash.c       \
    ctmem.c        \
    ctprocutils.c  \
    ctrwlock.c     \
    ctshell.c      \
    ctstrutils.c   \
    cttext.c       \
    ctuser.c       \
    lwexc.c        \
    ctsysfuncs.c

libcentutils_la_LIBADD = \
    $(top_builddir)/libgpglib/libgpglib.la \
    $(PROC_LIBS) \
    $(KVM_LIBS)  \
    $(RT_LIBS) \
    $(LWADVAPI_NOTHR_LIBS)

libcentutils_la_LDFLAGS = $(LWADVAPI_LDFLAGS)
