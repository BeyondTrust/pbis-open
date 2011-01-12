/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapException.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <unistd.h>

#include "SoapException.h"
#include "SoapMessage.h"

using namespace std;
using namespace OpenSOAP;

string SoapException::myUri;

void
SoapException::initMyUri()
{
    string uri("http://");

    size_t len = 128;  
    char* hostname = new char[len];
    int ret = gethostname(hostname, len);
    if (ret < 0) {
        uri += "localhost";
    }
    else {
        uri += hostname;

#if 0 //temp
        hostent* hostEntPtr = NULL;
        hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
        cerr << "hostEntPtr->h_name = " << hostEntPtr->h_name << endl;
#endif
        uri = hostEntPtr->h_name;
#endif //if 0 //temp
    }
    //next step : get end from server.conf or httpd.conf ?
    delete[] hostname;

    uri += "/opensoap";

    setMyUri(uri);
}


SoapException::SoapException(int errcode 
                             ,int detailcode
                             ,int level
                             ,const string& modname
                             ,int id) 
    : Exception(errcode, detailcode, level, modname, id)
    , soapHeaderRef(NULL)
    , httpStatusCode(500)
{
    
}

SoapException::SoapException(int errcode 
                             ,int detailcode
                             ,int level
                             ,const string& modname
                             ,int id
                             ,const SoapMessage* soapMessage) 
    //,const SoapMessage& soapMessage) 
    : Exception(errcode, detailcode, level, modname, id)
    , soapHeaderRef(soapMessage)
    , httpStatusCode(500)
{
    
}

SoapException::SoapException(const Exception& e) 
    : Exception(e)
    , soapHeaderRef(NULL)
    , httpStatusCode(500)
{
}

SoapException::SoapException(const SoapException& se) 
    : Exception(se)
    , soapHeaderRef(se.soapHeaderRef)
    , httpStatusCode(se.httpStatusCode)
{
    faultCode = se.faultCode;
    faultString = se.faultString;
    faultActor = se.faultActor;
    detail = se.detail;
}

SoapException::~SoapException() 
{

}

//soap-header referance
void 
//SoapException::setSoapHeaderRef(const SoapMessage& soapMessage)
SoapException::setSoapHeaderRef(const SoapMessage* soapMessage)
{
    soapHeaderRef = soapMessage;
}

//code setter
void 
SoapException::setFaultCode(const std::string& aFaultCode)
{
    faultCode = aFaultCode;
}
void 
SoapException::setFaultString(const std::string& aFaultString)
{
    faultString = aFaultString;
}
void 
SoapException::setFaultActor(const std::string& aFaultActor)
{
    faultActor = aFaultActor;
}
void 
SoapException::setDetail(const Detail& aDetail)
{
    detail = aDetail;
}

//code getter
const std::string& 
SoapException::getFaultCode() const
{
    return faultCode;
}
const std::string& 
SoapException::getFaultString() const
{
    return faultString;
}
const std::string& 
SoapException::getFaultActor() const
{
    return faultActor;
}

const SoapException::Detail& 
SoapException::getDetail() const
{
    return detail;
}

std::string 
SoapException::createSoapFault() const
{
    
    if (soapHeaderRef) {
        return soapHeaderRef->createSoapFault(*this);
    }
    else {
        SoapMessage sm;
        return sm.createSoapFault(*this);
    }
}

// End of SoapException.cpp
