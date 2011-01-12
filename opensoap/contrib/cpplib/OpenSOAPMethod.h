/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPMethod.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_CPPLIB_OPENSOAPMETHOD_H
#define OpenSOAP_CPPLIB_OPENSOAPMETHOD_H

#include <OpenSOAP/Envelope.h>
#include "OpenSOAPObject.h"
#include "OpenSOAPXMLElm.h"
#include "OpenSOAPBlock.h"
#include "OpenSOAPEnvelope.h"
#include "OpenSOAPTransport.h"
#include "OpenSOAPService.h"
#include <iostream>
#include <memory>

#define DEFAULT_CONNECT_TYPE "cgi"

namespace {
	const std::string DEFAULT_RESPONSE_SUFFIX = "Response";
};

namespace COpenSOAP {

class Method : public Object
{
public:
	std::string GetObjectName() const { return "Method"; }

	Method() : posEnvelopeOut(0) {}
	virtual ~Method() {}

	void PrintEnvelopeTo(std::ostream* pos)
	{
		posEnvelopeOut = pos;
	}

protected:
	virtual std::string GetMethodName() = 0;
	virtual std::string GetResponseName()
	{
		return GetMethodName()+DEFAULT_RESPONSE_SUFFIX;
	}
	virtual std::string GetNamespaceURI() = 0;
	virtual std::string GetNamespacePrefix() 
	{ return ""; }

	virtual std::string GetSOAPVersion()
	{ return "1.1"; }
	virtual std::string GetSOAPPrefix()
	{ return "SOAP-ENV"; }
	virtual std::string GetSOAPAction() 
	{ return ""; }

	virtual void PrepareRequestEnvelope(Envelope& req) {
	}
	void DefineXMLSchema(Envelope& req)
	{
		req.SetAttributeString("SOAP-ENV:encodingStyle"
			,"http://schemas.xmlsoap.org/soap/encoding/");
		req.SetAttributeString("xmlns:xsd"
			,"http://www.w3.org/2001/XMLSchema" );
		req.SetAttributeString("xmlns:xsi"
			,"http://www.w3.org/2001/XMLSchema-instance" );
	}

	std::ostream* posEnvelopeOut;
};

template<class STRUCTURE_IN, class STRUCTURE_OUT, class METHOD> class ServiceMethod : public METHOD
{
	typedef void (*SERVICE_FUNC)(void);
	
	friend class Service;
public:
	std::string GetObjectName() { return "ServiceMethod"; }

	ServiceMethod() {}
	virtual ~ServiceMethod() {}

	void Run(const std::string& connect_type = DEFAULT_CONNECT_TYPE)
	{
		Service srv(GetMethodName(), connect_type, 0);
		srv.Register(*this);
		srv.Run();
	}

	STRUCTURE_IN in;
	STRUCTURE_OUT out;
protected:
	virtual void ExecuteService() ;
private:
	static int ServiceFunc(OpenSOAPEnvelopePtr request, OpenSOAPEnvelopePtr *response, void *pMethodInstance)
	{
		ServiceMethod* pmethod = 
			static_cast<ServiceMethod*>(pMethodInstance);

		// parse reqest
		Envelope req(request);
		Block body = req.GetBodyBlockMB(pmethod->GetMethodName());

		if(pmethod->posEnvelopeOut)
			req.PrintEnvelope(*pmethod->posEnvelopeOut, "Received Request Message");

		pmethod->in.ParseMessage(body);
		// do service
		pmethod->ExecuteService();

		// create response
		Envelope res(pmethod->GetSOAPVersion(), pmethod->GetSOAPPrefix());

		pmethod->PrepareRequestEnvelope(req);

		body = res.AddBodyBlockMB(pmethod->GetResponseName() );
		body.SetNamespace(pmethod->GetNamespaceURI(), pmethod->GetNamespacePrefix());
		pmethod->out.CreateMessage(body);

		if(pmethod->posEnvelopeOut)
			res.PrintEnvelope(*pmethod->posEnvelopeOut, "Created Response Message");

		*response = res.Detach();
		return 0;
	}
private:
	SERVICE_FUNC service_func;
};

template<class STRUCTURE_IN, class STRUCTURE_OUT, class METHOD> class ClientMethod : public METHOD
{
public:

	ClientMethod() {}
	virtual ~ClientMethod() {}

	void SetEndpoint(const std::string& endpoint)
	{
		strEndpoint = endpoint;
	}

	void Invoke() {
		Envelope  req(GetSOAPVersion(), GetSOAPPrefix());
		Transport trans;

		PrepareRequestEnvelope(req);

		Block body = req.AddBodyBlockMB(GetMethodName());

		body.SetNamespace(GetNamespaceURI(), GetNamespacePrefix());
		in.CreateMessage(body);

		if(posEnvelopeOut)
			req.PrintEnvelope(*posEnvelopeOut, 
							GetMethodName()+" Created Request Message");
		
		trans.SetSOAPAction(GetSOAPAction());
		trans.SetURL(strEndpoint);

		std::auto_ptr<Envelope> res(trans.Invoke(req));

		if(posEnvelopeOut)
			res->PrintEnvelope(*posEnvelopeOut, GetMethodName()+" Received Response Message");

		Block fault = res->GetBodyBlockMB("Fault");
		if(fault.Good()) {
			parseErrorCode(fault);
			return;
		}

		body = res->GetBodyBlockMB(GetResponseName());
		out.ParseMessage(body);
	}
	void Invoke(const std::string& endpoint)
	{
		strEndpoint = endpoint;
		Invoke();
	}

	STRUCTURE_IN in;
	STRUCTURE_OUT out;

private:
	void parseErrorCode(XMLElm& fault)
	{
/*		std::string faultcode;
		std::string faultstring;
		std::string faultactor;

		fault.GetChildValue("faultcode", faultcode);
		fault.GetChildValue("faultstring", faultstring);
		fault.GetChildValue("faultactor", faultactor);

		cout << "Server replied fault message." << endl;
		cout << "faultcode  : "<< faultcode << endl;
		cout << "faultstring: "<< faultstring << endl;
		cout << "faultactor : "<< faultactor << endl;*/
	}

	std::string strEndpoint;
};

};// COpenSOAP
#endif //OpenSOAP_CPPLIB_OPENSOAPMETHOD_H
