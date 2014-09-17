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

SERVICE_NAME = ShoppingSec

SSML_IN_FILES = ShoppingSec.ssml.in Shopping.ssml.in
#
KEYCONF_IN_FILES = Shopping.keys.in

MAK_IN_FILES = \
		ShoppingSec.mak.in \
		Shopping.mak.in

#
DOCUMENT_FILES = \
	README.TXT \
	READMESJ.TXT

CLIENT_HTML_DIR = ShoppingHtml

CLIENT_HTML_FILES = \
	${CLIENT_HTML_DIR}/CommonFtr.html \
	${CLIENT_HTML_DIR}/CommonHdr.html \
	${CLIENT_HTML_DIR}/ErrorFtr.html \
	${CLIENT_HTML_DIR}/ErrorHdr.html \
	${CLIENT_HTML_DIR}/GetProductList.html \
	${CLIENT_HTML_DIR}/GetProductListFtr.html \
	${CLIENT_HTML_DIR}/GetProductListHdr.html \
	${CLIENT_HTML_DIR}/GetStockQtyErr.html \
	${CLIENT_HTML_DIR}/GetStockQtyFtr.html \
	${CLIENT_HTML_DIR}/GetStockQtyHdr.html \
	${CLIENT_HTML_DIR}/OrderResultErr.html \
	${CLIENT_HTML_DIR}/OrderResultFtr.html \
	${CLIENT_HTML_DIR}/OrderResultHdr.html \
	${CLIENT_HTML_DIR}/PlaceOrderErr.html \
	${CLIENT_HTML_DIR}/PlaceOrderFtr.html \
	${CLIENT_HTML_DIR}/PlaceOrderHdr.html

SAMPLE_MESSAGE_DIR = SampleMessages

SAMPLE_MESSAGE_FILES = \
	${SAMPLE_MESSAGE_DIR}/GetProductCountReq.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductCountReqF1.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductCountRes.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductCountSecReq.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductSpecReq.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductSpecReqF1.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductSpecReqF2.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductSpecReqF3.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductSpecRes.xml \
	${SAMPLE_MESSAGE_DIR}/GetProductSpecSecReq.xml \
	${SAMPLE_MESSAGE_DIR}/GetStockQtyReq.xml \
	${SAMPLE_MESSAGE_DIR}/GetStockQtyReqF1.xml \
	${SAMPLE_MESSAGE_DIR}/GetStockQtyReqF2.xml \
	${SAMPLE_MESSAGE_DIR}/GetStockQtyReqF3.xml \
	${SAMPLE_MESSAGE_DIR}/GetStockQtyRes.xml \
	${SAMPLE_MESSAGE_DIR}/GetStockQtySecReq.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderReq.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderReqF1.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderReqF2.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderReqF3.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderReqF4.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderReqF5.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderRes.xml \
	${SAMPLE_MESSAGE_DIR}/PlaceOrderSecReq.xml

cgi_client_USEKEYFILES = \
	 	pubKey_ShoppingService.pem \
		privKey_ShoppingClient.pem
service_USEKEYFILES = \
	 	privKey_ShoppingService.pem \
		pubKey_ShoppingClient.pem
service_CONFFILES = Shopping.conf
service_DATAFILES = ProductSpec.data ProductStock.data

ShoppingClient_SOURCEFILES = \
	ClCmn.c ClCmn.h \
	ShClCmn.c ShClCmn.h \
	ShClProductList.c ShClProductList.h \
	ShClGetProductCount.c ShClGetProductCount.h \
	ShClGetProductSpec.c ShClGetProductSpec.h \
	ShClGetStockQty.c ShClGetStockQty.h \
	ShClPlaceOrderCmn.c ShClPlaceOrderCmn.h \
	ShClPlaceOrderSync.c ShClPlaceOrderSync.h

ShoppingClient_SOURCES = \
	${ShoppingClient_SOURCEFILES} \
	ShClMainSync.c ShClMainSync.h

ShoppingClientAsync_SOURCES = \
	${ShoppingClient_SOURCEFILES} \
	ClAsyncCmn.c ClAsyncCmn.h \
	ShClPlaceOrderAsync.c ShClPlaceOrderAsync.h \
	ShClMainAsync.c ShClMainAsync.h

