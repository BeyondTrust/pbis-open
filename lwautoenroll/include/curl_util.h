/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef LWAUTOENROLL_CURL_H
#define LWAUTOENROLL_CURL_H

#include "bail.h"

#include <curl/curl.h>

#include <OpenSOAP/Envelope.h>

#define BAIL_WITH_CURL_ERROR(_curlError, ...) \
    do { \
        BAIL_WITH_LW_ERROR(LwCurlErrorToLwError(_curlError), \
            _BAIL_FORMAT_STRING(__VA_ARGS__) ": cURL error %d (%s)", \
            _BAIL_FORMAT_ARGS(__VA_ARGS__), _curlError, \
            curl_easy_strerror(_curlError)); \
    } while(0)

#define BAIL_ON_CURL_ERROR(_curlError, ...) \
    do { \
        if (_curlError != CURLE_OK) { \
            BAIL_WITH_CURL_ERROR(_curlError , ## __VA_ARGS__); \
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
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
