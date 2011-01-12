/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <soap_util.h>
#include <ssl_util.h>

#include <lwstr.h>

#include <OpenSOAP/XMLElm.h>
#include <OpenSOAP/Envelope.h>
#include <OpenSOAP/Block.h>
#include <OpenSOAP/OpenSOAP.h>

#include <openssl/pem.h>

#include <uuid/uuid.h>

DWORD
LwSoapErrorToLwError(
        IN int soapError
        )
{
    switch (soapError)
    {
        case OPENSOAP_NO_ERROR:
            return LW_ERROR_SUCCESS;

        case OPENSOAP_IMPLEMENTATION_ERROR:
            return LW_ERROR_INTERNAL;

        case OPENSOAP_YET_IMPLEMENTATION:
            return LW_ERROR_NOT_IMPLEMENTED;

        case OPENSOAP_UNSUPPORT_PROTOCOL:
            return LW_ERROR_NOT_SUPPORTED;

        case OPENSOAP_PARAMETER_BADVALUE:
            return LW_ERROR_INVALID_PARAMETER;

        case OPENSOAP_MEM_ERROR:
        case OPENSOAP_MEM_BADALLOC:
            return LW_ERROR_OUT_OF_MEMORY;

        case OPENSOAP_MEM_OUTOFRANGE:
            return ERROR_INVALID_ACCESS;

        case OPENSOAP_CHAR_ERROR:
        case OPENSOAP_ICONV_NOT_IMPL:
        case OPENSOAP_INVALID_MB_SEQUENCE:
        case OPENSOAP_INCOMPLETE_MB_SEQUENCE:
        case OPENSOAP_UNKNOWN_CHARENCODE:
            return LW_ERROR_STRING_CONV_FAILED;

        case OPENSOAP_IO_READ_ERROR:
        case OPENSOAP_IO_WRITE_ERROR:
        case OPENSOAP_FILE_ERROR:
            return LW_ERROR_GENERIC_IO;

        case OPENSOAP_FILEOPEN_ERROR:
            return ERROR_FILE_NOT_FOUND;

        case OPENSOAP_TRANSPORT_HOST_NOT_FOUND:
            return WSAHOST_NOT_FOUND;

        case OPENSOAP_TRANSPORT_CONNECTION_REFUSED:
            return ERROR_CONNECTION_REFUSED;

        case OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT:
            return WSAETIMEDOUT;

        case OPENSOAP_TRANSPORT_NETWORK_UNREACH:
            return WSAENETUNREACH;

        case OPENSOAP_TRANSPORT_HOST_UNREACH:
            return WSAEHOSTUNREACH;

        case OPENSOAP_USERDEFINE_ERROR:
        case OPENSOAP_XML_ERROR:
        case OPENSOAP_XMLNODE_NOT_FOUND:
        case OPENSOAP_XML_BADNAMESPACE:
        case OPENSOAP_XML_NOHEADERBODY:
        case OPENSOAP_XML_BADDOCUMENTTYPE:
        case OPENSOAP_XML_BADMAKEDOCUMENT:
        case OPENSOAP_XML_EMPTYDOCUMENT:
        case OPENSOAP_XML_NOTXMLDOCUMENT:
        case OPENSOAP_XML_NS_URI_UNMATCHED:
        case OPENSOAP_SEC_ERROR:
        case OPENSOAP_SEC_KEYGEN_ERROR:
        case OPENSOAP_SEC_SIGNGEN_ERROR:
        case OPENSOAP_SEC_SIGNVERIFY_ERROR:
        case OPENSOAP_SEC_ENCRYPT_ERROR:
        case OPENSOAP_SEC_DECRYPT_ERROR:
        case OPENSOAP_TRANSPORT_ERROR:
        case OPENSOAP_TRANSPORT_INVOKE_ERROR:
        case OPENSOAP_TRANSPORT_HTTP_ERROR:
        case OPENSOAP_TRANSPORT_SSL_ERROR:
        case OPENSOAP_TRANSPORT_SSL_VERSION_ERROR:
        case OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR:
            return LW_ERROR_UNKNOWN;
    }

    return LW_ERROR_UNKNOWN;
}

static DWORD
LwOpenSOAPBlockAddAttributeCString(
        IN OpenSOAPBlockPtr pSoapBlock,
        IN const char *name,
        IN const char *value,
        OUT OpenSOAPXMLAttrPtr *ppSoapAttribute
        )
{
    OpenSOAPStringPtr pSoapString = NULL;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPStringCreateWithMB(value, &pSoapString);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPBlockAddAttributeMB(
                    pSoapBlock,
                    name,
                    "string",
                    &pSoapString,
                    ppSoapAttribute);
    BAIL_ON_SOAP_ERROR(soapResult);

cleanup:
    if (pSoapString)
    {
        OpenSOAPStringRelease(pSoapString);
    }

    return error;
}