ShoppingService_SOURCEFILES = \
	SvCmn.c SvCmn.h \
	ShSvCmn.c ShSvCmn.h \
	ShSvGetProductCount.c ShSvGetProductCount.h \
	ShSvGetProductSpec.c ShSvGetProductSpec.h \
	ShSvGetStockQty.c ShSvGetStockQty.h \
	ShSvPlaceOrder.c ShSvPlaceOrder.h

ShoppingService_SOURCES = \
	${ShoppingService_SOURCEFILES} \
	ShSvMain.c ShSvMain.h

ShoppingService_cgi_SOURCES = \
	${ShoppingService_SOURCEFILES}

ShoppingClientSec_SOURCEFILES = \
	ClCmn.c ClCmn.h \
	ClSecCmn.c ClSecCmn.h \
	ShClCmn.c ShClCmn.h \
	ShClProductList.c ShClProductList.h \
	ShClGetProductCount.c ShClGetProductCount.h \
	ShClGetProductSpec.c ShClGetProductSpec.h \
	ShClGetStockQty.c ShClGetStockQty.h \
	ShClPlaceOrderCmn.c ShClPlaceOrderCmn.h \
	ShClPlaceOrderSec.c ShClPlaceOrderSec.h

ShoppingClientSec_SOURCES = \
	${ShoppingClientSec_SOURCEFILES} \
	ShClMainSec.c ShClMainSec.h

ShoppingClientSec_cgi_SOURCES = \
	${ShoppingClientSec_SOURCEFILES} \
	ShClMainSecWeb.c ShClMainSecWeb.h

ShoppingServiceSec_SOURCEFILES = \
	SvCmn.c SvCmn.h \
	SvSecCmn.c SvSecCmn.h \
	ShSvCmn.c ShSvCmn.h \
	ShSvGetProductCount.c ShSvGetProductCount.h \
	ShSvGetProductSpec.c ShSvGetProductSpec.h \
	ShSvGetStockQty.c ShSvGetStockQty.h \
	ShSvPlaceOrderSec.c ShSvPlaceOrderSec.h

ShoppingServiceSec_SOURCES = \
	${ShoppingServiceSec_SOURCEFILES} \
	ShSvMainSec.c ShSvMainSec.h

ShoppingServiceSec_cgi_SOURCES = \
	${ShoppingServiceSec_SOURCEFILES}


EXTRA_DIST = \
	${SSML_IN_FILES} \
	${MAK_IN_FILES} \
	${DOCUMENT_FILES} \
	${service_DATAFILES} \
	${service_CONFFILES} \
	${service_USEKEYFILES} \
	${KEYCONF_IN_FILES} \
	${cgi_client_USEKEYFILES} \
	${SAMPLE_MESSAGE_FILES} \
	${CLIENT_HTML_FILES} \
	ShoppingServiceSecDataInstall.bat

# ssml create rule
SSML_FILES = ${SSML_IN_FILES:.in=}

${SSML_FILES}: ${SSML_IN_FILES} Makefile
	@for i in ${SSML_IN_FILES}; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done

KEYCONF_FILES = ${KEYCONF_IN_FILES:.in=}
noinst_DATA = ${SSML_FILES} ${KEYCONF_FILES}

${KEYCONF_FILES}: ${KEYCONF_IN_FILES} Makefile
	@for i in ${KEYCONF_IN_FILES}; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done

#### for installation
noinst_PROGRAMS = \
	ShoppingClient \
	ShoppingClientAsync \
	ShoppingService \
	ShoppingService.cgi \
	ShoppingServiceSec \
	ShoppingServiceSec.cgi \
	ShoppingClientSec \
	ShoppingClientSec.cgi
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

INCLUDES = -I${top_srcdir} -I${top_srcdir}/include

# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR = @sysconfdir@
OPENSOAP_LOCALSTATEDIR = @localstatedir@
OPENSOAP_SERVICE_LOCALSTATEDIR = ${OPENSOAP_LOCALSTATEDIR}/services
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

#
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
SERVICE_LOCALSTATEDIR = ${OPENSOAP_SERVICE_LOCALSTATEDIR}/${SERVICE_NAME}
AM_CFLAGS = -DSERVICE_DIR=\"${SERVICE_DIR}\" \
	-DSERVICE_LOCALSTATEDIR=\"${SERVICE_LOCALSTATEDIR}\"

#
SECURITY_LIBS = ${top_builddir}/src/security/libOpenSOAPSecurity.la
CLIENT_LIBS = ${top_builddir}/src/api/libOpenSOAPClient.la
SERVICE_LIBS = ${top_builddir}/src/api/libOpenSOAPService.la \
	${CLIENT_LIBS}

