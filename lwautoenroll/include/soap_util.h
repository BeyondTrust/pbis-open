#ifndef LWAUTOENROLL_SOAP_H
#define LWAUTOENROLL_SOAP_H

#include <bail.h>

#include <lwerror.h>

#include <openssl/x509v3.h>

#include <OpenSOAP/Envelope.h>

#define BAIL_ON_SOAP_ERROR(_soapError) \
    do { \
        if (_soapError != OPENSOAP_NO_ERROR) { \
            _BAIL_WITH_LW_ERROR(LwSoapErrorToLwError(_soapError), \
                    ": OpenSOAP error %08x", _soapError); \
        } \
    } while (0)

DWORD
LwSoapErrorToLwError(
        IN int soapError
        );

DWORD
GenerateSoapRequest(
        IN const char *url,
        IN X509_REQ *pX509Request,
        OUT OpenSOAPEnvelopePtr *ppSoapRequest
        );

#endif /* LWAUTOENROLL_SOAP_H */
