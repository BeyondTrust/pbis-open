noinst_LTLIBRARIES = liblwtaskipc.la

liblwtaskipc_la_SOURCES = \
    globals.c \
    libmain.c

liblwtaskipc_la_CPPFLAGS = \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

liblwtaskipc_la_LIBADD = \
    @LWMSG_NOTHR_LIBS@