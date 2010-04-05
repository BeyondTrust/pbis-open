noinst_LTLIBRARIES = liblwadvapi_error.la

liblwadvapi_error_la_SOURCES = \
    error.c                    \
    error-conv.c
    
liblwadvapi_error_la_CPPFLAGS = -I$(top_srcdir)/include \
                                -I$(top_srcdir)/include/public \
                                $(LWBASE_INCLUDES)
                                
liblwadvapi_error_la_LIBADD = $(LWBASE_NOTHR_LIBS)

liblwadvapi_error_la_LDFLAGS = $(LWBASE_LDFLAGS)
