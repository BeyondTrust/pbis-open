testbin_PROGRAMS = ntlm-client

ntlm_client_CFLAGS = \
    $(AM_CFLAGS) \
    $(LWBASE_INCLUDES)
    
ntlm_client_SOURCES = \
    globals.c \
    ntlmclient.c

ntlm_client_LDADD = \
    $(top_builddir)/client/ntlm/liblsaclient_ntlm.la
