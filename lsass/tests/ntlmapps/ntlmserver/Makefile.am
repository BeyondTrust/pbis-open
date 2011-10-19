testbin_PROGRAMS = ntlm-server

ntlm_server_CFLAGS = \
    $(AM_CFLAGS) \
    $(LWBASE_INCLUDES)
    
ntlm_server_SOURCES = \
    globals.c \
    ntlm-server.c
       
ntlm_server_LDADD = \
    $(top_builddir)/client/ntlm/liblsaclient_ntlm.la  
