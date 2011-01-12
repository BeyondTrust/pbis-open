# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------

SUBDIRS = en ja

READMEFILES = \
	README.TXT \
	READMESJ.TXT

EXTRA_DIST = \
	${READMEFILES} \
	API_Ref.txt \
	API_Ref.sjis.txt

docs:
	@for d in ${SUBDIRS}; do (cd $${d} && ${MAKE} ${AM_MAKEFLAGS} $@ ); done
