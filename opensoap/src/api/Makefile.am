# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
# 

INCLUDES = -I${top_builddir} -I${top_builddir}/include @XML2_CFLAGS@ @MBFUNCS_CFLAGS@ @SSL_INCLUDES@

lib_LTLIBRARIES = \
	libOpenSOAPClient.la \
	libOpenSOAPService.la

libOpenSOAPClient_la_LIBADD = \
	@PTHREAD_LIBS@ @XML2_LIBS@ @ICONV_LIBS@ @SSL_LIBS@

# libOpenSOAPService_la_LIBADD = \
# 	${top_builddir}/src/api/libOpenSOAPClient.la

libOpenSOAPClient_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@

libOpenSOAPService_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@

#
MKDEFFILE = ${top_builddir}/${MKDEFFILE_SH}

libOpenSOAPClient_la_SOURCES = \
	AddrInfo.c AddrInfo.h \
	Block.c Block.h \
	ByteArray.c ByteArray.h \
	ClientSocket.c ClientSocket.h \
	CStdio.c CStdio.h \
	DLinkList.c DLinkList.h \
	Envelope.c Envelope.h \
	Locale.c LocaleImpl.h \
	Object.c Object.h \
	OpenSOAP.c \
	Serializer.c Serializer.h \
	Socket.c Socket.h \
	Stream.c Stream.h \
	String.c StringImpl.h \
	StringHash.c StringHash.h \
	Transport.c Transport.h \
	XMLAttr.c XMLAttr.h \
	XMLElm.c XMLElm.h \
	XMLNode.c XMLNode.h \
	XMLNamespace.c XMLNamespace.h \
	XMLNamespaceList.h

libOpenSOAPService_la_SOURCES = \
	Service.c Service.h

OpenSOAPClient.def: ${libOpenSOAPClient_la_SOURCES}
	${MKDEFFILE} $@ $^

OpenSOAPService.def: ${libOpenSOAPService_la_SOURCES}
	${MKDEFFILE} $@ $^

EXTRA_DIST = \
	Client_DLL.c \
	Service_DLL.c

W32_DEF_FILES = \
	OpenSOAPClient.def \
	OpenSOAPService.def

dist-hook:
	rm -f ${W32_DEF_FILES}
	${MAKE} ${W32_DEF_FILES}
	for file in ${W32_DEF_FILES}; do \
	  cp $$file ${distdir}/$$file; \
	done
