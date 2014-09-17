/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPTransport.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "OpenSOAPTransport.h"
#include "OpenSOAPEnvelope.h"

namespace COpenSOAP {

Transport::Transport()
{
	OpenSOAPTransportCreate(&pTransport);
}

Transport::~Transport()
{
	OpenSOAPTransportRelease(pTransport);
}

///////method
void Transport::SetCharset(const std::string& charset)
{
	OpenSOAPTransportSetCharset(pTransport, charset.c_str());
}

void Transport::SetSOAPAction(const std::string& soap_action)
{
	OpenSOAPTransportSetSOAPAction(pTransport, soap_action.c_str());
}

void Transport::SetURL(const std::string& url)
{
	OpenSOAPTransportSetURL(pTransport, url.c_str());
}

Envelope* Transport::Invoke(const Envelope& request)
{
	OpenSOAPEnvelopePtr response;
	Try(OpenSOAPTransportInvoke(pTransport, request.GetOpenSOAPEnvelopePtrConst(), &response));

	return new Envelope(response);//cresponse;
}

}; //COpenSOAP