static DWORD
LwOpenSOAPBlockSetValueCString(
        IN OpenSOAPBlockPtr pSoapBlock,
        const char *value
        )
{
    OpenSOAPStringPtr pSoapString = NULL;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPStringCreateWithMB(value, &pSoapString);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPBlockSetValueMB(
                    pSoapBlock,
                    "string",
                    &pSoapString);
    BAIL_ON_SOAP_ERROR(soapResult);

cleanup:
    if (pSoapString)
    {
        OpenSOAPStringRelease(pSoapString);
    }

    return error;
}

static DWORD
LwOpenSOAPXMLElmAddAttributeCString(
        IN OpenSOAPXMLElmPtr pSoapElement,
        IN const char *name,
        IN const char *value,
        OUT OpenSOAPXMLAttrPtr *ppSoapAttribute
        )
{
    OpenSOAPStringPtr pSoapString = NULL;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPStringCreateWithMB(value, &pSoapString);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPXMLElmAddAttributeMB(
                    pSoapElement,
                    name,
                    "string",
                    &pSoapString,
                    ppSoapAttribute);
    BAIL_ON_SOAP_ERROR(soapResult);

cleanup:
    if (pSoapString)
    {
        OpenSOAPStringRelease(pSoapString);
    }

    return error;
}

static DWORD
LwOpenSOAPXMLElmSetValueCString(
        IN OpenSOAPXMLElmPtr pSoapElement,
        const char *value
        )
{
    OpenSOAPStringPtr pSoapString = NULL;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPStringCreateWithMB(value, &pSoapString);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPXMLElmSetValueMB(
                    pSoapElement,
                    "string",
                    &pSoapString);
    BAIL_ON_SOAP_ERROR(soapResult);

cleanup:
    if (pSoapString)
    {
        OpenSOAPStringRelease(pSoapString);
    }

    return error;
}

static DWORD
LwOpenSOAPXMLElmSetValueInt(
        IN OpenSOAPXMLElmPtr pSoapElement,
        int value
        )
{
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPXMLElmSetValueMB(
                    pSoapElement,
                    "int",
                    &value);
    BAIL_ON_SOAP_ERROR(soapResult);

cleanup:
    return error;
}

