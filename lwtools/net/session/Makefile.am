noinst_LTLIBRARIES = liblwutilnetsession.la

liblwutilnetsession_la_SOURCES = \
    libmain.c \
    netsession.c
    
liblwutilnetsession_la_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/net/include \
    @LWNETAPI_INCLUDES@ \
    @LWADVAPI_INCLUDES@ \
    $(AM_CPPFLAGS)
    
liblwutilnetsession_la_LIBADD = -llwnetapi
