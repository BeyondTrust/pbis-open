/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <lwautoenroll/lwautoenroll.h>

#include <bail.h>
#include <mspki.h>
#include <ssl_util.h>
#include <x509_util.h>

#include <lwerror.h>

#include <sys/param.h>

#include <ctype.h>
#include <unistd.h>

#define OID_MS_ENROLLMENT_CSP_PROVIDER      "1.3.6.1.4.1.311.13.2.2"
#define OID_MS_OS_VERSION                   "1.3.6.1.4.1.311.13.2.3"
#define OID_MS_ENROLL_CERTTYPE_EXTENSION    "1.3.6.1.4.1.311.20.2"
#define OID_MS_REQUEST_CLIENT_INFO          "1.3.6.1.4.1.311.21.20"

DWORD
GenerateX509Request(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN PCSTR userNetbiosDomain,
        IN PCSTR userSamName,
        IN OPTIONAL X509_NAME *pSubjectName,
        IN OUT OPTIONAL EVP_PKEY **ppKeyPair,
        OUT X509_REQ **ppRequest
        )
{
    char hostName[MAXHOSTNAMELEN];
    RSA *pRsaKey = NULL;
    EVP_PKEY *pKeyPair = NULL;
    X509_REQ *pRequest = NULL;
    BUF_MEM *buf = NULL;
    X509V3_CTX extensionContext = { 0 };
    STACK_OF(ASN1_TYPE) *pAsn1Stack = NULL;
    STACK_OF(ASN1_TYPE) *pAsn1SubStack = NULL;
    ASN1_TYPE *pAsn1Value = NULL;
    STACK_OF(X509_EXTENSION) *extensions = NULL;
    int index = 0;
    int providerID = 0;
    PSTR end = NULL;
    int sslResult = 0;
    int unixResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    unixResult = gethostname(hostName, sizeof(hostName));
    BAIL_ON_UNIX_ERROR(unixResult == -1, ": Could not get hostname");

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

    if (pSubjectName)
    {
        sslResult = X509_REQ_set_subject_name(pRequest, pSubjectName);
        BAIL_ON_SSL_ERROR(sslResult == 0);
    }

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
        "INTEGER:%d",
        getpid());
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendPrintf(
        &pAsn1Stack,
        "UTF8String:%s",
        hostName);
    BAIL_ON_LW_ERROR(error);

    error = Asn1StackAppendPrintf(
        &pAsn1Stack,
        "UTF8String:%s\\%s",
        userNetbiosDomain,
        userSamName);
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

    if (pTemplate->enrollmentFlags & CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS)
    {
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
    }

    sslResult = X509_REQ_add_extensions(pRequest, extensions);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sslResult = X509_REQ_sign(pRequest, *ppKeyPair, EVP_sha1());
    BAIL_ON_SSL_ERROR(sslResult == 0);

cleanup:
    if (error)
    {
        if (pKeyPair)
        {
            EVP_PKEY_free(pKeyPair);
            pKeyPair = NULL;
        }

        if (pRequest)
        {
            X509_REQ_free(pRequest);
            pRequest = NULL;
        }
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

    *ppRequest = pRequest;
    return error;
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
