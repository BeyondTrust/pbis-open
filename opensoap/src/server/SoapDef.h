/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapDef.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SoapDef_H
#define SoapDef_H

#include <string>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {

    namespace XMLDef {
        extern const std::string OPENSOAP_VAR
        XMLNS;
    }

    namespace SoapTag {
        extern const std::string OPENSOAP_VAR
        ENVELOPE;
        extern const std::string OPENSOAP_VAR
        HEADER;
        extern const std::string OPENSOAP_VAR
        BODY;
    }
    namespace SoapNamespace {
        extern const std::string OPENSOAP_VAR
        SOAP_ENV;
        extern const std::string OPENSOAP_VAR
        SOAP_ENV_PREFIX;
    }

    namespace ExtSoapHeaderTag {
        extern const std::string OPENSOAP_VAR
        OPENSOAP_HEADER_BLOCK;
        extern const std::string OPENSOAP_VAR
        MESSAGE_ID;
        extern const std::string OPENSOAP_VAR
        ASYNC;
        extern const std::string OPENSOAP_VAR
        FORWARDER;
        extern const std::string OPENSOAP_VAR
        FORWARD_PATH;
        extern const std::string OPENSOAP_VAR
        HOPCOUNT;
        extern const std::string OPENSOAP_VAR
        RECEIVED_PATH;
        extern const std::string OPENSOAP_VAR
        URL;
        extern const std::string OPENSOAP_VAR
        TIME;
        extern const std::string OPENSOAP_VAR
        BACKWARD_PATH;
        extern const std::string OPENSOAP_VAR
        TTL;
        extern const std::string OPENSOAP_VAR
        RESPONSE_MSG;
        extern const std::string OPENSOAP_VAR
        UNDELETE;
        extern const std::string OPENSOAP_VAR
        IN;
    }
    namespace ExtSoapHeaderNamespace {
        extern const std::string OPENSOAP_VAR
        OPENSOAP_HEADER;
        extern const std::string OPENSOAP_VAR
        OPENSOAP_HEADER_PREFIX;
    }
    namespace ExtSoapHeaderAttributes {
        extern const std::string OPENSOAP_VAR
        SECOND;
        extern const std::string OPENSOAP_VAR
        HOPTIMES;
        extern const std::string OPENSOAP_VAR
        ASYNCSECOND;
        extern const std::string OPENSOAP_VAR
        TYPE;
        extern const std::string OPENSOAP_VAR
        COUNT;
    }
    namespace ExtSoapTag {
        extern const std::string OPENSOAP_VAR
        RESULT;
    }
    namespace ExtSoapNamespace {
        extern const std::string OPENSOAP_VAR
        RESULT;
        extern const std::string OPENSOAP_VAR
        RESULT_PREFIX;
    }
    namespace FaultElement {
        extern const std::string OPENSOAP_VAR
        FAULT_CLIENT;
        extern const std::string OPENSOAP_VAR
        FAULT_SERVER;
    }
}

#endif //SoapDef_H
