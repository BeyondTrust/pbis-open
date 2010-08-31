AM_CFLAGS = $(LWBASE_CFLAGS) -Wno-deprecated-declarations
AM_CPPFLAGS = $(LWBASE_CPPFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(LWBASE_LDFLAGS)

lib_LTLIBRARIES = liblwbase_nothr.la liblwbase.la

liblwbase_nothr_la_SOURCES = \
	error-conv.c \
	errno.c \
	ntstatus.c \
	dscache.c \
	rpcstatus.c \
        rtlmemory.c \
        rtlstring.c \
        rtlstring_cstring.c \
        rtlstring_wc16string.c \
        rtlstring_unicode.c \
        rtlstring_ansi.c \
        rbtree.c \
        security-sid.c \
        sddlsidstring.c \
        security-sddl.c \
        security-acl.c \
        security-sd.c \
	wc16str.c \
	wc16printf.c

liblwbase_nothr_la_LIBADD = \
        @DS_FRAMEWORK_LDFLAGS@ \
        @LIBUUID_LIBS@ \
	@LIB_ICONV@

liblwbase_la_SOURCES = \
	atomic.c \
        security-token.c \
        security-sd-inherit.c \
	threadpool-common.c \
	mapsecurity.c

liblwbase_la_LIBADD = \
        @LIB_PTHREAD@ \
	@LIB_DL@ \
        liblwbase_nothr.la

if USE_EPOLL
liblwbase_la_SOURCES += threadpool-epoll.c
endif

if USE_KQUEUE
liblwbase_la_SOURCES += threadpool-kqueue.c
endif

if USE_SELECT
liblwbase_la_SOURCES += threadpool-select.c
endif
