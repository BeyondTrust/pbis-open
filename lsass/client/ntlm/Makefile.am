lib_LTLIBRARIES = liblsaclient_ntlm.la

liblsaclient_ntlm_la_SOURCES = \
    acceptsecctxt.c \
    acquirecreds.c \
    clientipc.c \
    decryptmsg.c \
    delsecctxt.c \
    encryptmsg.c \
    exportsecctxt.c \
    freecreds.c \
    globals.c \
    importsecctxt.c \
    initsecctxt.c \
    makesign.c \
    querycreds.c \
    queryctxt.c \
    setcreds.c \
    verifysign.c

liblsaclient_ntlm_la_CFLAGS = \
    $(AM_CFLAGS) \
    $(LWADVAPI_INCLUDES) \
    @LWMSG_INCLUDES@

liblsaclient_ntlm_la_LDFLAGS = \
    $(AM_LDFLAGS) \
    $(LWADVAPI_LDFLAGS)

liblsaclient_ntlm_la_LIBADD = \
    @top_builddir@/common/liblsacommon.la \
    @LWMSG_LIBS@ \
    @PTHREAD_LIBS@ \
    @SOCKET_LIBS@
