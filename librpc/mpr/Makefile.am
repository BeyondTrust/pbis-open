librpc_srcdir = $(top_srcdir)

lib_LTLIBRARIES = libmpr.la

libmpr_la_SOURCES =         \
    wnet_addconnection2.c   \
    wnet_cancelconnection2.c

libmpr_la_CPPFLAGS =           \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(LWADVAPI_INCLUDES)       \
    $(LIBUNISTR_INCLUDES)      \
    -I$(librpc_srcdir)/include

libmpr_la_CFLAGS =             \
    $(AM_CFLAGS)

libmpr_la_LDFLAGS =       \
    $(AM_LDFLAGS)         \
    $(LWBASE_LDFLAGS)     \
    $(LWADVAPI_LDFLAGS)   \
    $(LIBUNISTR_LDFLAGS)

libmpr_la_LIBADD =      \
    $(LWBASE_LIBS)      \
    $(LWADVAPI_LIBS)    \
    $(LIBUNISTR_LIBS)
