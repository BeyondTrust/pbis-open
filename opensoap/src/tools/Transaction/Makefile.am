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

SERVICE_NAME = Transaction

stdio_service_PROGRAMFILES = TransactionService
cgi_service_PROGRAMFILES   = TransactionService.cgi

SSML_IN_FILES = ${SERVICE_NAME}.ssml.in
SSML_FILES = ${SSML_IN_FILES:.in=}

noinst_PROGRAMS = \
	${stdio_service_PROGRAMFILES} \
	${cgi_service_PROGRAMFILES}

# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
OPENSOAP_SERVICE_LOCALSTATEDIR = ${OPENSOAP_LOCALSTATEDIR}/services
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

#
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
SERVICE_LOCALSTATEDIR = ${OPENSOAP_SERVICE_LOCALSTATEDIR}/${SERVICE_NAME}

INCLUDES = -I${top_srcdir} -I${top_srcdir}/include

TransactionService_SOURCES = TransactionService.c
TransactionService_LDADD = \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

TransactionService_cgi_SOURCES =
TransactionService_cgi_LDADD = \
	TransactionService.cgi.o \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la
TransactionService_cgi_CFLAGS = \
	-DSERVICE_LOCALSTATEDIR=\"${SERVICE_LOCALSTATEDIR}\"

TransactionService.cgi.o: TransactionService.c
	${COMPILE} -DCONNECT_TYPE_CGI -c -o $@ $^

SOAP_MESSAGE_FILES = 	\
	transaction_request.xml

EXTRA_DIST = \
	${SSML_IN_FILES} \
	${SOAP_MESSAGE_FILES}

# ssml create rule
${SSML_FILES}: ${SSML_IN_FILES} Makefile
	@for i in ${SSML_IN_FILES} ; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done

ssml_DATA = ${SSML_FILES}
ssmldir = ${SSML_DIR}

# install rules
install-stdio-service: ${stdio_service_PROGRAMFILES}
	@if test -d ${DESTDIR}${SERVICE_LOCALSTATEDIR}; then \
		echo "${DESTDIR}${SERVICE_LOCALSTATEDIR} exits."; \
	else \
		echo "${DESTDIR}${SERVICE_LOCALSTATEDIR} does not exit."; \
		echo "mkdir -m 777 -p ${DESTDIR}${SERVICE_LOCALSTATEDIR}."; \
		mkdir -m 777 -p ${DESTDIR}${SERVICE_LOCALSTATEDIR}; \
	fi
	@if test -d ${DESTDIR}${SERVICE_DIR}; then \
		echo "${DESTDIR}${SERVICE_DIR} exits."; \
	else \
		echo "${DESTDIR}${SERVICE_DIR} does not exit."; \
		echo "mkdir -p ${DESTDIR}${SERVICE_DIR}."; \
		mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	fi
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${SERVICE_DIR}; \
	done

install-cgi-service: ${cgi_service_PROGRAMFILES}
	@if test -d ${DESTDIR}${CGI_BIN_DIR}; then \
		echo "${DESTDIR}${CGI_BIN_DIR} exits."; \
	else \
		echo "${DESTDIR}${CGI_BIN_DIR} does not exit."; \
		echo "mkdir -p ${DESTDIR}${CGI_BIN_DIR}."; \
		mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	fi
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}; \
	done

# install hooks
install-exec-hook: install-stdio-service install-cgi-service
