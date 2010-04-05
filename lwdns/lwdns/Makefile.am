AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = liblwdns.la

liblwdns_la_SOURCES =  \
    dnsdlinkedlist.c \
    dnserror.c       \
    dnsgss.c         \
    dnsinterfaces.c  \
    dnslogger.c      \
    dnsparser.c      \
    dnsrecord.c      \
    dnsrequest.c     \
    dnsresponse.c    \
    dnssign.c        \
    dnssock.c        \
    dnsstrerror.c    \
    dnsupdate.c      \
    dnsuprequest.c   \
    dnsupresp.c      \
    dnsutils.c       \
    globals.c        \
    libmain.c

liblwdns_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    @KRB5_INCLUDES@

liblwdns_la_LIBADD = \
    @KRB5_LIBS@      \
    @UUID_LIBS@      \
    @DL_LIBS@        \
    @RESOLV_LIBS@    \
    @SOCKET_LIBS@    \
    @NSL_LIBS@       \
    @PTHREAD_LIBS@   \
    $(LWBASE_LIBS)

liblwdns_la_LDFLAGS = \
    @KRB5_LDFLAGS@   \
    $(LWBASE_LDFLAGS)

