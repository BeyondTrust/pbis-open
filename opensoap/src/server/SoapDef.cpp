/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapDef.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "SoapDef.h"

using namespace std;

//XML definitions
const string OpenSOAP::XMLDef::XMLNS = "xmlns";

//SoapTag
const string OpenSOAP::SoapTag::ENVELOPE = "Envelope";
const string OpenSOAP::SoapTag::HEADER = "Header";
const string OpenSOAP::SoapTag::BODY = "Body";

//SoapNamespace
const string OpenSOAP::SoapNamespace::SOAP_ENV = 
"http://schemas.xmlsoap.org/soap/envelope/";
const string OpenSOAP::SoapNamespace::SOAP_ENV_PREFIX = "SOAP-ENV";

//ExtSoapHeaderTag
const string OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK =
"opensoap-header-block";
const string OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID = 
"message_id";
const string OpenSOAP::ExtSoapHeaderTag::ASYNC = 
"async";
const string OpenSOAP::ExtSoapHeaderTag::FORWARDER =
"forwarder";
const string OpenSOAP::ExtSoapHeaderTag::FORWARD_PATH =
"forward_path";
const string OpenSOAP::ExtSoapHeaderTag::HOPCOUNT =
"hopcount";
const string OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH =
"received_path";
const string OpenSOAP::ExtSoapHeaderTag::URL =
"url";
const string OpenSOAP::ExtSoapHeaderTag::TIME =
"time";
const string OpenSOAP::ExtSoapHeaderTag::BACKWARD_PATH =
"backward_path";
const string OpenSOAP::ExtSoapHeaderTag::TTL =
"ttl";
const string OpenSOAP::ExtSoapHeaderTag::RESPONSE_MSG =
"response_msg";
const string OpenSOAP::ExtSoapHeaderTag::UNDELETE =
"undelete";
const string OpenSOAP::ExtSoapHeaderTag::IN = 
"in";

//ExtSoapHeaderNamespace
const string OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER =
"http://header.opensoap.jp/1.0/";
const string OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER_PREFIX =
"opensoap-header";

//ExtSoapHeaderAttributes
const string OpenSOAP::ExtSoapHeaderAttributes::SECOND =
"second";
const string OpenSOAP::ExtSoapHeaderAttributes::HOPTIMES =
"hoptimes";
const string OpenSOAP::ExtSoapHeaderAttributes::ASYNCSECOND =
"async-second";
const string OpenSOAP::ExtSoapHeaderAttributes::TYPE = 
"type";
const string OpenSOAP::ExtSoapHeaderAttributes::COUNT =
"count";

//ExtSoapTag
const string OpenSOAP::ExtSoapTag::RESULT = "Result";

//ExtSoapNamespace
const string OpenSOAP::ExtSoapNamespace::RESULT = 
"http://async.opensoap.jp/1.0/";
const string OpenSOAP::ExtSoapNamespace::RESULT_PREFIX =
"r";

//FaultElement
const string OpenSOAP::FaultElement::FAULT_CLIENT = "Client";
const string OpenSOAP::FaultElement::FAULT_SERVER = "Server";

// End of SoapDef.h
