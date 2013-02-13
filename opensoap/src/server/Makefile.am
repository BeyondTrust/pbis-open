# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------


MODSO_DIR = mod_opensoap
MODSO_FILES = \
	${MODSO_DIR}/Makefile.in \
	${MODSO_DIR}/mod_opensoap.c
if WITH_APXS2
SUBDIRS = ${MODSO_DIR}
else
SUBDIRS = 
endif

DIRECTORY_CFLAGS = \
	-DPREFIX=\"@prefix@\" \
	-DSYSCONFDIR=\"@sysconfdir@\" \
	-DLOCALSTATEDIR=\"@localstatedir@\"

LDADD_LIBS = \
	@XML2_LIBS@ @GNU_PTHREAD_LIBS@ @PTHREAD_LIBS@

PRIV_KEY = privKey.pem
PUB_KEY  = pubKey.pem
SECURITY_DATA_FILES = ${PRIV_KEY} ${PUB_KEY}
SECURITY_DATA_DIR = @sysconfdir@

### scripts ###

SCRIPTS_IN = \
		queueManager_fwd.in    \
		opensoap-server-ctl.in

sbin_SCRIPTS = ${SCRIPTS_IN:.in=}

### binaries ###
cgi_bin_PROGRAMFILES = soapInterface.cgi

sbin_PROGRAMS = \
		OpenSOAPMgr	\
		ssmlAttrMgr 	\
		srvConfAttrMgr	\
		idManager	\
		queueManager	\
		msgDrvCreator	\
		spoolManager	\
		ttlManager

noinst_PROGRAMS = ${cgi_bin_PROGRAMFILES}

# extra dist

EXTRA_DIST	=	\
			${SCRIPTS_IN} \
			${SECURITY_DATA_FILES}	\
			${MODSO_FILES}	\
			\
			Server_DLL.c		\
			IdManagerWinService.cpp 		\
			IdManagerWinService.h			\
			MsgDrvCreatorWinService.cpp 		\
			MsgDrvCreatorWinService.h		\
			QueueManagerFwdWinService.cpp 		\
			QueueManagerFwdWinService.h		\
			QueueManagerWinService.cpp 		\
			QueueManagerWinService.h		\
			SSMLAttrMgrWinService.cpp 		\
			SSMLAttrMgrWinService.h			\
			SpoolManagerWinService.cpp 		\
			SpoolManagerWinService.h		\
			SrvConfAttrMgrWinService.cpp 		\
			SrvConfAttrMgrWinService.h		\
			TTLManagerWinService.cpp 		\
			TTLManagerWinService.h			\
			WinServiceBase.cpp 			\
			WinServiceBase.h

INCLUDES = \
	${DIRECTORY_CFLAGS} \
	-I${top_srcdir} -I${top_srcdir}/include \
	@XML2_CFLAGS@ @GNU_PTHREAD_CFLAGS@ @PTHREAD_CFLAGS@

### libraries ###
lib_LTLIBRARIES = \
	libconnection.la 	\
	libSOAPMessage.la	\
	libOpenSOAPServer.la	\
	libOpenSOAPInterface.la	\
	libFileLib.la		\
	libSharedLib.la		\
	libTraceLib.la

libOpenSOAPServer_la_SOURCES = \
	ChannelDescriptor.cpp ChannelDescriptor.h	\
	ChannelManager.cpp ChannelManager.h		\
	ChannelSelector.cpp ChannelSelector.h		\
	DataRetainer.cpp DataRetainer.h		\
	Logger.h Logger.cpp					\
	FileLogger.h FileLogger.cpp			\
	SysLogger.h SysLogger.cpp			\
	AppLogger.h AppLogger.cpp			\
	ProcessInfo.h ProcessInfo.cpp		\
	ThreadInfo.h ThreadInfo.cpp			\
	MsgInfo.h MsgInfo.cpp				\
	SrvErrorCode.h 						\
	Exception.h Exception.cpp			\
	ErrorUtil.h ErrorUtil.cpp			\
	MapUtil.cpp MapUtil.h 				\
	PIDFactory.cpp PIDFactory.h			\
	ServerCommon.cpp ServerCommon.h			\
	ServerDef.h					\
	SrvConf.cpp SrvConf.h				\
	SrvConfAttrHandler.cpp SrvConfAttrHandler.h	\
	SSMLAttrMgrProtocol.h				\
	ThreadDef.cpp ThreadDef.h

