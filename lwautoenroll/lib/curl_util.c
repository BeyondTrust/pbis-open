/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <curl_util.h>
#include <soap_util.h>

#include <lwerror.h>

#include <unistd.h>

DWORD
LwCurlErrorToLwError(
        CURLcode curlError
        )
{
    switch (curlError)
    {
        case CURLE_OK:
            return LW_ERROR_SUCCESS;

        case CURLE_OUT_OF_MEMORY:
            return LW_ERROR_OUT_OF_MEMORY;

        case CURLE_BAD_FUNCTION_ARGUMENT:
        case CURLE_URL_MALFORMAT:
            return LW_ERROR_INVALID_PARAMETER;

        case CURLE_COULDNT_CONNECT:
        case CURLE_SSL_CONNECT_ERROR:
            return ERROR_CONNECTION_REFUSED;

        case CURLE_WRITE_ERROR:
        case CURLE_READ_ERROR:
        case CURLE_SEND_ERROR:
        case CURLE_RECV_ERROR:
            return LW_ERROR_GENERIC_IO;

        case CURLE_CONV_FAILED:
            return LW_ERROR_STRING_CONV_FAILED;

        default:
            return LW_ERROR_UNKNOWN;
    }

    return LW_ERROR_UNKNOWN;
}

static DWORD
LwAutoEnrollCurlSlistAppend(
        struct curl_slist **ppStringList,
        const char *string
        )
{
    struct curl_slist * pStringItem = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    pStringItem = curl_slist_append(
                    *ppStringList,
                    string);
    if (pStringItem == NULL)
    {
        BAIL_WITH_CURL_ERROR(CURLE_OUT_OF_MEMORY);
    }

    if (*ppStringList == NULL)
    {
        *ppStringList = pStringItem;
    }

cleanup:
    return error;
}

static size_t
CurlSoapReceiveCallback(
        void *data,
        size_t size,
        size_t count,
        void *userInfo
        )
{
    OpenSOAPByteArrayPtr pBuffer = userInfo;
    size_t totalSize = size * count;
    int soapResult = 0;

    soapResult = OpenSOAPByteArrayAppend(pBuffer, data, totalSize);
    if (soapResult != OPENSOAP_NO_ERROR)
    {
        totalSize = 0;
    }

    return totalSize;
}

DWORD
LwAutoEnrollCurlSoapRequest(
        IN const char *url,
        IN const OpenSOAPEnvelopePtr pSoapRequest,
        OUT OpenSOAPEnvelopePtr *ppSoapReply
        )
{
    OpenSOAPByteArrayPtr pSoapRequestBuffer = NULL;
    OpenSOAPByteArrayPtr pSoapReplyBuffer = NULL;
    const unsigned char *soapRequestStr = NULL;
    size_t soapRequestSize = 0;
    OpenSOAPEnvelopePtr pSoapReply = NULL;
    CURL *curlHandle = NULL;
    struct curl_slist *pHeaderList = NULL;
    long responseCode = 0;
    CURLcode curlResult = CURLE_OK;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPByteArrayCreate(&pSoapRequestBuffer);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPEnvelopeGetCharEncodingString(
                    pSoapRequest,
                    "UTF-8",
                    pSoapRequestBuffer);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPByteArrayGetBeginSizeConst(
                    pSoapRequestBuffer,
                    &soapRequestStr,
                    &soapRequestSize);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPByteArrayCreate(&pSoapReplyBuffer);
    BAIL_ON_SOAP_ERROR(soapResult);

    curlHandle = curl_easy_init();
    if (curlHandle == NULL)
    {
        BAIL_WITH_CURL_ERROR(CURLE_OUT_OF_MEMORY);
    }

    curlResult = curl_easy_setopt(curlHandle, CURLOPT_URL, url);
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_POSTFIELDS,
                    soapRequestStr);
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_POSTFIELDSIZE,
                    soapRequestSize);
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_CAPATH,
                    "/var/lib/likewise/trusted_certs");
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_HTTPAUTH,
                    4);
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_USERPWD,
                    ":");
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_WRITEFUNCTION,
                    CurlSoapReceiveCallback);
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_WRITEDATA,
                    pSoapReplyBuffer);
    BAIL_ON_CURL_ERROR(curlResult);

    error = LwAutoEnrollCurlSlistAppend(
                    &pHeaderList,
                    "Content-Type: application/soap+xml; charset=utf-8");
    BAIL_ON_LW_ERROR(error);

    curlResult = curl_easy_setopt(
                    curlHandle,
                    CURLOPT_HTTPHEADER,
                    pHeaderList);
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_perform(curlHandle);
    BAIL_ON_CURL_ERROR(curlResult);

    curlResult = curl_easy_getinfo(
                    curlHandle,
                    CURLINFO_RESPONSE_CODE,
                    &responseCode);
    BAIL_ON_CURL_ERROR(curlResult);

    //
    // Parse the reply even if the HTTP request failed; SOAP faults
    // return a 500 error but have more information in the body
    // of the response.
    //
    soapResult = OpenSOAPEnvelopeCreateCharEncoding(
                    "UTF-8",
                    pSoapReplyBuffer,
                    &pSoapReply);

    //
    // A 500 response with a valid SOAP parse is a SOAP fault; we
    // want to return 0 in that case, so the first error logged
    // will be from the layer above that extracts the reason
    // from the fault response.  Otherwise, we want to check
    // the HTTP response code before the SOAP result, since that's
    // the first (and therefore most meaningful) error.
    //
    if (soapResult != 0 || responseCode != 500)
    {
        if (responseCode != 200)
        {
            //
            // cURL doesn't make the information from the rest of the 
            // error available, unfortunately.
            //
            BAIL_WITH_LW_ERROR(
                LW_ERROR_AUTOENROLL_HTTP_REQUEST_FAILED,
                "HTTP Request failed with code %d",
                responseCode);
        }
        else
        {
            BAIL_ON_SOAP_ERROR(soapResult);
        }
    }

cleanup:
    if (error)
    {
        if (pSoapReply)
        {
            OpenSOAPEnvelopeRelease(pSoapReply);
            pSoapReply = NULL;
        }
    }

    if (pSoapRequestBuffer)
    {
        OpenSOAPByteArrayRelease(pSoapRequestBuffer);
    }

    if (pSoapReplyBuffer)
    {
        OpenSOAPByteArrayRelease(pSoapReplyBuffer);
    }

    if (curlHandle)
    {
        curl_easy_cleanup(curlHandle);
    }

    if (pHeaderList)
    {
        curl_slist_free_all(pHeaderList);
    }

    *ppSoapReply = pSoapReply;
    return error;
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
