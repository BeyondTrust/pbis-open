lib_LTLIBRARIES = liblwnetapi.la

liblwnetapi_la_SOURCES = \
    libmain.c \
    srvsvc.c

liblwnetapi_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/include \
    -I$(SRVSVC_INCLUDES)

liblwnetapi_la_CFLAGS = \
    $(AM_CFLAGS) \
    -Wmissing-prototypes
    
liblwnetapi_la_LIBADD = \
    $(SRVSVC_LIBS) \
    $(LWADVAPI_LIBS)

