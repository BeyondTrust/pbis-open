bin_PROGRAMS = gssserver

gssserver_CFLAGS = \
       $(AM_CFLAGS) \
       $(LIBUNISTR_INCLUDES) \
       $(KRB5_INCLUDES)

gssserver_SOURCES = \
       gss-server.c

gssserver_LDFLAGS = \
       $(LIBUNISTR_LDFLAGS) \
       $(KRB5_LDFLAGS)

gssserver_LDADD = \
       $(KRB5_LIBS) \
       $(LIBUNISTR_LIBS)
