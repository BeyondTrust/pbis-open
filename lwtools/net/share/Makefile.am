noinst_LTLIBRARIES = liblwutilnetshare.la

liblwutilnetshare_la_SOURCES = \
    netshare.c \
    libmain.c
    
liblwutilnetshare_la_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/net/include \
    @LWNETAPI_INCLUDES@ \
    @LWADVAPI_INCLUDES@ \
    $(AM_CPPFLAGS)
    
liblwutilnetshare_la_LIBADD = -llsaclient
