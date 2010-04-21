bin_PROGRAMS = gssserver

gssserver_CFLAGS = \
       $(AM_CFLAGS) \
       $(KRB5_INCLUDES)

gssserver_SOURCES = \
       gss-server.c

gssserver_LDFLAGS = \
       $(LWBASE_LDFLAGS) \
       $(KRB5_LDFLAGS)

gssserver_LDADD = \
       $(LWBASE_NOTHR_LIBS) \
       $(KRB5_LIBS)
