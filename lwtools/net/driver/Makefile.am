bin_PROGRAMS = lwnet

lwnet_SOURCES = \
    main.c

lwnet_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/net/include

lwnet_LDADD = \
    $(top_builddir)/net/file/liblwutilnetfile.la \
    $(top_builddir)/net/session/liblwutilnetsession.la \
    $(top_builddir)/net/share/liblwutilnetshare.la \
    $(top_builddir)/net/common/liblwnetutils.la \
    @LWNETAPI_LIBS@ \
    @LWADVAPI_LIBS@
    