libOpenSOAPServer_la_LDFLAGS = \
	-version-info  @OPENSOAP_VERSION_INFO@

libOpenSOAPServer_la_LIBADD = \
	libSOAPMessage.la \
	${PTHREAD_LIBS}


libconnection_la_SOURCES = \
	connection.cpp connection.h

libconnection_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@
#

libSOAPMessage_la_SOURCES = \
	fio.cpp fio.h				\
	MsgAttrHandler.cpp MsgAttrHandler.h	\
	RequestMessage.cpp RequestMessage.h	\
	SignatureFunc.cpp SignatureFunc.h	\
	SoapDef.cpp SoapDef.h			\
	SOAPMessageFunc.cpp SOAPMessageFunc.h		\
	StringUtil.h SSMLAttrMgrProtocol.h	\
	XmlModifier.cpp	XmlModifier.h		\
	XmlQuery.cpp XmlQuery.h			\
	XmlQuery2.cpp XmlQuery2.h

#	SOAPMessage.cpp	SOAPMessage.h		\


libSOAPMessage_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@

libSOAPMessage_la_LIBADD = \
	${top_builddir}/src/security/libOpenSOAPSecurity.la 	\
	${top_builddir}/src/api/libOpenSOAPClient.la

#
libOpenSOAPInterface_la_SOURCES = \
	MsgDrvChnlSelector.cpp MsgDrvChnlSelector.h	\
	soapInterface4ap_dso.cpp soapInterface4ap_dso.h

libOpenSOAPInterface_la_LDFLAGS = \
	-version-info  @OPENSOAP_VERSION_INFO@

libOpenSOAPInterface_la_LIBADD = \
	${top_builddir}/src/api/libOpenSOAPClient.la

libFileLib_la_SOURCES = \
	FileMgr.cpp FileMgr.h TraceTypes.h

libFileLib_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@


libSharedLib_la_SOURCES = \
	SharedMgr.cpp SharedMgr.h	\
	shmem.cpp shmem.h		\
	semop.cpp semop.h

libSharedLib_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@


libTraceLib_la_SOURCES = \
	TraceMgr.cpp TraceMgr.h

libTraceLib_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@

#
OpenSOAPMgr_LDADD = \
	libFileLib.la libSharedLib.la libTraceLib.la \
	libOpenSOAPServer.la \
	${LDADD_LIBS}

OpenSOAPMgr_SOURCES = \
	OpenSOAPMgr.cpp OpenSOAPMgr.h
#
ssmlAttrMgr_LDADD = \
	libFileLib.la libSharedLib.la libTraceLib.la	\
	libOpenSOAPServer.la \
	${LDADD_LIBS}

ssmlAttrMgr_SOURCES = \
	SSMLAttrMgr.cpp SSMLAttrMgr.h 	\
	SSMLAttrMgr_main.cpp
#
srvConfAttrMgr_LDADD = \
	libFileLib.la libSharedLib.la libTraceLib.la	\
	libOpenSOAPServer.la \
	${LDADD_LIBS}

srvConfAttrMgr_SOURCES = \
	SrvConfAttrMgr.cpp SrvConfAttrMgr.h	\
	SrvConfAttrMgr_main.cpp			\
	TraceLog.cpp TraceLog.h

#
idManager_LDFLAGS = \
	${UUID_LIBS}

