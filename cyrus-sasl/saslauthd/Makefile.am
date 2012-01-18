AUTOMAKE_OPTIONS = 1.7
sbin_PROGRAMS	= saslauthd testsaslauthd
EXTRA_PROGRAMS  = saslcache

saslauthd_SOURCES = mechanisms.c globals.h \
		    mechanisms.h auth_dce.c auth_dce.h auth_getpwent.c \
		    auth_getpwent.h auth_krb5.c auth_krb5.h auth_krb4.c \
		    auth_krb4.h auth_pam.c auth_pam.h auth_rimap.c auth_httpform.c \
		    auth_rimap.h auth_shadow.c auth_shadow.h auth_sia.c auth_httpform.h \
		    auth_sia.h auth_sasldb.c auth_sasldb.h lak.c lak.h \
		    auth_ldap.c auth_ldap.h cache.c cache.h cfile.c cfile.h \
		    krbtf.c krbtf.h utils.c utils.h \
                    ipc_unix.c ipc_doors.c saslauthd-main.c saslauthd-main.h \
		    md5.c saslauthd_md5.h md5global.h 
EXTRA_saslauthd_sources = getaddrinfo.c getnameinfo.c
saslauthd_DEPENDENCIES = saslauthd-main.o @LTLIBOBJS@
saslauthd_LDADD	= @SASL_KRB_LIB@ \
		  @GSSAPIBASE_LIBS@ @GSSAPI_LIBS@ @LIB_CRYPT@ @LIB_SIA@ \
		  @LIB_SOCKET@ @SASL_DB_LIB@ @LIB_PAM@ @LDAP_LIBS@ @LTLIBOBJS@

testsaslauthd_SOURCES = testsaslauthd.c utils.c
testsaslauthd_LDADD = @LIB_SOCKET@

saslcache_SOURCES = saslcache.c

EXTRA_DIST	= saslauthd.8 saslauthd.mdoc config include \
		  getnameinfo.c getaddrinfo.c LDAP_SASLAUTHD
INCLUDES	= -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/../include
DEFS            = @DEFS@ -DSASLAUTHD_CONF_FILE_DEFAULT=\"@sysconfdir@/saslauthd.conf\" -I. -I$(srcdir) -I..


dist-hook: saslauthd.8

saslauthd.8: saslauthd.mdoc
	nroff -mdoc $(srcdir)/saslauthd.mdoc > $(srcdir)/saslauthd.8

install-data-local: saslauthd.8
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man8
	$(INSTALL_DATA) $(srcdir)/saslauthd.8 $(DESTDIR)$(mandir)/man8/saslauthd.8
