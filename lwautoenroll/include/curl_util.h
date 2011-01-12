#ifndef LWAUTOENROLL_CURL_H
#define LWAUTOENROLL_CURL_H

#include "bail.h"

#include <curl/curl.h>

#include <OpenSOAP/Envelope.h>

#define BAIL_WITH_CURL_ERROR(_curlError) \
    do { \
        _BAIL_WITH_LW_ERROR(LwCurlErrorToLwError(_curlError), \
                ": cURL error %d (%s)", _curlError, \
                curl_easy_strerror(_curlError)); \
    } while(0)

#define BAIL_ON_CURL_ERROR(_curlError) \
    do { \
        if (_curlError != CURLE_OK) { \
            BAIL_WITH_CURL_ERROR(_curlError); \
        } \
    } while(0)

DWORD
LwCurlErrorToLwError(
        IN CURLcode curlError
        );

DWORD
LwAutoEnrollCurlSoapRequest(
        IN const char *url,
        IN const OpenSOAPEnvelopePtr pSoapRequest,
        OUT OpenSOAPEnvelopePtr *ppSoapReply
        );

#endif /* LWAUTOENROLL_CURL_H */