idManager_LDADD = \
	libFileLib.la libSharedLib.la libTraceLib.la	\
	libOpenSOAPServer.la \
	${LDADD_LIBS}

idManager_SOURCES = \
	IdManager_main.cpp	\
	IdManager.cpp IdManager.h	\
	TraceLog.cpp TraceLog.h
#
queueManager_LDADD = \
	libFileLib.la libSharedLib.la libTraceLib.la	\
	libOpenSOAPServer.la \
	${LDADD_LIBS}

queueManager_SOURCES = \
	FileIDFunc.cpp FileIDFunc.h			\
	MsgDrvChnlSelector.cpp MsgDrvChnlSelector.h	\
	Queue.cpp Queue.h					\
	QueueManager.cpp QueueManager.h				\
	QueueManager_main.cpp				\
	TraceLog.cpp TraceLog.h

#
msgDrvCreator_LDADD = \
	${top_builddir}/src/api/libOpenSOAPClient.la 		\
	${top_builddir}/src/security/libOpenSOAPSecurity.la 	\
	libFileLib.la libSharedLib.la libTraceLib.la	\
	libconnection.la libSOAPMessage.la 			\
	libOpenSOAPServer.la \
	${LDADD_LIBS}


msgDrvCreator_SOURCES = \
	FileIDFunc.cpp FileIDFunc.h				\
	FwdQueuePushChnlSelector.cpp FwdQueuePushChnlSelector.h	\
	MsgDrv.cpp MsgDrv.h					\
	MsgDrvCreator.cpp MsgDrvCreator.h			\
	MsgDrvCreator_main.cpp 					\
	QueueAdopter.cpp QueueAdopter.h				\
	ReqQueueExpireChnlSelector.cpp ReqQueueExpireChnlSelector.h	\
	ReqQueuePushChnlSelector.cpp ReqQueuePushChnlSelector.h	\
	ResSpoolExpireChnlSelector.cpp ResSpoolExpireChnlSelector.h	\
	ResSpoolPopChnlSelector.cpp ResSpoolPopChnlSelector.h	\
	ResSpoolPushChnlSelector.cpp ResSpoolPushChnlSelector.h	\
	SpoolAdopter.cpp SpoolAdopter.h		\
	SSMLAttrHandler.cpp SSMLAttrHandler.h			\
	SSMLInfo.cpp SSMLInfo.h					\
	SSMLInfoFunc.cpp SSMLInfoFunc.h		 		\
	Timeout.cpp Timeout.h					\
	TraceLog.cpp TraceLog.h					\
	TTLTablePushChnlSelector.cpp TTLTablePushChnlSelector.h	\
	TTLTableRefChnlSelector.cpp TTLTableRefChnlSelector.h	\
	TTLAdopter.cpp TTLAdopter.h	\
	XmlDoc.cpp XmlDoc.h	\
	SoapException.cpp SoapException.h	\
	SoapMessage.cpp SoapMessage.h	\
	SoapMessageSerializer.cpp SoapMessageSerializer.h	\
	RuleBase.h Rule.h Rule.cpp 	\
	Invoker.cpp Invoker.h	\
	RequestQueueingInvoker.h RequestQueueingInvoker.cpp	\
	ResponseQueryInvoker.h ResponseQueryInvoker.cpp		\
	ResponseSpoolingOrForwardingInvoker.h 			\
	ResponseSpoolingOrForwardingInvoker.cpp			\
	SpoolingInvoker.h SpoolingInvoker.cpp			\
	ForwardingInvoker.h ForwardingInvoker.cpp		\
	ServiceInvoker.cpp ServiceInvoker.h \
	HttpServiceInvoker.cpp HttpServiceInvoker.h \
	StdIOServiceInvoker.cpp StdIOServiceInvoker.h \
	ServiceInvokerFactory.cpp ServiceInvokerFactory.h


