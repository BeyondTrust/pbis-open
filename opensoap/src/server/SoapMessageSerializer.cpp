/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapMessageSerializer.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <string>
#include <vector>
#include <iostream>

//for libxml2
#include <libxml/tree.h>

#include "SoapMessageSerializer.h"
#include "DataRetainer.h"
#include "StringUtil.h"

using namespace std;
using namespace OpenSOAP;

//initialize static member
string SoapMessageSerializer::soapMessageSpoolPath;

SoapMessageSerializer::SoapMessageSerializer()
{
}

SoapMessageSerializer::~SoapMessageSerializer()
{
}

void 
SoapMessageSerializer::deserialize(SoapMessage& soapMessage) const
{
    //
    DataRetainer dr(soapMessageSpoolPath);
    string id = soapMessage.getMessageId();
    if (id.empty()) {
        //throw exception
    }
    dr.SetId(id);
    //dr.Decompose();
    string sm;
    dr.GetSoapEnvelope(sm);
    //set soapmessage
    soapMessage.deserialize(sm);
}

void 
SoapMessageSerializer::deserialize(SoapMessage& soapMessage,
                                   const string& id) const
{
    //
    DataRetainer dr(soapMessageSpoolPath);
    dr.SetId(id);
    //dr.Decompose();
    string sm;
    dr.GetSoapEnvelope(sm);
    //set soapmessage
    soapMessage.deserialize(sm);
    soapMessage.setMessageId(id);
}

void 
SoapMessageSerializer::deserialize(SoapMessage& soapMessage,
                                   const string& id,
                                   map<string,string>& hdrElem) const
{
    //
    DataRetainer dr(soapMessageSpoolPath);
    dr.SetId(id);
    //dr.Decompose();
    string sm;
    dr.GetSoapEnvelope(sm);
    //set soapmessage
    soapMessage.deserialize(sm);
    soapMessage.setMessageId(id);
    map<string,string>::iterator itr;
    for (itr = hdrElem.begin(); itr != hdrElem.end(); itr++) {
        dr.GetHttpHeaderElement(itr->first, itr->second);
    }
}

void 
SoapMessageSerializer::serialize(SoapMessage& soapMessage) const
{
    //
    DataRetainer dr(soapMessageSpoolPath);
    string id = soapMessage.getMessageId();
    if (id.empty()) {
        dr.Create();
        dr.GetId(id);
        soapMessage.setMessageId(id);
    }
    else {
        dr.SetId(id);
    }
    dr.SetSoapEnvelope(soapMessage.toString());
    //dr.Compose();
}

void 
SoapMessageSerializer::serialize(SoapMessage& soapMessage, 
                                 const SoapException& se) const
{
    //
    DataRetainer dr(soapMessageSpoolPath);
    string id = soapMessage.getMessageId();
    if (id.empty()) {
        dr.Create();
        dr.GetId(id);
        soapMessage.setMessageId(id);
    }
    else {
        dr.SetId(id);
    }
    string soapFault(se.createSoapFault());
    dr.SetSoapEnvelope(soapFault);
    dr.AddHttpHeaderElement("status", 
                            StringUtil::toString(se.getHttpStatusCode()));
    //dr.Compose();
    //update soapmessage
    soapMessage.deserialize(soapFault);

}

void 
SoapMessageSerializer::serialize(SoapMessage& soapMessage, 
                                 const string& soapEnvelopeStr) const
{
    //
    DataRetainer dr(soapMessageSpoolPath);
    string id = soapMessage.getMessageId();
    if (id.empty()) {
        dr.Create();
        dr.GetId(id);
        soapMessage.setMessageId(id);
    }
    else {
        dr.SetId(id);
    }
    dr.SetSoapEnvelope(soapEnvelopeStr);
    //dr.Compose();
    //update soapmessage
    soapMessage.deserialize(soapEnvelopeStr);
}

void 
SoapMessageSerializer::serialize(SoapMessage& soapMessage, 
                                 const string& soapEnvelopeStr,
                                 const map<string,string>& hdrElem) const
{
    //
    DataRetainer dr(soapMessageSpoolPath);
    string id = soapMessage.getMessageId();
    if (id.empty()) {
        dr.Create();
        dr.GetId(id);
        soapMessage.setMessageId(id);
    }
    else {
        dr.SetId(id);
    }
    dr.SetSoapEnvelope(soapEnvelopeStr);
    map<string,string>::const_iterator itr;
    for(itr = hdrElem.begin(); itr != hdrElem.end(); itr++) {
        dr.AddHttpHeaderElement(itr->first, itr->second);
    }
    //dr.Compose();
    //update soapmessage
    soapMessage.deserialize(soapEnvelopeStr);
}

// End of SoapMessageSerializer.cpp
