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

SERVICE_NAME = Soaping

console_client_PROGRAMFILES = soaping

cgi_service_PROGRAMFILES = SoapingService.cgi
stdio_service_PROGRAMFILES = SoapingService

SSML_IN_FILES = ${SERVICE_NAME}.ssml.in
SSML_FILES = ${SSML_IN_FILES:.in=}

bin_PROGRAMS = ${console_client_PROGRAMFILES}

noinst_PROGRAMS = \
	${stdio_service_PROGRAMFILES} \
	${cgi_service_PROGRAMFILES}

SERVICE_DIR = @servicesdir@/${SERVICE_NAME}

# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

# 
INCLUDES = -I${top_builddir} -I${top_builddir}/include

soaping_SOURCES = SoapingClient.c
soaping_LDADD = ${top_builddir}/src/api/libOpenSOAPClient.la

SoapingService_SOURCES = SoapingService.c
SoapingService_LDADD = \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

SoapingService_cgi_SOURCES =
SoapingService_cgi_LDADD = \
	SoapingService.cgi.o \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

SoapingService.cgi.o: SoapingService.c
	${COMPILE} -DCONNECT_TYPE=\"cgi\" -c -o $@ $^

EXTRA_DIST = \
	${SSML_IN_FILES}

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

install-exec-hook: install-stdio-service install-cgi-service
