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

SERVICE_NAME = Echo

cgi_bin_PROGRAMFILES = EchoService.cgi
stdio_PROGRAMFILES = EchoService
client_PROGRAMFILES = EchoClient

noinst_PROGRAMS = ${client_PROGRAMFILES} ${cgi_bin_PROGRAMFILES} ${stdio_PROGRAMFILES}

SERVICE_DIR = @servicesdir@/${SERVICE_NAME}

SSML_IN_FILES = ${SERVICE_NAME}.ssml.in
SSML_FILES = ${SSML_IN_FILES:.in=}

MAK_IN_FILES = ${SERVICE_NAME}.mak.in

#WSDL_FILES = EchoService.wsdl
WSDL_FILES = 

# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

INCLUDES = -I${top_builddir} -I${top_builddir}/include

EchoClient_SOURCES = EchoClient.c Echo.h
EchoClient_LDADD = ${top_builddir}/src/api/libOpenSOAPClient.la

EchoService_SOURCES = EchoService.c Echo.h
EchoService_LDADD = \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

EchoService_cgi_SOURCES =
EchoService_cgi_LDADD = \
	EchoService.cgi.o \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

EchoService.cgi.o: EchoService.c
	${COMPILE} -DCONNECT_TYPE=\"cgi\" -c -o $@ $^

EXTRA_DIST = \
	${SSML_IN_FILES} \
	${MAK_IN_FILES} \
	READMESJ.TXT \
	README.TXT \
	${WSDL_FILES}

# ssml create rule
${SSML_FILES}: ${SSML_IN_FILES} Makefile
	@for i in ${SSML_IN_FILES}; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done
noinst_DATA = ${SSML_FILES}

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

install-samples: client-install service-install


## Test Service Programs

#stest: ${stdio_service_PROGRAMFILES}
#	cat hello_request.xml | ./EchoService.cgi

## Test Client-Service (or Client-Server-Service) Hookups

ctest1: ${console_client_PROGRAMFILES}
	./EchoClient 1 TESTstring

ctest3: ${console_client_PROGRAMFILES}
	./EchoClient 3 123456

ctest5: ${console_client_PROGRAMFILES}
	./EchoClient 5 3.141592

ctest11: ${console_client_PROGRAMFILES}
	./EchoClient 11