DWORD
GenerateSoapRequest(
        IN const char *url,
        IN X509_REQ *pX509Request,
        OUT OpenSOAPEnvelopePtr *ppSoapRequest
        )
{
    OpenSOAPEnvelopePtr pSoapRequest = NULL;
    OpenSOAPBlockPtr pSoapBlock = NULL;
    OpenSOAPXMLElmPtr pSoapElement = NULL;
    BIO *pRequestBio = NULL;
    BUF_MEM *pRequestBuf = NULL;
    PSTR x509RequestStr = NULL;
    int x509RequestLen = 0;
    uuid_t uuid = { 0 };
    char uuidString[37];
    PSTR uuidAttribute = NULL;
    int sslResult = 0;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    pRequestBio = BIO_new(BIO_s_mem());
    BAIL_ON_SSL_ERROR(pRequestBio == NULL);

    sslResult = PEM_write_bio_X509_REQ(pRequestBio, pX509Request);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sslResult = BIO_get_mem_ptr(pRequestBio, &pRequestBuf);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    // Skip the first and last lines of the PEM certificate request.
    x509RequestStr = strchr(pRequestBuf->data, '\n');
    if (x509RequestStr == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_PARAMETER,
            "Invalid certificate request");
    }
    x509RequestLen = pRequestBuf->length - (x509RequestStr - pRequestBuf->data);

    --x509RequestLen; // Skip trailing newline
    do
    {
        --x509RequestLen;
    }
    while (x509RequestStr[x509RequestLen] != '\n');

    x509RequestStr[x509RequestLen + 1] = '\0';

    soapResult = OpenSOAPInitialize(NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeCreateMB("1.2", "s", &pSoapRequest);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeDefineNamespaceMB(
                    pSoapRequest,
                    "http://www.w3.org/2005/08/addressing",
                    "a",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeDefineNamespaceMB(
                    pSoapRequest,
                    "http://www.w3.org/2001/XMLSchema-instance",
                    "xsi",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeDefineNamespaceMB(
                    pSoapRequest,
                    "http://www.w3.org/2001/XMLSchema",
                    "xsd",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:Action",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPBlockAddAttributeCString(
                pSoapBlock,
                "s:mustUnderstand",
                "1",
                NULL);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPBlockSetValueCString(
                pSoapBlock,
                "http://schemas.microsoft.com/windows/pki/2009/01/enrollment/RST/wstep");
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:MessageID",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    uuid_generate(uuid);
    uuid_unparse(uuid, uuidString);

    error = LwAllocateStringPrintf(
                &uuidAttribute,
                "urn:uuid:%s",
                uuidString);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPBlockSetValueCString(
                pSoapBlock,
                uuidAttribute);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:ReplyTo",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "a:Address",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmSetValueCString(
                pSoapElement,
                "http://www.w3.org/2005/08/addressing/anonymous");
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:To",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPBlockAddAttributeCString(
                pSoapBlock,
                "s:mustUnderstand",
                "1",
                NULL);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPBlockSetValueCString(pSoapBlock, url);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddBodyBlockMB(
                    pSoapRequest,
                    "RequestSecurityToken",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPBlockSetNamespaceMB(
                    pSoapBlock,
                    "http://docs.oasis-open.org/ws-sx/ws-trust/200512",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "TokenType",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmSetValueCString(
                pSoapElement,
                "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509v3");
    BAIL_ON_LW_ERROR(error);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "RequestType",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmSetValueCString(
                pSoapElement,
                "http://docs.oasis-open.org/ws-sx/ws-trust/200512/Issue");
    BAIL_ON_LW_ERROR(error);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "BinarySecurityToken",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPXMLElmSetNamespaceMB(
                    pSoapElement,
                    "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmAddAttributeCString(
                pSoapElement,
                "EncodingType",
                "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd#base64binary",
                NULL);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPXMLElmAddAttributeCString(
                pSoapElement,
                "ValueType",
                "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd#PKCS7",
                NULL);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPXMLElmSetValueCString(
                pSoapElement,
                x509RequestStr);
    BAIL_ON_LW_ERROR(error);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "RequestID",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmAddAttributeCString(
                pSoapElement,
                "xsi:nil",
                "true",
                NULL);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPXMLElmSetNamespaceMB(
                    pSoapElement,
                    "http://schemas.microsoft.com/windows/pki/2009/01/enrollment",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

cleanup:
    if (error)
    {
        if (pSoapRequest)
        {
            OpenSOAPEnvelopeRelease(pSoapRequest);
            pSoapRequest = NULL;
        }
    }

    LW_SAFE_FREE_STRING(uuidAttribute);

    if (pRequestBio)
    {
        BIO_free_all(pRequestBio); 
    }

    *ppSoapRequest = pSoapRequest;
    return error;
}

DWORD
GenerateSoapStatusRequest(
        IN const char *url,
        IN int requestId,
        OUT OpenSOAPEnvelopePtr *ppSoapRequest
        )
{
    OpenSOAPEnvelopePtr pSoapRequest = NULL;
    OpenSOAPBlockPtr pSoapBlock = NULL;
    OpenSOAPXMLElmPtr pSoapElement = NULL;
    uuid_t uuid = { 0 };
    char uuidString[37];
    PSTR uuidAttribute = NULL;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPInitialize(NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeCreateMB("1.2", "s", &pSoapRequest);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeDefineNamespaceMB(
                    pSoapRequest,
                    "http://www.w3.org/2005/08/addressing",
                    "a",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeDefineNamespaceMB(
                    pSoapRequest,
                    "http://www.w3.org/2001/XMLSchema-instance",
                    "xsi",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeDefineNamespaceMB(
                    pSoapRequest,
                    "http://www.w3.org/2001/XMLSchema",
                    "xsd",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:Action",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPBlockAddAttributeCString(
                pSoapBlock,
                "s:mustUnderstand",
                "1",
                NULL);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPBlockSetValueCString(
                pSoapBlock,
                "http://schemas.microsoft.com/windows/pki/2009/01/enrollment/RST/wstep");
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:MessageID",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    uuid_generate(uuid);
    uuid_unparse(uuid, uuidString);

    error = LwAllocateStringPrintf(
                &uuidAttribute,
                "urn:uuid:%s",
                uuidString);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPBlockSetValueCString(
                pSoapBlock,
                uuidAttribute);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:ReplyTo",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "a:Address",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmSetValueCString(
                pSoapElement,
                "http://www.w3.org/2005/08/addressing/anonymous");
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddHeaderBlockMB(
                    pSoapRequest,
                    "a:To",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPBlockAddAttributeCString(
                pSoapBlock,
                "s:mustUnderstand",
                "1",
                NULL);
    BAIL_ON_LW_ERROR(error);

    error = LwOpenSOAPBlockSetValueCString(pSoapBlock, url);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeAddBodyBlockMB(
                    pSoapRequest,
                    "RequestSecurityToken",
                    &pSoapBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPBlockSetNamespaceMB(
                    pSoapBlock,
                    "http://docs.oasis-open.org/ws-sx/ws-trust/200512",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "TokenType",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmSetValueCString(
                pSoapElement,
                "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509v3");
    BAIL_ON_LW_ERROR(error);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "RequestType",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmSetValueCString(
                pSoapElement,
                "http://schemas.microsoft.com/windows/pki/2009/01/enrollment/QueryTokenStatus");
    BAIL_ON_LW_ERROR(error);

    pSoapElement = NULL;
    soapResult = OpenSOAPBlockAddChildMB(
                    pSoapBlock,
                    "RequestID",
                    &pSoapElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = LwOpenSOAPXMLElmSetValueInt(
                pSoapElement,
                requestId);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPXMLElmSetNamespaceMB(
                    pSoapElement,
                    "http://schemas.microsoft.com/windows/pki/2009/01/enrollment",
                    NULL);
    BAIL_ON_SOAP_ERROR(soapResult);

cleanup:
    if (error)
    {
        if (pSoapRequest)
        {
            OpenSOAPEnvelopeRelease(pSoapRequest);
            pSoapRequest = NULL;
        }
    }

    LW_SAFE_FREE_STRING(uuidAttribute);

    *ppSoapRequest = pSoapRequest;
    return error;
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
