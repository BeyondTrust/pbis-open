# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------

# 
SERVICE_NAME = SimpleCalc

#
WSDL_FILES = SimpleCalcService.wsdl
#
DOCUMENT_FILES = \
	README.TXT \
	READMESJ.TXT

SAMPLE_MESSAGE_DIR = SimpleCalcSampleMessages

SAMPLE_MESSAGE_FILES = \
	${SAMPLE_MESSAGE_DIR}/add_request.xml \
	${SAMPLE_MESSAGE_DIR}/add_response.xml \
	${SAMPLE_MESSAGE_DIR}/subtract_request.xml \
	${SAMPLE_MESSAGE_DIR}/subtract_response.xml \
	${SAMPLE_MESSAGE_DIR}/multiply_request.xml \
	${SAMPLE_MESSAGE_DIR}/multiply_response.xml \
	${SAMPLE_MESSAGE_DIR}/divide_request.xml \
	${SAMPLE_MESSAGE_DIR}/divide_response.xml

SSML_IN_FILES =  ${SERVICE_NAME}.ssml.in

MAK_IN_FILES = ${SERVICE_NAME}.mak.in

PROGRAM_SOURCEFILES = \
	SimpleCalcClient.c \
	SimpleCalcService.c

EXTRA_DIST = \
	${DOCUMENT_FILES} \
	${PROGRAM_SOURCEFILES} \
	${MAK_IN_FILES} \
	${SSML_IN_FILES} \
	${SAMPLE_MESSAGE_FILES} \
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
cgi_bin_PROGRAMFILES = SimpleCalcService.cgi
stdio_PROGRAMFILES =   SimpleCalcService
client_PROGRAMFILES =  SimpleCalcClient

noinst_PROGRAMS = ${client_PROGRAMFILES} ${cgi_bin_PROGRAMFILES} ${stdio_PROGRAMFILES}
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

INCLUDES = -I${top_builddir} -I${top_builddir}/include

SimpleCalcClient_SOURCES = SimpleCalcClient.c
SimpleCalcClient_LDADD = ${top_builddir}/src/api/libOpenSOAPClient.la

SimpleCalcService_SOURCES = SimpleCalcService.c
SimpleCalcService_LDADD = \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

SimpleCalcService_cgi_SOURCES =
SimpleCalcService_cgi_LDADD = \
	SimpleCalcService.cgi.o \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

SimpleCalcService.cgi.o: SimpleCalcService.c
	${COMPILE} -DCONNECT_TYPE=\"cgi\" -c -o $@ $^

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
