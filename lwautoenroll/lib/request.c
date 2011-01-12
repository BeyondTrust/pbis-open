#include <lwautoenroll/lwautoenroll.h>
#include <bail.h>

#include <openssl/bn.h>
#include <openssl/conf.h>
#include <openssl/conf_api.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h> /* DeBuG */

#include <lwerror.h>
#include <lwmem.h>
#include <lwstr.h>

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define OID_MS_ENROLLMENT_CSP_PROVIDER      "1.3.6.1.4.1.311.13.2.2"
#define OID_MS_OS_VERSION                   "1.3.6.1.4.1.311.13.2.3"
#define OID_MS_ENROLL_CERTTYPE_EXTENSION    "1.3.6.1.4.1.311.20.2"
#define OID_MS_REQUEST_CLIENT_INFO          "1.3.6.1.4.1.311.21.20"

static DWORD
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

    fprintf(stderr, "Asn1GeneratePrintfV(%s)\n", asn1String); /* DeBuG */
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

#if 0
static DWORD
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
#endif

static DWORD
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

static DWORD
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

static DWORD
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

static DWORD
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

static DWORD
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
    strncpy(buf->data + old_length, value, length);
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

static DWORD
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

static DWORD
AddExtensionAsn1Value(
        STACK_OF(X509_EXTENSION) **extensions,
        const STACK_OF(ASN1_OBJECT) *criticalExtensions,
        PCSTR name,
        ASN1_TYPE *value
        )
{
    ASN1_OBJECT *object = NULL;
    ASN1_OCTET_STRING *valueString;
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

static DWORD
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

    fprintf(stderr, "AddExtension: %s = '%s'\n", name, value); /* DeBuG */
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

static DWORD
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

DWORD
LwAutoEnrollRequestCertificate(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN OUT EVP_PKEY **ppKeyPair,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestID
        )
{
    RSA *pRsaKey = NULL;
    EVP_PKEY *pKeyPair = NULL;
    X509_REQ *pRequest = NULL;
    X509* pCertificate = NULL;
    BUF_MEM *buf = NULL;
    DWORD requestID = 0;
    X509V3_CTX extensionContext = { 0 };
    STACK_OF(ASN1_TYPE) *pAsn1Stack = NULL;
    STACK_OF(ASN1_TYPE) *pAsn1SubStack = NULL;
    ASN1_TYPE *pAsn1Value = NULL;
    STACK_OF(X509_EXTENSION) *extensions = NULL;
    int index = 0;
    int providerID = 0;
    PSTR end = NULL;
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    BAIL_ON_LW_ERROR(error);

    if (*ppKeyPair == NULL)
    {
        pRsaKey = RSA_generate_key(pTemplate->keySize,
                                65537,
                                NULL,
                                NULL);
        BAIL_ON_SSL_ERROR(pRsaKey == NULL);

        pKeyPair = EVP_PKEY_new();
        BAIL_ON_SSL_ERROR(pKeyPair == NULL);

        sslResult = EVP_PKEY_assign_RSA(pKeyPair, pRsaKey);
        BAIL_ON_SSL_ERROR(sslResult == 0);

        *ppKeyPair = pKeyPair;
        pKeyPair = NULL;
        pRsaKey = NULL;
    }

    pRequest = X509_REQ_new();
    BAIL_ON_SSL_ERROR(pRequest == NULL);

    sslResult = X509_REQ_set_version(pRequest, 0L);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sslResult = X509_REQ_set_pubkey(pRequest, *ppKeyPair);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    for (index = 0;
            index < sk_X509_ATTRIBUTE_num(pTemplate->attributes);
            ++index)
    {
        sslResult = X509_REQ_add1_attr(
                        pRequest,
                        sk_X509_ATTRIBUTE_value(
                            pTemplate->attributes,
                            index));
        BAIL_ON_SSL_ERROR(sslResult == 0);
    }

    // Add the Microsoft OS Version attribute.
    pAsn1Value = ASN1_generate_nconf("IA5STRING:6.1.7053.2", NULL);
    BAIL_ON_SSL_ERROR(pAsn1Value == NULL);

    sslResult = X509_REQ_add1_attr_by_txt(
                    pRequest,
                    OID_MS_OS_VERSION,
                    pAsn1Value->type,
                    (unsigned char *) pAsn1Value->value.ptr,
                    -1);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    ASN1_TYPE_free(pAsn1Value);
    pAsn1Value = NULL;

    // Add the Microsoft Request Client Info attribute.
    error = Asn1StackAppendPrintf(
        &pAsn1Stack,
        "INTEGER:42"); // XXX
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendPrintf(
        &pAsn1Stack,
        "UTF8String:keithr-centos53.keithr.test"); // XXX
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendPrintf(
        &pAsn1Stack,
        "UTF8String:KEITHR\\Administrator"); // XXX
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendPrintf(
        &pAsn1Stack,
        "UTF8String:lwautoenroll");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackToSequence(
        pAsn1Stack,
        &pAsn1Value,
        V_ASN1_SEQUENCE);
    BAIL_ON_LW_ERROR(error);

    sslResult = X509_REQ_add1_attr_by_txt(
                    pRequest,
                    OID_MS_REQUEST_CLIENT_INFO,
                    pAsn1Value->type,
                    (unsigned char *) pAsn1Value->value.ptr,
                    -1);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sk_ASN1_TYPE_pop_free(pAsn1Stack, ASN1_TYPE_free);
    pAsn1Stack = NULL;
    ASN1_TYPE_free(pAsn1Value);
    pAsn1Value = NULL;

    // Add the Microsoft Enrollment CSP Info attribute.
    if (pTemplate->csp && isdigit(pTemplate->csp[0]))
    {
        providerID = strtoul(pTemplate->csp, &end, 0);

        error = Asn1StackAppendPrintf(
            &pAsn1Stack,
            "INTEGER:%d",
            providerID);
        BAIL_ON_LW_ERROR(error);

        error = Asn1StackAppendPrintf(
            &pAsn1Stack,
            "BMPSTRING:%s",
            end + 1);
        BAIL_ON_LW_ERROR(error);
    }
    else
    {
        error = Asn1StackAppendPrintf(
            &pAsn1Stack,
            "BMPSTRING:%s",
            pTemplate->csp);
        BAIL_ON_LW_ERROR(error);
    }

    //
    // This is at the end of the example from Microsoft; I'm not sure if
    // it's actually necessary.
    //
    error = Asn1StackAppendPrintf(
        &pAsn1Stack,
        "EXPLICIT:3U,BOOLEAN:FALSE");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackToSequence(
        pAsn1Stack,
        &pAsn1Value,
        V_ASN1_SEQUENCE);
    BAIL_ON_LW_ERROR(error);

    sslResult = X509_REQ_add1_attr_by_txt(
                    pRequest,
                    OID_MS_ENROLLMENT_CSP_PROVIDER,
                    pAsn1Value->type,
                    (unsigned char *) pAsn1Value->value.ptr,
                    -1);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sk_ASN1_TYPE_pop_free(pAsn1Stack, ASN1_TYPE_free);
    pAsn1Stack = NULL;
    ASN1_TYPE_free(pAsn1Value);
    pAsn1Value = NULL;

    // Add extensions.
    X509V3_set_ctx(&extensionContext, NULL, NULL, pRequest, NULL, 0);

    if (pTemplate->extensions)
    {
        extensions = sk_X509_EXTENSION_dup(pTemplate->extensions);
    }

    error = AddExtensionPrintf(
                &extensions,
                &extensionContext,
                pTemplate->criticalExtensions,
                "keyUsage",
                "ASN1:EXPLICIT:3U,INTEGER:%d",
                pTemplate->keyUsage);
    BAIL_ON_LW_ERROR(error);

    error = AddExtensionString(
                &extensions,
                &extensionContext,
                pTemplate->criticalExtensions,
                "subjectKeyIdentifier",
                "hash");
    BAIL_ON_LW_ERROR(error);

    error = AddExtensionPrintf(
                &extensions,
                &extensionContext,
                pTemplate->criticalExtensions,
                OID_MS_ENROLL_CERTTYPE_EXTENSION,
                "ASN1:BMPSTRING:%s",
                pTemplate->name);
    BAIL_ON_LW_ERROR(error);

    if (pTemplate->extendedKeyUsage)
    {
        buf = BUF_MEM_new();
        BAIL_ON_SSL_ERROR(buf == NULL);

        for (index = 0;
                index < sk_ASN1_OBJECT_num(pTemplate->extendedKeyUsage);
                ++index)
        {
            if (buf->length)
            {
                error = BufAppend(buf, ", ", 2);
                BAIL_ON_LW_ERROR(error);
            }

            error = BufAppend(
                        buf,
                        OBJ_nid2sn(OBJ_obj2nid(
                            sk_ASN1_OBJECT_value(
                                pTemplate->extendedKeyUsage,
                                index))),
                        -1);
            BAIL_ON_LW_ERROR(error);
        }

        error = AddExtensionString(
                    &extensions,
                    &extensionContext,
                    pTemplate->criticalExtensions,
                    "extendedKeyUsage",
                    buf->data);
        BAIL_ON_LW_ERROR(error);
    }

    // Add the SMIME Capabilities extension.
    error = Asn1StackAppendPrintf(
        &pAsn1SubStack,
        "OBJECT:RC2-CBC");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendPrintf(
        &pAsn1SubStack,
        "INTEGER:0x0080");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackToSequence(
        pAsn1SubStack,
        &pAsn1Value,
        V_ASN1_SEQUENCE);
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendValue(
        &pAsn1Stack,
        pAsn1Value);
    BAIL_ON_LW_ERROR(error);

    pAsn1SubStack = NULL;
    pAsn1Value = NULL;

    error = Asn1StackAppendPrintf(
        &pAsn1SubStack,
        "OBJECT:RC4");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendPrintf(
        &pAsn1SubStack,
        "INTEGER:0x0080");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackToSequence(
        pAsn1SubStack,
        &pAsn1Value,
        V_ASN1_SEQUENCE);
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendValue(
        &pAsn1Stack,
        pAsn1Value);
    BAIL_ON_LW_ERROR(error);

    pAsn1SubStack = NULL;
    pAsn1Value = NULL;

    error = Asn1StackAppendPrintf(
        &pAsn1SubStack,
        "OBJECT:DES-CBC");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackToSequence(
        pAsn1SubStack,
        &pAsn1Value,
        V_ASN1_SEQUENCE);
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendValue(
        &pAsn1Stack,
        pAsn1Value);
    BAIL_ON_LW_ERROR(error);

    pAsn1SubStack = NULL;
    pAsn1Value = NULL;

    error = Asn1StackAppendPrintf(
        &pAsn1SubStack,
        "OBJECT:DES-EDE3-CBC");
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackToSequence(
        pAsn1SubStack,
        &pAsn1Value,
        V_ASN1_SEQUENCE);
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendValue(
        &pAsn1Stack,
        pAsn1Value);
    BAIL_ON_LW_ERROR(error);

    pAsn1SubStack = NULL;
    pAsn1Value = NULL;

    error = Asn1StackToSequence(
        pAsn1Stack,
        &pAsn1Value,
        V_ASN1_SEQUENCE);
    BAIL_ON_LW_ERROR(error);

    error = AddExtensionAsn1Value(
            &extensions,
            pTemplate->criticalExtensions,
            "SMIME-CAPS",
            pAsn1Value);
    BAIL_ON_LW_ERROR(error);

    sk_ASN1_TYPE_pop_free(pAsn1Stack, ASN1_TYPE_free);
    pAsn1Stack = NULL;
    ASN1_TYPE_free(pAsn1Value);
    pAsn1Value = NULL;

    sslResult = X509_REQ_add_extensions(pRequest, extensions);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sslResult = X509_REQ_sign(pRequest, *ppKeyPair, EVP_sha1());
    BAIL_ON_SSL_ERROR(sslResult == 0);

    BIO *out = BIO_new(BIO_s_file()); /* DeBuG */
    BIO_set_fp(out, stdout, BIO_NOCLOSE); /* DeBuG */
    PEM_write_bio_X509_REQ(out, pRequest); /* DeBuG */
    BIO_free_all(out); /* DeBuG */
cleanup:
    if (error)
    {
        if (pKeyPair)
        {
            EVP_PKEY_free(pKeyPair);
            pKeyPair = NULL;
        }

        if (pCertificate)
        {
            X509_free(pCertificate);
            pCertificate = NULL;
        }
    }

    if (pRequest)
    {
        X509_REQ_free(pRequest);
        pRequest = NULL;
    }

    if (extensions)
    {
        sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);
    }

    if (pRsaKey)
    {
        RSA_free(pRsaKey);
    }

    if (buf)
    {
        BUF_MEM_free(buf);
    }

    if (pAsn1Stack)
    {
        sk_ASN1_TYPE_pop_free(pAsn1Stack, ASN1_TYPE_free);
    }

    if (pAsn1SubStack)
    {
        sk_ASN1_TYPE_pop_free(pAsn1SubStack, ASN1_TYPE_free);
    }

    if (pAsn1Value)
    {
        ASN1_TYPE_free(pAsn1Value);
    }

    *ppCertificate = pCertificate;
    *pRequestID = requestID;

    return error;
}

DWORD
LwAutoEnrollGetRequestStatus(
        IN DWORD RequestID,
        OUT X509 **ppCertificate
        )
{
    return 0;
}
