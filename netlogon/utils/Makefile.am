AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES=liblwnetutils.la

liblwnetutils_la_SOURCES = \
        globals.c           \
	lwnet-cfg.c         \
	lwnet-dlinkedlist.c \
	lwnet-dns.c         \
	lwnet-futils.c      \
	lwnet-info.c        \
	lwnet-logger.c      \
	lwnet-mem.c         \
	lwnet-paths.c       \
	lwnet-stack.c       \
	lwnet-time.c        \
	lwnet-sysfuncs.c

noinst_HEADERS = \
        externs.h       \
        includes.h      \
	lwnet-dns_p.h

liblwnetutils_la_LDFLAGS =  \
	@LIBUNISTR_LIBS@    \
	@RESOLV_LIBS@       \
	@RT_LIBS@           \
	@PTHREAD_LIBS@ 

liblwnetutils_la_LIBADD =  \
    @LWREG_LIBS@

