/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPTransport.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(OpenSOAP_CPPLIB_OPENSOAPTRANSPORT_H)
#define OpenSOAP_CPPLIB_OPENSOAPTRANSPORT_H

#include <OpenSOAP/Transport.h>
#include "OpenSOAPObject.h"

#include <string>

namespace COpenSOAP {

class Envelope;
class Transport : public Object
{
public:
	std::string GetObjectName() const { return "Transport"; }
	Transport();
	virtual ~Transport();

	void SetCharset(const std::string& charset);
	void SetSOAPAction(const std::string& soapAction);
	void SetURL(const std::string& url);
	Envelope* Invoke(const Envelope& request);
private:
	OpenSOAPTransportPtr pTransport;
};

};// COpenSOAP
#endif // !defined(OpenSOAP_CPPLIB_OPENSOAPTRANSPORT_H)
