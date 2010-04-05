bin_PROGRAMS = gssclient

gssclient_CFLAGS = \
       $(AM_CFLAGS) \
       $(LIBUNISTR_INCLUDES) \
       $(KRB5_INCLUDES) \
       $(LWBASE_INCLUDES)

gssclient_SOURCES = \
       gss-client.c

gssclient_LDFLAGS = \
       $(LIBUNISTR_LDFLAGS) \
       $(KRB5_LDFLAGS)

gssclient_LDADD = \
       $(KRB5_LIBS) \
       $(LIBUNISTR_LIBS)
