lib_LTLIBRARIES = librsutils.la

librsutils_la_SOURCES = \
    regutilwrap.c \
    rsutils.c \
    import.c \
    export.c

librsutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS)      \
    @LWBASE_INCLUDES@   \
    @LWMSG_INCLUDES@

librsutils_la_LIBADD = \
    $(top_builddir)/client/libregclient.la
