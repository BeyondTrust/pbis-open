# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
##
##AUTOMAKE_OPTIONS=no-dependencies

noinst_PROGRAMS = genrsakey secEnv

INCLUDES = -I$(top_builddir) -I$(top_builddir)/include

genrsakey_SOURCES = \
	genkey.c
genrsakey_LDADD = \
	$(top_builddir)/src/api/libOpenSOAPClient.la \
	$(top_builddir)/src/security/libOpenSOAPSecurity.la

secEnv_SOURCES = \
	envelopeTst.c fio.c tstcmn.h
secEnv_LDADD = \
	$(top_builddir)/src/api/libOpenSOAPClient.la \
	$(top_builddir)/src/security/libOpenSOAPSecurity.la

EXTRA_DIST = \
	Security.mak.in \
	decrypt.xml \
	dsig.xml \
	encrypt.xml \
	env_noenc.xml \
	envelope.xml \
	privKey_1.pem \
	privKey_2.pem \
	privKey_z.pem \
	pubKey_1.pem \
	pubKey_2.pem \
	pubKey_z.pem
