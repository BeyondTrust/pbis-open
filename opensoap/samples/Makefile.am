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
	Base \
	Hello \
	Calc \
	CalcAsync \
	ShoppingSec \
	Security \
	SecCertAuth \
	Transaction \
	SimpleCalc \
	HelloSec \
	Echo

EXTRA_DIST = \
	README.TXT \
	READMESJ.TXT

# compile and install after API libraries are installed.
SAMPLE_MAK = \
	Calc/Calc.mak \
	CalcAsync/CalcAsync.mak \
	Hello/Hello.mak \
	HelloSec/HelloSec.mak \
	SecCertAuth/GetCert/GetCert.mak \
	SecCertAuth/Regist/Regist.mak \
	Security/Security.mak \
	ShoppingSec/Shopping.mak \
	ShoppingSec/ShoppingSec.mak \
	SimpleCalc/SimpleCalc.mak \
	Transaction/Transaction.mak

all-samples:
	for d in ${SAMPLE_MAK}; do \
		( cd `dirname $$d` ; \
		${MAKE} ${AM_MAKEFLAGS} -f `basename $$d` ) \
	done

clean-samples:
	for d in ${SAMPLE_MAK}; do \
		( cd `dirname $$d` ; \
		${MAKE} ${AM_MAKEFLAGS} -f `basename $$d` clean ) \
	done

install-samples-mak:
	for d in ${SAMPLE_MAK}; do \
		( cd `dirname $$d` ; \
		${MAKE} ${AM_MAKEFLAGS} -f `basename $$d` install-samples ) \
	done

uninstall-samples-mak:
	for d in ${SAMPLE_MAK}; do \
		( cd `dirname $$d` ; \
		${MAKE} ${AM_MAKEFLAGS} -f `basename $$d` uninstall ) \
	done

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
