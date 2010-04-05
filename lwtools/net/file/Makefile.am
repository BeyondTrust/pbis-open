noinst_LTLIBRARIES = liblwutilnetfile.la

liblwutilnetfile_la_SOURCES = \
    libmain.c \
    netfile.c
    
liblwutilnetfile_la_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/net/include \
    @LWNETAPI_INCLUDES@ \
    @LWADVAPI_INCLUDES@ \
    $(AM_CPPFLAGS)
    
liblwutilnetfile_la_LIBADD = -llwnetapi
