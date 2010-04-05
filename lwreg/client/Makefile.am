lib_LTLIBRARIES = libregclient.la

libregclient_la_SOURCES = \
    clientipc.c \
    regclient.c \
    regntclient.c

libregclient_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    @LWBASE_INCLUDES@    \
    @LWMSG_INCLUDES@

libregclient_la_LIBADD = \
	@top_builddir@/utils/libregutils.la \
	@top_builddir@/ipc/libregipc.la \
	@LWBASE_NOTHR_LIBS@ \
	@LWMSG_LIBS@
