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

#
SERVICE_NAME = TransactionABank

DOCUMENT_FILES = \
	README.TXT \
	READMESJ.TXT

TransactionClient_SOURCEFILES = TransactionClient.c TransactionClient.h
TransactionABankService_SOURCEFILES = TransactionABankService.c
TransactionApp_cgi_SOURCEFILES = TransactionApp.c

SOURCEFILES = \
	${TransactionClient_SOURCEFILES} \
	${TransactionABankService_SOURCEFILES} \
	${TransactionApp_cgi_SOURCEFILES}

SOAP_MESSAGE_FILES = 		\
	a_bank_commit.xml	\
	a_bank_request.xml 	\
	transaction_request.xml

SSML_IN_FILES = ${SERVICE_NAME}.ssml.in

MAK_IN_FILES = Transaction.mak.in

CLIENT_HTML_DIR = TransactionHtml

CLIENT_HTML_FILES = \
	${CLIENT_HTML_DIR}/AcceptedFooter.html \
	${CLIENT_HTML_DIR}/AcceptedHeader.html \
	${CLIENT_HTML_DIR}/CommonFooter.html \
	${CLIENT_HTML_DIR}/CommonHeader.html \
	${CLIENT_HTML_DIR}/ErrorFooter.html \
	${CLIENT_HTML_DIR}/ErrorHeader.html \
	${CLIENT_HTML_DIR}/RequestFooter.html \
	${CLIENT_HTML_DIR}/RequestHeader.html \
	${CLIENT_HTML_DIR}/ResultFooter.html \
	${CLIENT_HTML_DIR}/ResultHeader.html 

EXTRA_DIST = \
	${SSML_IN_FILES} \
	${MAK_IN_FILES} \
	${SOURCEFILES} \
	${SOAP_MESSAGE_FILES} \
	${CLIENT_HTML_FILES} \
	${DOCUMENT_FILES}


# ssml create rule
SSML_FILES = ${SSML_IN_FILES:.in=}
noinst_DATA = ${SSML_FILES}

${SSML_FILES}: ${SSML_IN_FILES} Makefile
	@for i in ${SSML_IN_FILES}; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done


#### for installation
cgi_bin_PROGRAMFILES = TransactionABankService.cgi
stdio_PROGRAMFILES =   TransactionABankService
client_PROGRAMFILES =  TransactionClient
#
cgi_client_PROGRAMFILES = TransactionApp.cgi
CLIENT_HTML_DIR = TransactionHtml
CLIENT_HTML_FILES = \
	${CLIENT_HTML_DIR}/AcceptedFooter.html \
	${CLIENT_HTML_DIR}/AcceptedHeader.html \
	${CLIENT_HTML_DIR}/CommonFooter.html \
	${CLIENT_HTML_DIR}/CommonHeader.html \
	${CLIENT_HTML_DIR}/ErrorFooter.html \
	${CLIENT_HTML_DIR}/ErrorHeader.html \
	${CLIENT_HTML_DIR}/RequestFooter.html \
	${CLIENT_HTML_DIR}/RequestHeader.html \
	${CLIENT_HTML_DIR}/ResultFooter.html \
	${CLIENT_HTML_DIR}/ResultHeader.html

noinst_PROGRAMS = ${client_PROGRAMFILES} ${cgi_client_PROGRAMFILES} ${cgi_bin_PROGRAMFILES} ${stdio_PROGRAMFILES}
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
OPENSOAP_SERVICE_LOCALSTATEDIR = ${OPENSOAP_LOCALSTATEDIR}/services
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml
#
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
SERVICE_LOCALSTATEDIR = ${OPENSOAP_SERVICE_LOCALSTATEDIR}/${SERVICE_NAME}

INCLUDES = -I${top_builddir} -I${top_builddir}/include

TransactionClient_SOURCES = TransactionClient.c
TransactionClient_LDADD = ${top_builddir}/src/api/libOpenSOAPClient.la

TransactionApp_cgi_SOURCES =
TransactionApp_cgi_LDADD = \
	TransactionApp.o \
	TransactionClient.cgi.o \
	${top_builddir}/src/api/libOpenSOAPClient.la

TransactionABankService_SOURCES = TransactionABankService.c
TransactionABankService_LDADD = \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

TransactionABankService_cgi_SOURCES =
TransactionABankService_cgi_LDADD = \
	TransactionABankService.cgi.o \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

TransactionClient.cgi.o: TransactionClient.c TransactionClient.h
	${COMPILE} -DCLIENT_CGI -c -o $@ TransactionClient.c

TransactionApp.o: TransactionApp.c TransactionClient.h
	${COMPILE} -c -o $@ TransactionApp.c

TransactionABankService.cgi.o: TransactionABankService.c
	${COMPILE} -DCONNECT_TYPE_CGI -c -o $@ $^

cgi-install: ${cgi_bin_PROGRAMFILES}
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}; \
	done

stdio-install: ${stdio_PROGRAMFILES} ${WSDL_FILES}
	@mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${SERVICE_DIR}; \
	done

cgi-client-data-install: ${CLIENT_HTML_FILES}
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}/${CLIENT_HTML_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${INSTALL_PROGRAM} \
			-m 0644 \
			$$f ${DESTDIR}${CGI_BIN_DIR}/$$f; \
	done

cgi-client-install: ${cgi_client_PROGRAMFILES}
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}; \
	done

client-install: ${client_PROGRAMFILES}
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

service-install: cgi-install stdio-install ssml-install

install-samples: client-install service-install cgi-client-install cgi-client-data-install
