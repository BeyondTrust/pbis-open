# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
##

opensoapincdir = $(includedir)/OpenSOAP

opensoapinc_HEADERS = \
	Block.h \
	ByteArray.h \
	CStdio.h \
	ClientSocket.h \
	Defines.h \
	Envelope.h \
	ErrorCode.h \
	Locale.h \
	OpenSOAP.h \
	Serializer.h \
	Service.h \
	Security.h \
	Stream.h \
	String.h \
	StringHash.h \
	Transport.h \
	XMLAttr.h \
	XMLElm.h \
	XMLNamespace.h

install-exe-hook:
	$(mkinstalldirs) $(DESTDIR)$(opensoapincdir) $(DESTDIR)$(opensoapincdir)/OpenSOAP

