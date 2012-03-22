/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <bail.h>
#include <ssl_util.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <lwerror.h>
#include <lwmem.h>
#include <lwstr.h>

#include <stdarg.h>
#include <string.h>

DWORD
LwSSLErrorToLwError(unsigned long ssl_error)
{
    switch (ERR_GET_REASON(ssl_error))
    {
        case ERR_R_MALLOC_FAILURE:
            return LW_ERROR_OUT_OF_MEMORY;

        case ERR_R_PASSED_NULL_PARAMETER:
            return LW_ERROR_INVALID_PARAMETER;

        case ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED:
        case ERR_R_INTERNAL_ERROR:
            return LW_ERROR_INTERNAL;

        default:
            return LW_ERROR_UNKNOWN;
    }
}

DWORD
Asn1GeneratePrintfV(
        ASN1_TYPE **ppAsn1Type,
        const char * const format,
        va_list args
        )
{
    ASN1_TYPE *pAsn1Type = NULL;
    PSTR asn1String = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    error = LwAllocateStringPrintfV(
                &asn1String,
                format,
                args);
    BAIL_ON_LW_ERROR(error);

    pAsn1Type = ASN1_generate_nconf(asn1String, NULL);
    BAIL_ON_SSL_ERROR(pAsn1Type == NULL);

cleanup:
    if (error)
    {
        if (pAsn1Type)
        {
            ASN1_TYPE_free(pAsn1Type);
            pAsn1Type = NULL;
        }
    }

    LW_SAFE_FREE_STRING(asn1String);

    *ppAsn1Type = pAsn1Type;
    return error;
}

DWORD
Asn1GeneratePrintf(
        ASN1_TYPE **ppAsn1Type,
        const char * const format,
        ...)
{
    va_list args;
    DWORD error = LW_ERROR_SUCCESS;

    va_start(args, format);
    error = Asn1GeneratePrintfV(
                ppAsn1Type,
                format,
                args);
    va_end(args);

    // Using BAIL_ON here would just add an extra useless log message.
    return error;
}

DWORD
Asn1StackAppendValue(
        STACK_OF(ASN1_TYPE) **ppAsn1Stack,
        ASN1_TYPE *pAsn1Value
        )
{
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    if (*ppAsn1Stack == NULL)
    {
        *ppAsn1Stack = sk_ASN1_TYPE_new_null();
        BAIL_ON_SSL_ERROR(*ppAsn1Stack == NULL);
    }

    sslResult = sk_ASN1_TYPE_push(*ppAsn1Stack, pAsn1Value);
    BAIL_ON_SSL_ERROR(sslResult == 0);

cleanup:
    return error;
}

DWORD
Asn1StackAppendPrintfV(
        STACK_OF(ASN1_TYPE) **ppAsn1Stack,
        const char * const format,
        va_list args
        )
{
    ASN1_TYPE *pAsn1Value = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    error = Asn1GeneratePrintfV(
                &pAsn1Value,
                format,
                args);
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendValue(ppAsn1Stack, pAsn1Value);
    BAIL_ON_LW_ERROR(error);

    pAsn1Value = NULL;

cleanup:
    if (pAsn1Value)
    {
        ASN1_TYPE_free(pAsn1Value);
    }

    return error;
}

DWORD
Asn1StackAppendPrintf(
        STACK_OF(ASN1_TYPE) **ppAsn1Stack,
        const char * const format,
        ...
        )
{
    va_list args;
    DWORD error = LW_ERROR_SUCCESS;

    va_start(args, format);
    error = Asn1StackAppendPrintfV(ppAsn1Stack, format, args);
    va_end(args);

    return error;
}

DWORD
Asn1StackToSequence(
        STACK_OF(ASN1_TYPE) *pAsn1Stack,
        ASN1_TYPE **ppAsn1Sequence,
        int type
        )
{
    ASN1_TYPE *pAsn1Sequence = NULL;
    ASN1_STRING *pStringValue = NULL;
    unsigned char *data = NULL;
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    pAsn1Sequence = ASN1_TYPE_new();
    BAIL_ON_SSL_ERROR(pAsn1Sequence == NULL);

    pAsn1Sequence->type = type;
    pAsn1Sequence->value.asn1_string = ASN1_STRING_type_new(type);
    BAIL_ON_SSL_ERROR(pAsn1Sequence->value.asn1_string == NULL);

    pStringValue = pAsn1Sequence->value.asn1_string;

    pStringValue->length = i2d_ASN1_SET_OF_ASN1_TYPE(
                                pAsn1Stack,
                                NULL,
                                i2d_ASN1_TYPE,
                                type,
                                V_ASN1_UNIVERSAL,
                                type == V_ASN1_SET);
    BAIL_ON_SSL_ERROR(pStringValue->length == 0);

    pStringValue->data = OPENSSL_malloc(pStringValue->length);
    BAIL_ON_SSL_ERROR(pStringValue->data == NULL);

    data = pStringValue->data;
    sslResult = i2d_ASN1_SET_OF_ASN1_TYPE(
                    pAsn1Stack,
                    &data,
                    i2d_ASN1_TYPE,
                    type,
                    V_ASN1_UNIVERSAL,
                    type == V_ASN1_SET);
    BAIL_ON_SSL_ERROR(sslResult == 0);

cleanup:
    if (error)
    {
        if (pAsn1Sequence)
        {
            ASN1_TYPE_free(pAsn1Sequence);
            pAsn1Sequence = NULL;
        }
    }

    *ppAsn1Sequence = pAsn1Sequence;
    return error;
}

