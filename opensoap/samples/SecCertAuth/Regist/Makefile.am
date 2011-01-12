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

noinst_PROGRAMS = registCA

EXTRA_DIST = \
	README.TXT \
	READMESJ.TXT \
	Regist.mak.in \
	regist.txt

INCLUDES = -I$(top_builddir) -I$(top_builddir)/include

registCA_SOURCES = regist.c
registCA_LDADD = \
	$(top_builddir)/src/api/libOpenSOAPClient.la \
	$(top_builddir)/src/security/libOpenSOAPSecurity.la

install-samples: ${noinst_PROGRAMS}
	@mkdir -p ${DESTDIR}${bindir}; \
	for f in $^ ; do \
		echo "$$f install to ${DESTDIR}${bindir}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${bindir}; \
	done
