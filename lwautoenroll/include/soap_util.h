/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef LWAUTOENROLL_SOAP_H
#define LWAUTOENROLL_SOAP_H

#include <bail.h>

#include <lwerror.h>

#include <openssl/x509v3.h>

#include <OpenSOAP/Envelope.h>

#define BAIL_WITH_SOAP_ERROR(_soapError, ...) \
    do { \
        BAIL_WITH_LW_ERROR(LwSoapErrorToLwError(_soapError), \
            _BAIL_FORMAT_STRING(__VA_ARGS__) ": OpenSOAP error 0x%08x", \
            _BAIL_FORMAT_ARGS(__VA_ARGS__), _soapError); \
    } while (0)

#define BAIL_ON_SOAP_ERROR(_soapError, ...) \
    do { \
        if (_soapError != OPENSOAP_NO_ERROR) { \
            BAIL_WITH_SOAP_ERROR(_soapError , ## __VA_ARGS__); \
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

DWORD
GenerateSoapStatusRequest(
        IN const char *url,
        IN int requestId,
        OUT OpenSOAPEnvelopePtr *ppSoapRequest
        );

#endif /* LWAUTOENROLL_SOAP_H */
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
