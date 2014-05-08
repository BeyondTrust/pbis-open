# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------

# 
SERVICE_NAME = CalcAsync

#
WSDL_FILES = CalcAsyncService.wsdl
#
DOCUMENT_FILES = \
	README.TXT \
	READMESJ.TXT

SAMPLE_MESSAGE_DIR = CalcAsyncSampleMessages
SAMPLE_MESSAGE_FILES = \
	${SAMPLE_MESSAGE_DIR}/CalcAsyncAdd.xml \
	${SAMPLE_MESSAGE_DIR}/CalcAsyncDivide.xml \
	${SAMPLE_MESSAGE_DIR}/CalcAsyncMultiply.xml \
	${SAMPLE_MESSAGE_DIR}/CalcAsyncSubtract.xml \
	forward_path.txt

CLIENT_HTML_DIR = CalcAsyncHtml
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

SSML_IN_FILES =  ${SERVICE_NAME}.ssml.in

MAK_IN_FILES = ${SERVICE_NAME}.mak.in

EXTRA_DIST = \
	${DOCUMENT_FILES} \
	${MAK_IN_FILES} \
	${SSML_IN_FILES} \
	${SAMPLE_MESSAGE_FILES} \
	${CLIENT_HTML_FILES} \
	${WSDL_FILES}

# ssml create rule
SSML_FILES = ${SSML_IN_FILES:.in=}
noinst_DATA = ${SSML_FILES}

${SSML_FILES}: ${SSML_IN_FILES} Makefile
	@for i in ${SSML_IN_FILES}; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done

#### for installation
cgi_bin_PROGRAMFILES = CalcAsyncService.cgi
stdio_PROGRAMFILES =   CalcAsyncService
client_PROGRAMFILES =  CalcAsyncClient
#
cgi_client_PROGRAMFILES = CalcAsyncApp.cgi
CLIENT_HTML_DIR = CalcAsyncHtml
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
AM_CFLAGS = -DSERVICE_LOCALSTATEDIR=\"${SERVICE_LOCALSTATEDIR}\"

CalcAsyncClient_SOURCES = CalcAsyncClient.c CalcAsyncClient.h
CalcAsyncClient_LDADD = ${top_builddir}/src/api/libOpenSOAPClient.la

CalcAsyncApp_cgi_SOURCES = CalcAsyncApp.c CalcAsyncClient.h
CalcAsyncApp_cgi_LDADD = \
	CalcAsyncClient.cgi.o \
	${top_builddir}/src/api/libOpenSOAPClient.la

CalcAsyncService_SOURCES = CalcAsyncService.c
CalcAsyncService_LDADD = \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

CalcAsyncService_cgi_SOURCES =
CalcAsyncService_cgi_LDADD = \
	CalcAsyncService.cgi.o \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

CalcAsyncClient.cgi.o: CalcAsyncClient.c CalcAsyncClient.h
	${COMPILE} -DCLIENT_CGI -c -o $@ CalcAsyncClient.c

CalcAsyncApp.o: CalcAsyncApp.c CalcAsyncClient.h
	${COMPILE} -c -o $@ CalcAsyncApp.c

CalcAsyncService.cgi.o: CalcAsyncService.c
	${COMPILE} -DCONNECT_TYPE_CGI -c -o $@ $^

cgi-install: ${cgi_bin_PROGRAMFILES}
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}; \
	done

stdio-install: ${stdio_PROGRAMFILES} ${WSDL_FILES}
	@mkdir -m 777 -p ${DESTDIR}${SERVICE_LOCALSTATEDIR}; \
	mkdir -p ${DESTDIR}${SERVICE_DIR}; \
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
