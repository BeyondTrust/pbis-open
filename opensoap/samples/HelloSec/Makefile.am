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

SERVICE_NAME = HelloSec

cgi_bin_PROGRAMFILES = HelloSecService.cgi
stdio_PROGRAMFILES = HelloSecService
client_PROGRAMFILES = HelloSecClient

noinst_PROGRAMS = ${client_PROGRAMFILES} ${cgi_bin_PROGRAMFILES} ${stdio_PROGRAMFILES}

SERVICE_DIR = @servicesdir@/${SERVICE_NAME}

SSML_IN_FILES = ${SERVICE_NAME}.ssml.in
SSML_FILES = ${SSML_IN_FILES:.in=}

MAK_IN_FILES = ${SERVICE_NAME}.mak.in

# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

INCLUDES = -I${top_srcdir} -I${top_srcdir}/include

AM_CFLAGS = -DSERVICE_DIR=\"${SERVICE_DIR}\"

HelloSecClient_SOURCES = HelloSecClient.c
HelloSecClient_LDADD = \
	${top_builddir}/src/security/libOpenSOAPSecurity.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

HelloSecService_SOURCES = HelloSecService.c
HelloSecService_LDADD = \
	${top_builddir}/src/security/libOpenSOAPSecurity.la \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

HelloSecService_cgi_SOURCES =
HelloSecService_cgi_LDADD = \
	HelloSecService.cgi.o \
	${top_builddir}/src/security/libOpenSOAPSecurity.la \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

HelloSecService.cgi.o: HelloSecService.c
	${COMPILE} -DCONNECT_TYPE=\"cgi\" -c -o $@ $^

console_client_USEKEYFILES = \
	 	pubKey_HelloService.pem \
		privKey_HelloClient.pem

service_USEKEYFILES = \
	 	privKey_HelloService.pem \
		pubKey_HelloClient.pem

EXTRA_DIST = \
	${SSML_IN_FILES} \
	${MAK_IN_FILES} \
	READMESJ.TXT \
	README.TXT \
	hello_sec_request.xml \
	hello_sec_response.xml \
	${console_client_USEKEYFILES} \
	${service_USEKEYFILES}

# ssml create rule
${SSML_FILES}: ${SSML_IN_FILES} Makefile
	for i in ${SSML_IN_FILES} ; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done

cgi-install: ${cgi_bin_PROGRAMFILES}
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

stdio-install: ${stdio_PROGRAMFILES}
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

client-install: ${client_PROGRAMFILES}
	@if test -d ${DESTDIR}${bindir}; then \
		echo "${DESTDIR}${bindir} exits."; \
	else \
		echo "${DESTDIR}${bindir} does not exit."; \
		echo "mkdir -p ${DESTDIR}${bindir}."; \
		mkdir -p ${DESTDIR}${bindir}; \
	fi
	for f in $^ ; do \
		echo "$$f install to ${bindir}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${bindir}; \
	done

ssml-install: ${SSML_FILES}
	@if test -d ${DESTDIR}${SSML_DIR}; then \
		echo "${DESTDIR}${SSML_DIR} exits."; \
	else \
		echo "mkdir -p ${DESTDIR}${SSML_DIR}."; \
		mkdir -p ${DESTDIR}${SSML_DIR}; \
	fi
	for f in $^ ; do \
		echo "$$f install to ${SSML_DIR}."; \
		cp $$f ${DESTDIR}${SSML_DIR}; \
	done

service-install: cgi-install stdio-install ssml-install \
                 install-service-usekeys

install-service-usekeys: ${service_USEKEYFILES}
	@if test -d ${DESTDIR}${SERVICE_DIR}; then \
		echo "${DESTDIR}${SERVICE_DIR} exist."; \
	else \
		echo "mkdir -p ${DESTDIR}${SERVICE_DIR}."; \
		mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	fi
	@for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${INSTALL_PROGRAM} \
			-m 0644 \
			$$f ${DESTDIR}${SERVICE_DIR}; \
	done


install-samples: client-install service-install

## clean
clean: 
	rm -f ${OBJS} ${PROGRAMS}

## Client test

ctest: ${console_client_PROGRAMFILES}
	./HelloSecClient foo http://localhost/cgi-bin/HelloSecService.cgi

## Service test
stest: ${cgi_service_PROGRAMFILES}
	./HelloSecService.cgi < ${SAMPLE_MESSAGE_DIR}/hello_sec_request.xml
