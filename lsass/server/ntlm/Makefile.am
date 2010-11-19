AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

lib_LTLIBRARIES = libntlmserver.la

libntlmserver_la_SOURCES = \
    acceptsecctxt.c \
    acquirecreds.c \
    context.c \
    crc32.c \
    credentials.c \
    decryptmsg.c \
    delsecctxt.c \
    encryptmsg.c \
    exportsecctxt.c \
    freecreds.c \
    globals.c \
    importsecctxt.c \
    initsecctxt.c \
    ipc_dispatch.c \
    makesign.c \
    querycreds.c \
    queryctxt.c \
    setcreds.c \
    cfg.c \
    verifysign.c

libntlmserver_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    $(LWADVAPI_INCLUDES) \
    -I$(top_srcdir)/server/include \
    $(LWMSG_INCLUDES) \
    $(PSTORE_INCLUDES) \
    $(KRB5_INCLUDES) \
    $(CRYPTO_INCLUDES)

libntlmserver_la_LDFLAGS = \
    $(AM_LDFLAGS) \
    $(PSTORE_LDFLAGS) \
    $(KRB5_LDFFLAGS) \
    $(CRYPTO_LDFLAGS)

libntlmserver_la_LIBADD = \
    $(top_builddir)/common/liblsacommon.la \
    $(top_builddir)/client/rpc/liblsarpc.la \
    $(LWMSG_NOTHR_LIBS) \
    $(PSTORE_LIBS) \
    $(CRYPTO_LIBS) \
    $(KRB5_LIBS) \
    $(LWADVAPI_LIBS) \
    $(SOCKET_LIBS)
