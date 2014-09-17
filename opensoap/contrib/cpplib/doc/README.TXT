OpenSOAP C++ library
====================

This library implements part of the OpenSOAP API as C++ class wrappers.
 1. Module construction/destruction is automated.
 2. SOAP message creation/analysis is more easily implemented.

Using this class library should result in a reduction of the amount of coding
required in comparison to coding in C.

However, this library does not have support for the complete API.


File composition

OpenSOAPBlock.h		
OpenSOAPByteArray.h	
OpenSOAPEnvelope.h	
OpenSOAPService.h	
OpenSOAPString.h	
OpenSOAPTransport.h	
OpenSOAPXMLElm.h	

The C++ OpenSOAP API wrapper classes have the same names as the OpenSOAP API
C files.

OpenSOAPInitializer.h	OpenSOAP Initialisation, Termination
OpenSOAPMethod.h        SOAP Message Generation, analysis
OpenSOAPObject.h        Base Class for all classes
OpenSOAPStructure.h     Base Class for structures


samples/

	Client.cpp	Complex Client example source code

	Helloc.cpp	Client for Hello Service source code

	Hellos.cpp	Hello Service source code

	Hello.hpp	Hello Service/Client common source code



Class construction

using namespace COpenSOAP;

Object <- Block
       <- ByteArray
       <- Envelope
       <- Service
       <- String
       <- Transport
       <- XMLElm
       <- Structure(V) <- InStructure(V)
                       <- OutStructure(V)
                       <- EmptyStructure
       <- Method(V)    <- ClientMethod(T)
                       <- ServiceMethod(T)

(V)is a basic abstract class, (T) is a class template


Simple usage:

To call SOAP, firstly, the I/O structure (a derivative of Structure)
is implemented.

Example:

#include "OpenSOAPMethod.h"

using namespace COpenSOAP;
using namespace std;

struct Hello_in : public Structure // Define value passed to server
{
	void CreateMessage(XMLElm& elm) const {
		elm.SetChildValue("name", name);
	}
	void ParseMessage(const XMLElm& elm) {   
		elm.GetChildValue("name", name);
	}

	string name; // std::string can also be passed directly to SetChildValue
};


struct Hello_out : public Structure // Define the value received from the serve
{
	void CreateMessage(XMLElm& elm) const {
		elm.SetChildValue("reply", reply);
	}
	void ParseMessage(const XMLElm& elm) {
		elm.GetChildValue("reply", reply);
	}

	string reply;
};


Implement a Method derivative class

Define the service name (Body Block name), SOAP action, namespace, etc

Example:

class Hello_method : public Method // Override required method
{
public:
	std::string GetSOAPAction()
	{ return ""; }
	std::string GetMethodName()
	{ return "Hello"; }

	std::string GetNamespaceURI()
	{ return "http://opensoap.jp/samples/hello"; }
};


Create a client class using the ClientMethod template

Example:

typedef ClientMethod <Hello_in, Hello_out, Hello_method> Hello;

Specify the endpoint and call the Invoke method

Example:

	Hello he;
	he.SetEndpoint("http://localhost/cgi-bin/hellos.cgi");
	try{
		he.PrintEnvelopeTo(&cout);// Output the envelope contents
		he.Invoke();
	}catch(opensoap_failed e){
		cout << "error " << hex << e.GetErrorCode() 
			<< "@"<< e.GetObjectName() << endl;
	}


Error messages are received through the opensoap_failed exception


On the service side,

Create a service class using the ServiceMethod and implement
void ServiceMethod::ExecuteService().

Example:

typedef ServiceMethod<Hello_in, Hello_out, Hello_method> Hello;

void Hello::ExecuteService()
{
	out.reply = "Hello! " + in.name;
}

Call the Run method

Example:

	try
	{
		Hello he;
		he.Run();
	}
	catch(opensoap_failed e)
	{
		cerr << "errorcode:" << e.GetErrorCode() 
		     <<"@"<< e.GetObjectName() << endl;
	}


  Regarding Structure XMLElm

  It is possible to pass the structure derived from Structure directly to
XMLElm::SetChildValue. The generated message is dependent on the implementation
of the derived Structure class. It is possible to pass the STL vector<T>.

Create the message in the following manner.

		typename std::vector<T>::iterator it;
		for( it=ar.begin(); it<ar.end(); it++) {
			SetChildValue(name, *it);
		}

The derived class requires that
CreateMessage(),ParseMessage()
be implemented.

If only input is used when creating the client then depending
on specific requirements use
InStructure(create only, parse is not required)
OutStructure(parse only, create is not required)
to make any derivative classes.


If no value is required (ie. no elements in the Body Block)
specify EmptyStructure in the parameter list for ClientMethod.

Example:

struct GetDicList_out : public OutStructure
{
	void ParseMessage(const XMLElm& elm) {
		elm.GetChild("DicInfoList").GetChildValue("DicInfo", DicInfoList);
	}
	vector<DicInfo> DicInfoList;
};

class GetDicList_method : public Method
{
public:
	std::string GetSOAPAction()
	{ return "http://btonic.est.co.jp/NetDic/NetDicV06/GetDicList"; }
	std::string GetMethodName()
	{ return "GetDicList"; }

	std::string GetNamespaceURI()
	{ return "http://btonic.est.co.jp/NetDic/NetDicV06"; }

};

typedef ClientMethod<EmptyStructure, GetDicList_out,
			GetDicList_method> GetDicList;