DWORD
BufAppend(
        BUF_MEM *buf,
        PCSTR value,
        int length
        )
{
    int old_length = buf->length;
    int new_length = 0;
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    if (length == -1)
    {
        length = strlen(value);
    }

    new_length = buf->length + length;
    if (new_length >= buf->max)
    {
        sslResult = BUF_MEM_grow(buf, new_length + 1);
        BAIL_ON_SSL_ERROR(sslResult == 0);
    }

    buf->length = new_length;
    memcpy(buf->data + old_length, value, length);
    buf->data[buf->length] = '\0';

cleanup:
    return error;
}

static BOOL
IsCriticalExtension(
        const ASN1_OBJECT* object,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions
        )
{
    int index = 0;
    BOOL result = FALSE;

    if (criticalExtensions)
    {
        for (index = 0; index < sk_ASN1_OBJECT_num(criticalExtensions); ++index)
        {
            if (OBJ_cmp(
                    object,
                    sk_ASN1_OBJECT_value(criticalExtensions, index)) == 0)
            {
                result = TRUE;
                break;
            }
        }
    }

    return result;
}

DWORD
AddExtension(
        STACK_OF(X509_EXTENSION) **extensions,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        X509_EXTENSION *extension
        )
{
    DWORD error = LW_ERROR_SUCCESS;

    extension->critical = IsCriticalExtension(
                                extension->object,
                                criticalExtensions);

    BAIL_ON_SSL_ERROR(X509v3_add_ext(extensions, extension, -1) == NULL);

cleanup:
    return error;
}

DWORD
AddExtensionAsn1Value(
        STACK_OF(X509_EXTENSION) **extensions,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        PCSTR name,
        ASN1_TYPE *value
        )
{
    ASN1_OBJECT *object = NULL;
    ASN1_OCTET_STRING *valueString = NULL;
    unsigned char *data;
    X509_EXTENSION *extension = NULL;
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    object = OBJ_txt2obj(name, 0);
    BAIL_ON_SSL_ERROR(object == NULL);

    valueString = ASN1_OCTET_STRING_new();
    BAIL_ON_SSL_ERROR(valueString == NULL);

    valueString->length = i2d_ASN1_TYPE(value, NULL);
    BAIL_ON_SSL_ERROR(valueString->length < 0);

    valueString->data = OPENSSL_malloc(valueString->length);
    BAIL_ON_SSL_ERROR(valueString->data == NULL);

    data = valueString->data; // i2d_ASN1_TYPE changes the data pointer.
    sslResult = i2d_ASN1_TYPE(value, &data);
    BAIL_ON_SSL_ERROR(sslResult < 0);

    extension = X509_EXTENSION_create_by_OBJ(
                    NULL,
                    object,
                    0,
                    valueString);
    BAIL_ON_SSL_ERROR(extension == NULL);

    error = AddExtension(extensions, criticalExtensions, extension);
    BAIL_ON_LW_ERROR(error);

cleanup:
    if (object)
    {
        ASN1_OBJECT_free(object);
    }

    if (valueString)
    {
        ASN1_OCTET_STRING_free(valueString);
    }

    if (extension)
    {
        X509_EXTENSION_free(extension);
    }

    return error;
}

DWORD
AddExtensionString(
        STACK_OF(X509_EXTENSION) **extensions,
        X509V3_CTX *pExtensionContext,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        PCSTR name,
        PCSTR value
        )
{
    X509_EXTENSION *extension = NULL;
    PSTR criticalValue = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    extension = X509V3_EXT_nconf(
                    NULL,
                    pExtensionContext,
                    (PSTR) name,
                    (PSTR) value);
    BAIL_ON_SSL_ERROR(extension == NULL);

    error = AddExtension(extensions, criticalExtensions, extension);
    BAIL_ON_LW_ERROR(error);

cleanup:
    if (extension)
    {
        X509_EXTENSION_free(extension);
    }

    LW_SAFE_FREE_STRING(criticalValue);

    return error;
}

DWORD
AddExtensionPrintf(
        STACK_OF(X509_EXTENSION) **extensions,
        X509V3_CTX *pExtensionContext,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        PSTR name,
        const char * const format,
        ...
        )
{
    PSTR value = NULL;
    va_list args;
    DWORD error = LW_ERROR_SUCCESS;

    va_start(args, format);
    error = LwAllocateStringPrintfV(
                &value,
                format,
                args);
    va_end(args);
    BAIL_ON_LW_ERROR(error);

    error = AddExtensionString(
                extensions,
                pExtensionContext,
                criticalExtensions,
                name,
                value);
    // BAIL_ON here would just add a useless extra log message.

cleanup:
    LW_SAFE_FREE_STRING(value);

    return error;
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
