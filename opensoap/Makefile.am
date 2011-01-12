# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
# 

SUBDIRS = src include contrib

AUX_DIST	=	$(ac_aux_dir)/config.guess \
			$(ac_aux_dir)/config.sub \
			$(ac_aux_dir)/install-sh \
			$(ac_aux_dir)/ltconfig \
			$(ac_aux_dir)/ltmain.sh \
			$(ac_aux_dir)/missing \
			$(ac_aux_dir)/mkinstalldirs \
			$(ac_aux_dir)/depcomp

EXTRA_DIST	=	LICENSE \
			INSTALL.ujis \
			HISTORY \
			HISTORY.ujis \
			README.ujis \
			TODO.ujis \
			RELEASE-2.0.ujis \
			RELEASE-2.0 \
			mkdeffile.sh.in \
			uninstall-olddir.sh \
			opensoap.spec \
			bootstrap

MAINTAINERCLEANFILES	=	\
				Makefile.in \
				aclocal.m4 \
				configure \
				config.h.in \
				stamp-h.in \
				$(AUX_DIST)

dist-hook:
	(cd $(distdir) && (test -d $(ac_aux_dir) || mkdir $(ac_aux_dir)))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  test ! -f $$file || cp $$file $(distdir)/$$file; \
	done

#
#
#
api: 
	(cd src/api && $(MAKE) $(AM_MAKEFLAGS) )

security: api
	(cd src/security && $(MAKE) $(AM_MAKEFLAGS) )

tools: api
	(cd src/tools && $(MAKE) $(AM_MAKEFLAGS) )

#
src/api/libOpenSOAPClient.la src/api/libOpenSOAPService.la : api
src/security/libOpenSOAPSecurity.la: security
src/tools/Transaction/Transaction.cgi: tools

#
install-api: src/api/libOpenSOAPClient.la src/api/libOpenSOAPService.la
	(cd src/api && $(MAKE) $(AM_MAKEFLAGS) install)

install-api-dev:
	(cd include && $(MAKE) $(AM_MAKEFLAGS) install)

install-security: src/security/libOpenSOAPSecurity.la
	(cd src/security && $(MAKE) $(AM_MAKEFLAGS) install)

install-security-dev: 

install-tools: src/tools/Transaction/Transaction.cgi
	(cd src/tools && $(MAKE) $(AM_MAKEFLAGS) install)
