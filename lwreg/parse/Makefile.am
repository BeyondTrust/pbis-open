lib_LTLIBRARIES = libregparse.la

libregparse_la_SOURCES = \
    regio.c \
    reglex.c \
    regiconv.c \
    regparse.c

libregparse_la_CPPFLAGS = \
    $(AM_CPPFLAGS)        \
    -I@top_srcdir@/server/include \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@

libregparse_la_LIBADD = \
        @top_builddir@/utils/libregutils.la \
        @LWBASE_NOTHR_LIBS@ \
        @LWMSG_NOTHR_LIBS@

libregparse_la_LDFLAGS = @LWBASE_LDFLAGS@
