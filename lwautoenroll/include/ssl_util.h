/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef LWAUTOENROLL_SSL_H
#define LWAUTOENROLL_SSL_H

#include <bail.h>

#include <openssl/asn1.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#define BAIL_WITH_SSL_ERROR(_sslError, ...) \
    do { \
        char _SSLerrorString[256]; \
        ERR_error_string_n(_sslError, _SSLerrorString, \
            sizeof(_SSLerrorString)); \
        BAIL_WITH_LW_ERROR(LwSSLErrorToLwError(_sslError), \
            _BAIL_FORMAT_STRING(__VA_ARGS__) ": OpenSSL error %lx (%s)", \
            _BAIL_FORMAT_ARGS(__VA_ARGS__), _sslError, _SSLerrorString); \
    } while(0)

#define BAIL_ON_SSL_ERROR(_expr, ...) \
    do { \
        if (_expr) { \
            unsigned long _sslError = ERR_get_error(); \
            BAIL_WITH_SSL_ERROR(_sslError , ## __VA_ARGS__); \
        } \
    } while(0)

#define BAIL_ON_SSL_CTX_ERROR(_expr, _ctx, ...) \
    do { \
        if (_expr) { \
            BAIL_WITH_SSL_ERROR(X509_STORE_CTX_get_error(_ctx) , \
                ## __VA_ARGS__); \
        } \
    } while(0)

DWORD
LwSSLErrorToLwError(
        unsigned long ssl_error
        );

DWORD
Asn1GeneratePrintfV(
        ASN1_TYPE **ppAsn1Type,
        const char * const format,
        va_list args
        );

DWORD
Asn1GeneratePrintf(
        ASN1_TYPE **ppAsn1Type,
        const char * const format,
        ...);

DWORD
Asn1StackAppendValue(
        STACK_OF(ASN1_TYPE) **ppAsn1Stack,
        ASN1_TYPE *pAsn1Value
        );

DWORD
Asn1StackAppendPrintfV(
        STACK_OF(ASN1_TYPE) **ppAsn1Stack,
        const char * const format,
        va_list args
        );

DWORD
Asn1StackAppendPrintf(
        STACK_OF(ASN1_TYPE) **ppAsn1Stack,
        const char * const format,
        ...
        );

DWORD
Asn1StackToSequence(
        STACK_OF(ASN1_TYPE) *pAsn1Stack,
        ASN1_TYPE **ppAsn1Sequence,
        int type
        );

DWORD
AddExtension(
        STACK_OF(X509_EXTENSION) **extensions,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        X509_EXTENSION *extension
        );

DWORD
AddExtensionAsn1Value(
        STACK_OF(X509_EXTENSION) **extensions,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        PCSTR name,
        ASN1_TYPE *value
        );

DWORD
AddExtensionString(
        STACK_OF(X509_EXTENSION) **extensions,
        X509V3_CTX *pExtensionContext,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        PCSTR name,
        PCSTR value
        );

DWORD
AddExtensionPrintf(
        STACK_OF(X509_EXTENSION) **extensions,
        X509V3_CTX *pExtensionContext,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        PSTR name,
        const char * const format,
        ...
        );

DWORD
BufAppend(
        BUF_MEM *buf,
        PCSTR data,
        int length
        );

#endif /* LWAUTOENROLL_SSL_H */
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
