lib_LTLIBRARIES = libcentutils.la

AM_CPPFLAGS += -I@top_srcdir@/include

libcentutils_la_SOURCES = \
    ctarray.c      \
    ctcfgparser.c  \
    cterr.c        \
    ctexec.c       \
    ctfileutils.c  \
    ctgroup.c      \
    cthash.c       \
    ctmem.c        \
    ctprocutils.c  \
    ctrwlock.c     \
    ctshell.c      \
    ctstrutils.c   \
    cttext.c       \
    ctuser.c       \
    lwexc.c        \
    sysfuncs.c

libcentutils_la_LIBADD = \
    @top_builddir@/libgpglib/libgpglib.la \
    @PROC_LIBS@ \
    @KVM_LIBS@  \
    @RT_LIBS@
