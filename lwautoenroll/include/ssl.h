#ifndef LWAUTOENROLL_SSL_H
#define LWAUTOENROLL_SSL_H

#include <openssl/asn1.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

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
        PCSTR value,
        int length
        );

#endif /* LWAUTOENROLL_SSL_H */
