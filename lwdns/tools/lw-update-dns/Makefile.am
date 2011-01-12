AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

bin_PROGRAMS = lw-update-dns

lw_update_dns_SOURCES = \
    globals.c   \
    lwkrb5.c    \
    main.c

lw_update_dns_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    @NETLOGON_INCLUDES@

lw_update_dns_LDADD = \
    @top_builddir@/lwdns/liblwdns.la \
    @NETLOGON_LIBS@   \
    @KRB5_LIBS@ \
    -llwbase_nothr \
    -llsaclient

lw_update_dns_LDFLAGS = \
	@NETLOGON_LDFLAGS@
