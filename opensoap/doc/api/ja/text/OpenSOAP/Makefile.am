# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
##

SUFFIXES = .TXT .h

DTOU_FILTER = @DTOU_JA_FILTER@

SOURCE_INCLUDEDIR = ${top_builddir}/include/OpenSOAP

DOCSOURCE_FILES = \
	Block.TXT \
	ByteArray.TXT \
	CStdio.TXT \
	ClientSocket.TXT \
	Defines.TXT \
	Envelope.TXT \
	ErrorCode.TXT \
	Locale.TXT \
	OpenSOAP.TXT \
	Security.TXT \
	Serializer.TXT \
	Service.TXT \
	Stream.TXT \
	String.TXT \
	StringHash.TXT \
	Transport.TXT \
	XMLAttr.TXT \
	XMLElm.TXT \
	XMLNamespace.TXT

DOXYGEN_SOURCE_FILES = ${DOCSOURCE_FILES:.TXT=.h}

EXTRA_DIST = ${DOCSOURCE_FILES}

DOCS_SRC_STAMP = .docs-src-stamp

.TXT.h:
	if test "x${SED}" != "x" \
	  && test -f $< && test -f ${SOURCE_INCLUDEDIR}/$@ ; then \
	    (cat $< | eval ${DTOU_FILTER} | ${SED} "s/$</$@/g"; \
	     cat ${SOURCE_INCLUDEDIR}/$@ | \
		 ${SED} '1,\%\*/%d;\%/\*[\*\!]%,\%\*/%d') > $@ ; \
	fi

docs-src-stamp: 
	@rm -f ${DOXYGEN_SOURCE_FILES}
	@${MAKE} ${AM_MAKEFLAGS} ${DOXYGEN_SOURCE_FILES}
	@touch ${DOCS_SRC_STAMP}

docs-src: docs-src-stamp
	@rm -f ${DOCS_SRC_STAMP}

