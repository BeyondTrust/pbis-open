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
SERVICE_NAME = GetCert

#
SSML_IN_FILES = GetCert.ssml.in

MAK_IN_FILES = GetCert.mak.in

#
DOCUMENT_FILES = \
	README.TXT \
	READMESJ.TXT

# 
KEYFILES = \
	pubKey_Beckham.pem \
	pubKey_CA.pem \
	pubKey_DelPiero.pem

servicedatadir=${SERVICE_LOCALSTATEDIR}
service_DATAFILES = \
	CA.db \
	CA.db_sno
service_DATAFILES_MODE = 0666

EXTRA_DIST = \
	${SSML_IN_FILES} \
	${MAK_IN_FILES} \
	${DOCUMENT_FILES} \
	${service_DATAFILES} \
	${KEYFILES}

# ssml create rule
SSML_FILES = ${SSML_IN_FILES:.in=}
noinst_DATA = ${SSML_FILES}

${SSML_FILES}: ${SSML_IN_FILES} Makefile
	@for i in ${SSML_IN_FILES}; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed "s%\@servicesdir\@%@servicesdir@%g" > $$s < $$i; \
	done


#### for installation
console_client_PROGRAMFILES = GetCertClient
cgi_service_PROGRAMFILES = GetCertService.cgi
stdio_service_PROGRAMFILES = GetCertService
service_support_PROGRAMFILES = CA_Refer
service_support_PROGRAMFILES_MODE = 4755

noinst_PROGRAMS = \
	${console_client_PROGRAMFILES} \
	${service_support_PROGRAMFILES} \
	${stdio_service_PROGRAMFILES} \
	${cgi_service_PROGRAMFILES}

INCLUDES = -I${top_srcdir} -I${top_srcdir}/include

# OpenSOAP conf and data directory
OPENSOAP_SYSCONFDIR = @sysconfdir@
OPENSOAP_LOCALSTATEDIR = @localstatedir@
OPENSOAP_SERVICE_LOCALSTATEDIR = ${OPENSOAP_LOCALSTATEDIR}/services
SSML_DIR=${OPENSOAP_SYSCONFDIR}/ssml

#
SERVICE_DIR = @servicesdir@/${SERVICE_NAME}
SERVICE_LOCALSTATEDIR = ${OPENSOAP_SERVICE_LOCALSTATEDIR}/${SERVICE_NAME}
AM_CFLAGS = \
	-DOPENSOAP_SYSCONFDIR=\"${OPENSOAP_SYSCONFDIR}\" \
	-DSERVICE_DIR=\"${SERVICE_DIR}\" \
	-DSERVICE_LOCALSTATEDIR=\"${SERVICE_LOCALSTATEDIR}\"

SECURITY_LIBS = ${top_builddir}/src/security/libOpenSOAPSecurity.la
CLIENT_LIBS = ${top_builddir}/src/api/libOpenSOAPClient.la \
	${SECURITY_LIBS}
SERVICE_LIBS = ${top_builddir}/src/api/libOpenSOAPService.la \
	${CLIENT_LIBS}


CA_Refer_SOURCES = DBrefer.c
CA_Refer_LDADD = ${CLIENT_LIBS}

GetCertClient_SOURCES = GetCertClient.c
GetCertClient_LDADD = ${CLIENT_LIBS}

GetCertService_SOURCES = GetCertService.c
GetCertService_LDADD = ${SERVICE_LIBS}

GetCertService_cgi_SOURCES =
GetCertService_cgi_LDADD = ${SERVICE_LIBS}  GetCertService.cgi.o


GetCertService.cgi.o: GetCertService.c
	${COMPILE} -DCONNECT_TYPE=\"cgi\" -c -o $@ $^

#############
## Install Client Programs

#
console-client-install: ${console_client_PROGRAMFILES}
	@mkdir -p ${DESTDIR}${bindir}; \
	for f in $^ ; do \
		echo "$$f install to ${bindir}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${bindir}/$$f; \
	done
#
client-install: console-client-install

## Install Service Programs

#
service-support-install: ${service_support_PROGRAMFILES}
	@mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			-m ${service_support_PROGRAMFILES_MODE} \
			$$f ${DESTDIR}${SERVICE_DIR}; \
	done

#
service-datas-install: ${service_DATAFILES}
	@mkdir -m 777 -p ${DESTDIR}${servicedatadir}; \
	for f in $^ ; do \
		echo "$$f install to ${servicedatadir}."; \
		${INSTALL} -m ${service_DATAFILES_MODE} \
			$$f ${DESTDIR}${servicedatadir}/$$f; \
	done

service-common-install: service-support-install service-datas-install

#
ssml-install: ${SSML_FILES}
	@mkdir -p ${DESTDIR}${SSML_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${SSML_DIR}."; \
		${INSTALL_DATA} \
			$$f ${DESTDIR}${SSML_DIR}/$$f; \
	done

stdio-service-programs-install: ${stdio_service_PROGRAMFILES}
	@mkdir -p ${DESTDIR}${SERVICE_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${SERVICE_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${SERVICE_DIR}; \
	done

stdio-service-install: \
	service-common-install \
	stdio-service-programs-install \
	ssml-install

#
cgi-service-programs-install: ${cgi_service_PROGRAMFILES}
	@mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}/$$f; \
	done

cgi-service-install: \
	service-common-install \
	cgi-service-programs-install

#
service-install: cgi-service-install stdio-service-install

#
install-samples: client-install service-install