#
spoolManager_LDADD = \
	${top_builddir}/src/api/libOpenSOAPClient.la		\
	libFileLib.la libSharedLib.la libTraceLib.la	\
	libSOAPMessage.la \
	libOpenSOAPServer.la \
	${LDADD_LIBS}

spoolManager_SOURCES = \
	FileIDFunc.cpp FileIDFunc.h 		\
	Spool.cpp Spool.h			\
	SpoolManager.cpp SpoolManager.h		\
	SpoolManager_main.cpp			\
	TraceLog.cpp TraceLog.h

#
ttlManager_LDADD = \
	${top_builddir}/src/api/libOpenSOAPClient.la		\
	libFileLib.la libSharedLib.la libTraceLib.la	\
	libSOAPMessage.la \
	libOpenSOAPServer.la \
	${LDADD_LIBS}

ttlManager_SOURCES = \
	FileIDFunc.cpp FileIDFunc.h	\
	ResSpoolExpireChnlSelector.cpp ResSpoolExpireChnlSelector.h	\
	ResSpoolPopChnlSelector.cpp ResSpoolPopChnlSelector.h	\
	ResSpoolPushChnlSelector.cpp ResSpoolPushChnlSelector.h	\
	SpoolAdopter.cpp SpoolAdopter.h		\
	SSMLInfo.cpp SSMLInfo.h					\
	SSMLInfoFunc.cpp SSMLInfoFunc.h		 		\
	SSMLAttrHandler.cpp SSMLAttrHandler.h			\
	TraceLog.cpp TraceLog.h					\
	TTL.cpp TTL.h			\
	TTLManager.cpp TTLManager.h	\
	TTLManager_main.cpp

#
soapInterface_cgi_LDADD = \
	${top_builddir}/src/api/libOpenSOAPClient.la 	\
	${top_builddir}/src/api/libOpenSOAPService.la 	\
	libconnection.la \
	libSOAPMessage.la \
	libOpenSOAPServer.la \
	${LDADD_LIBS}

soapInterface_cgi_SOURCES = \
	FileIDFunc.cpp FileIDFunc.h			\
	MsgDrvChnlSelector.cpp MsgDrvChnlSelector.h	\
	soapInterface.cpp	


install-cgi: ${cgi_bin_PROGRAMFILES}
	@if test -d ${DESTDIR}${CGI_BIN_DIR}; then \
		echo "${DESTDIR}${CGI_BIN_DIR} exits."; \
	else \
		echo "${DESTDIR}${CGI_BIN_DIR} does not exit."; \
		echo "mkdir -p ${DESTDIR}${CGI_BIN_DIR}."; \
		mkdir -p ${DESTDIR}${CGI_BIN_DIR}; \
	fi
	@for f in $^ ; do \
		echo "$$f install to ${CGI_BIN_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_PROGRAM} \
			$$f ${DESTDIR}${CGI_BIN_DIR}; \
	done

install-security-data: ${SECURITY_DATA_FILES}
	@if test -d ${DESTDIR}${SECURITY_DATA_DIR}; then \
		echo "${DESTDIR}${SECURITY_DATA_DIR} exits."; \
	else \
		echo "mkdir -p ${DESTDIR}${SECURITY_DATA_DIR}."; \
		mkdir -p ${DESTDIR}${SECURITY_DATA_DIR}; \
		for f in $^ ; do \
			echo "$$f install to ${SECURITY_DATA_DIR}."; \
			${LIBTOOL} --mode=install ${INSTALL_DATA} \
				$$f ${DESTDIR}${SECURITY_DATA_DIR}/$$f; \
		done \
	fi
	@for f in $^ ; do \
		echo "$$f install to ${SECURITY_DATA_DIR}."; \
		${LIBTOOL} --mode=install ${INSTALL_DATA} \
			$$f ${DESTDIR}${SECURITY_DATA_DIR}/$$f.default; \
	done

install-exec-hook: install-cgi

install-data-hook: install-security-data
