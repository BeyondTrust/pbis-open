# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------

# 
SERVICE_NAME = Calc

#
DOCUMENT_FILES = \
	README.TXT \
	READMESJ.TXT

SAMPLE_MESSAGE_DIR = CalcSampleMessages

SAMPLE_MESSAGE_FILES = 

SSML_IN_FILES =  ${SERVICE_NAME}.ssml.in

MAK_IN_FILES = ${SERVICE_NAME}.mak.in

WSDL_FILES = CalcService.wsdl

PROGRAM_SOURCEFILES = \
	CalcClient.c \
	CalcService.c

EXTRA_DIST = \
	${DOCUMENT_FILES} \
	${PROGRAM_SOURCEFILES} \
	${MAK_IN_FILES} \
	${SSML_IN_FILES} \
	${WSDL_FILES} \
	${SAMPLE_MESSAGE_FILES}


# ssml create rule
SSML_FILES = ${SSML_IN_FILES:.in=}
noinst_DATA = ${SSML_FILES}

${SSML_FILES}: ${SSML_IN_FILES} Makefile
	@for i in ${SSML_IN_FILES}; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done


#### for installation
cgi_bin_PROGRAMFILES = CalcService.cgi
stdio_PROGRAMFILES =   CalcService
client_PROGRAMFILES =  CalcClient

noinst_PROGRAMS = ${client_PROGRAMFILES} ${cgi_bin_PROGRAMFILES} ${stdio_PROGRAMFILES}
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR=@sysconfdir@
OPENSOAP_LOCALSTATEDIR=@localstatedir@
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

INCLUDES = -I${top_srcdir} -I${top_srcdir}/include

CalcClient_SOURCES = CalcClient.c
CalcClient_LDADD = ${top_builddir}/src/api/libOpenSOAPClient.la

CalcService_SOURCES = CalcService.c
CalcService_LDADD = \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

CalcService_cgi_SOURCES =
CalcService_cgi_LDADD = \
	CalcService.cgi.o \
	${top_builddir}/src/api/libOpenSOAPService.la \
	${top_builddir}/src/api/libOpenSOAPClient.la

CalcService.cgi.o: CalcService.c
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
