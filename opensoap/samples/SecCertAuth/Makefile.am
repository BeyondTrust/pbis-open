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

SUBDIRS = \
	GetCert \
	Regist

install-samples:
	for d in ${SUBDIRS}; do \
		( cd $$d ; \
		${MAKE} ${AM_MAKEFLAGS} install-samples ) \
	done

uninstall-samples:
	for d in ${SUBDIRS}; do \
		( cd $$d ; \
		${MAKE} ${AM_MAKEFLAGS} uninstall-samples ) \
	done
