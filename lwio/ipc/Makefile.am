SUBDIRS = .

noinst_LTLIBRARIES = liblwioipc.la

liblwioipc_la_SOURCES = \
    globals.c \
    libmain.c \
    ntipc.c \
    ntipc-unicode_string.c

liblwioipc_la_CPPFLAGS = \
    -I@top_srcdir@/include  \
    @LWMSG_INCLUDES@

liblwioipc_la_LIBADD = \
    @LWMSG_NOTHR_LIBS@