noinst_LTLIBRARIES = liblwnetutils.la

liblwnetutils_la_SOURCES =       \
    memory.c \
    utils.c
    
liblwnetutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/net/include \
    $(LWBASE_INCLUDES) \
    $(LWADVAPI_INCLUDES)

liblwnetutils_la_LIBADD = \
    $(RESOLV_LIBS) \
    $(NSL_LIBS) \
    $(LWBASE_NOTHR_LIBS) \
    $(LWADVAPI_NOTHR_LIBS) \
    $(LWMSG_NOTHR_LIBS)
