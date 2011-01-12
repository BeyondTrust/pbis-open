/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapMessage.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <string>
#include "SoapMessage.h"
#include "SoapException.h"
#include "XmlModifier.h"
#include "SOAPMessageFunc.h"

using namespace std;
using namespace OpenSOAP;

static char SOAP_ENV_URI[] = "http://schemas.xmlsoap.org/soap/envelope/";
static char SOAP_ENV_DEFPREFIX[] = "SOAP-ENV";


SoapMessage::SoapMessage() {}
SoapMessage::~SoapMessage() {}

void
SoapMessage::parse()
{
    XmlDoc::parse();
    
}        

void 
SoapMessage::setMessageId(const string& mid)
{
    messageId = mid; 
}

const string& 
SoapMessage::getMessageId() const 
{ 
    return messageId; 
}

string 
SoapMessage::createSoapFault(const string& faultCode,
                             const string& faultString,
                             const string& faultActor,
                             const string& detail) const
{
    string soapMessage(toString());
    
    if (soapMessage.empty()) {
        return createSOAPFaultMessage(faultCode, 
                                      faultString,
                                      faultActor,
                                      detail);
    }
    try {
        //copy original soap-message
        XmlModifier xmlMod(soapMessage);
        
        //delete Body block
        int ret = 0;
        string fmt;
        string val;
        
        //clear Body
        fmt = "/Envelope/Body=?";
        ret = xmlMod.del(fmt);

        //Body
        fmt = "/Envelope/Body=?";
        val = "";
        ret = xmlMod.append(fmt, val, NULL);
        
        //Fault
        fmt = "/Envelope/Body/Fault=?";
        val = "";
        ret = xmlMod.attach(fmt, val, NULL);
        
        //faultcode
        fmt = "/Envelope/Body/Fault/faultcode=?";
        val = faultCode;
        ret = xmlMod.append(fmt, val, NULL);
        
        //faultstring
        fmt = "/Envelope/Body/Fault/faultstring=?";
        
        val = faultString;
        ret = xmlMod.append(fmt, val, NULL);
        
        if (!faultActor.empty()) {
            //faultactor
            fmt = "/Envelope/Body/Fault/faultactor=?";
            
            val = faultActor;
            ret = xmlMod.append(fmt, val, NULL);
        }
        
        //detail
        fmt = "/Envelope/Body/Fault/detail=?";
        val = detail;
        ret = xmlMod.append(fmt, val, NULL);
        
        return xmlMod.toString();
    }
    catch(...) {
        return createSOAPFaultMessage(faultCode, 
                                      faultString,
                                      faultActor,
                                      detail);
    }
    //xmlMod.update(query, StringUtil::toString(inChange));
}


string 
SoapMessage::createSoapFault(const SoapException& se) const
{
    return createSoapFault(se.getFaultCode(),
                           se.getFaultString(),
                           se.getFaultActor(),
                           se.getDetail().toString());
}


// End of SoapMessage.cpp
