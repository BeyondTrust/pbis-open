bin_PROGRAMS = domainjoin2-cli

domainjoin2_cli_SOURCES = \
  join.c

domainjoin2_cli_join.o: join.c
domainjoin2_cli_LDADD = \
    $(AM_LDFLAGS) \
    @LSACLIENT_LIBS@ \
    @LWNET_CLIENT_LIBS@

domainjoin2_cli_CPPFLAGS =
    $(AM_CPPFLAGS)