ShoppingClient_LDADD = ${CLIENT_LIBS}
ShoppingClientAsync_LDADD = ${CLIENT_LIBS}
ShoppingService_LDADD = ${SERVICE_LIBS}
ShoppingService_cgi_LDADD = ${SERVICE_LIBS} ShSvMain.cgi.o

ShoppingClientSec_LDADD = ${CLIENT_LIBS} ${SECURITY_LIBS}
ShoppingClientSec_cgi_LDADD = ${CLIENT_LIBS} ${SECURITY_LIBS}
ShoppingServiceSec_LDADD = ${SERVICE_LIBS} ${SECURITY_LIBS}
ShoppingServiceSec_cgi_LDADD = ShSvMainSec.cgi.o \
	${SECURITY_LIBS} ${SERVICE_LIBS}

ShSvMainSec.cgi.o: ShSvMainSec.c
	${COMPILE} -DCONNECT_TYPE_CGI -c -o $@ $^

ShSvMain.cgi.o: ShSvMain.c
	${COMPILE} -DCONNECT_TYPE_CGI -c -o $@ $^

cgi-install: ShoppingServiceSec.cgi ShoppingService.cgi
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}; \
	done

stdio-install: ShoppingServiceSec ShoppingService
#	@mkdir -m 777 -p ${DESTDIR}${SERVICE_LOCALSTATEDIR};
	@mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${SERVICE_DIR}; \
	done

cgi-client-data-install: ${CLIENT_HTML_FILES} ${cgi_client_USEKEYFILES}
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}/${CLIENT_HTML_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${INSTALL_PROGRAM} \
			-m 0644 \
			$$f ${DESTDIR}${CGI_BIN_DIR}/$$f; \
	done

cgi-client-install: ShoppingClientSec.cgi
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}; \
	done

client-install: ShoppingClientSec ShoppingClient ShoppingClientAsync
	@mkdir -p ${DESTDIR}${bindir}; \
	for f in $^ ; do \
		echo "$$f install to ${bindir}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${bindir}; \
	done

ssml-install: ${SSML_FILES}
	@mkdir -p ${DESTDIR}${SSML_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${SSML_DIR}."; \
		cp $$f ${DESTDIR}${SSML_DIR}; \
	done

service-conf-install: ${service_CONFFILES}
	if test -d ${DESTDIR}${SERVICE_DIR}; then \
		echo "${DESTDIR}${SERVICE_DIR} exist."; \
	else \
		echo "mkdir -p ${DESTDIR}${SERVICE_DIR}."; \
		mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	fi
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${INSTALL_PROGRAM} -m 0666 \
			$$f ${DESTDIR}${SERVICE_DIR}; \
		for k in ${cgi_client_USEKEYFILES}; do \
			echo "${CGI_BIN_DIR}/$$k" >> \
			${DESTDIR}${SERVICE_DIR}/$$f; \
		done; \
	done

service-datas-install: ${service_DATAFILES}
	if test -d ${DESTDIR}${SERVICE_LOCALSTATEDIR}; then \
		echo "${DESTDIR}${SERVICE_LOCALSTATEDIR} exist."; \
	else \
		echo "mkdir -m 777 -p ${DESTDIR}${SERVICE_LOCALSTATEDIR}."; \
		mkdir -m 777 -p ${DESTDIR}${SERVICE_LOCALSTATEDIR}; \
	fi
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_LOCALSTATEDIR}."; \
		${INSTALL_PROGRAM} \
			-m 0666 \
			$$f ${DESTDIR}${SERVICE_LOCALSTATEDIR}/$$f; \
	done

service-usekeys-install: ${service_USEKEYFILES} ${KEYCONF_FILES}
	if test -d ${DESTDIR}${SERVICE_DIR}; then \
		echo "${DESTDIR}${SERVICE_DIR} exist."; \
	else \
		echo "mkdir -p ${DESTDIR}${SERVICE_DIR}."; \
		mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	fi
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${INSTALL_PROGRAM} \
			-m 0644 \
			$$f ${DESTDIR}${SERVICE_DIR}/$$f; \
	done

service-common-install: service-conf-install service-datas-install service-usekeys-install

service-install: cgi-install stdio-install ssml-install service-common-install

install-samples: client-install service-install cgi-client-install cgi-client-data-install
