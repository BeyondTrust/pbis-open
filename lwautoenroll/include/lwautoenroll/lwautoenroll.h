/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
/**
 * @file lwautoenroll.h
 * @brief Likewise AutoEnrollment C API
 */
#ifndef __LWAUTOENROLL_H__
#define __LWAUTOENROLL_H__

#include <openssl/evp.h>
#include <openssl/x509v3.h>

#include <lw/attrs.h>
#include <lw/types.h>

/**
 * @brief Autoenrollment Service
 *
 * The information needed about an autoenrollment service.
 */
typedef struct _LW_AUTOENROLL_SERVICE
{
    struct _LW_AUTOENROLL_SERVICE   *next;
    int                             priority;
    LW_BOOL                         renewOnly;
    PSTR                            url;

} LW_AUTOENROLL_SERVICE, *PLW_AUTOENROLL_SERVICE;

/**
 * @brief Autoenrollment Certificate Template
 *
 * The information needed about a certificate template.
 */
typedef struct _LW_AUTOENROLL_TEMPLATE
{
        /** @brief The name of the template. */
        PCSTR                       name;
        /** @brief A user-friendly display name for the template. */
        PCSTR                       displayName;
        /**
         * @brief The Cryptographic Service Provider specified in the template.
         *
         * Only needed for internal use.
         */
        PCSTR                       csp;
        /**
         * @brief A linked list of enrollment servers that can be used
         * for this template.
         */
        PLW_AUTOENROLL_SERVICE      pEnrollmentServices;
        /**
         * @brief The minimum key size specified in the template.
         *
         * May be modified by the client to request a larger key.
         * */
        unsigned int                keySize;
        /**
         * @brief the X.509 Key Usage bits specified in the template.
         *
         * May be modified by the client to request additional key usage bits,
         * although the Certification Authority may then reject the request.
         */
        DWORD                       keyUsage;
        /**
         * @brief The enrollment flags specified in the template.
         *
         * Only needed for internal use.
         */
        DWORD                       enrollmentFlags;
        /**
         * @brief The name flags specified in the template.
         *
         * Only needed for internal use.
         */
        DWORD                       nameFlags;
        /**
         * @brief the X.509 Extended Key Usage attribute specified in
         * the template.
         *
         * May be modified by the client to request additional extended
         * key usage values, although the Certification Authority may
         * then reject the request.
         */
        EXTENDED_KEY_USAGE          *extendedKeyUsage;
        /**
         * @brief the list of X.509 extensions that should be marked critical.
         *
         * May be modified by the client to add additional extensions,
         * although the Certification Authority may then reject the request.
         */
        STACK_OF(ASN1_OBJECT)       *criticalExtensions;
        /**
         * @brief The list of X.509 attributes specified in the template.
         *
         * May be modified by the client to add additional attributes,
         * although the Certification Authority may then reject the request.
         */
	STACK_OF(X509_ATTRIBUTE)    *attributes;
        /**
         * @brief The list of X.509 extensions specified in the template.
         *
         * May be modified by the client to add additional extensions,
         * although the Certification Authority may then reject the request.
         */
        STACK_OF(X509_EXTENSION)    *extensions;
} LW_AUTOENROLL_TEMPLATE, *PLW_AUTOENROLL_TEMPLATE;

LW_BEGIN_EXTERN_C

/**
 * @brief Get the list of certificate templates.
 *
 * Returns a list of certificate templates, and the number of templates.
 * The list should be freed with #LwAutoEnrollFreeTemplateList when
 * it is no longer needed.
 *
 * @param[in] credentialsCache  A path to a kerberos credentials cache file.  If credentialsCache is NULL the default credentials cache will be used.
 * @param[out] ppTemplateList   The returned array of template structures.
 * @param[out] pNumTemplates    The returned number of templates in the array.
 *
 * @return An LW error code indicating success or failure.
 */
DWORD
LwAutoEnrollGetTemplateList(
        IN OPTIONAL PCSTR credentialsCache,
        OUT PLW_AUTOENROLL_TEMPLATE *ppTemplateList,
        OUT PDWORD pNumTemplates
        );

/**
 * @brief Free a list of templates.
 *
 * Frees a template list returned by #LwAutoEnrollGetTemplateList.
 *
 * @param[in] pTemplateList The array of template structures.
 * @param[in] numTemplates  The number of templates in the array.
 */
VOID
LwAutoEnrollFreeTemplateList(
        IN PLW_AUTOENROLL_TEMPLATE pTemplateList,
        IN DWORD numTemplates
        );

/**
 * @brief Request a certificate.
 *
 * #LwAutoEnrollRequestCertificate generates an X.509 certificate request
 * based on a certificate template, Active Directory information for the
 * current user (as identified by the kerberos principal found in the
 * kerberos credentials cache), and an optional subject name.  It sends
 * the request to an enrollment web service, using either a caller-provided
 * URL or searching Active Directory to find a suitable enrollment server
 * URL.
 *
 * The request may be processed immediately, in which case
 * #LwAutoEnrollRequestCertificate will return LW_ERROR_SUCCESS or
 * LW_ERROR_AUTOENROLL_FAILED, depending on the response from the
 * server.  If the request succeeds a certificate is returned.
 *
 * The request may also be accepted for later processing, which
 * generally means an administrator will approve or deny the
 * request.  In this case the server returns a request ID, which
 * #LwAutoEnrollRequestCertificate will return along with the URL
 * (if it was not provided by the caller) and the error code
 * ERROR_CONTINUE.
 *
 * @param[in] pTemplate         The template stucture to use.
 * @param[in] pSubjectName      The X.509 subject distinguised name to place into the certificate request.  If pSubjectName is NULL, no subject name attribute will be added to the request.  Unless the template is marked as requiring a subject name, the Certification Authority will fill in the subject name based on the user's attributes in Active Directory.
 * @param[in] credentialsCache  A path to a kerberos credentials cache file.  If credentialsCache is NULL the default credentials cache will be used.
 * @param[in,out] pUrl          A pointer to the URL of the certificate enrollment web service.  If *pUrl is NULL, a suitable URL will be found and stored in *pUrl.
 * @param[in,out] ppKeyPair     A pointer to a public/private key pair.  If *ppKeyPair is NULL, a new key pair will be generated and stored in *ppKeyPair.
 * @param[out] ppCertificate    The returned X.509 certificate.  If the request fails, *ppCertificate will be set to NULL.
 * @param[out] pRequestId       The returned request ID for requests that have been accepted for later processing.
 *
 * @returns An LW error code indicating success or failure.  Errors that are specific to #LwAutoEnrollRequestCertificate include:
 * @retval LW_ERROR_SUCCESS Success.
 * @retval ERROR_CONTINUE The request has been accepted for later processing.  Use #LwAutoEnrollGetRequestStatus later to get the status of the request.
 * @retval LW_ERROR_AUTOENROLL_FAILED The request was rejected by the enrollment server.  For security reasons the server only returns a generic failure status.
 * @retval LW_ERROR_AUTOENROLL_HTTP_REQUEST_FAILED The HTTP request to the enrollment server failed.
 * @retval LW_ERROR_AUTOENROLL_SUBJECT_NAME_REQUIRED The specified template requires a subject name, but pSubjectName was specified as NULL.
 */
DWORD
LwAutoEnrollRequestCertificate(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN OPTIONAL X509_NAME *pSubjectName,
        IN OPTIONAL PCSTR credentialsCache,
        IN OUT PSTR *pUrl,
        IN OUT EVP_PKEY **ppKeyPair,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestId
        );

/**
 * @brief Get the status of a previous certificate request that returned ERROR_CONTINUE.
 *
 * @param[in] url               The URL of the certificate enrollment web service.  This should be the URL passed to or returned by a previous call to #LwAutoEnrollRequestCertificate.
 * @param[in] requestId         The request id returned by a previous call to #LwAutoEnrollRequestCertificate.
 * @param[in] credentialsCache  A path to a kerberos credentials cache file.  If credentialsCache is NULL the default credentials cache will be used.
 * @param[out] ppCertificate    The returned X.509 certificate.  If the request fails, *ppCertificate will be set to NULL.
 *
 * @returns An LW error code indicating success or failure.  Errors that are specific to #LwAutoEnrollGetRequestStatus include:
 * @retval LW_ERROR_SUCCESS Success.
 * @retval ERROR_CONTINUE The request is still waiting for processing.
 * @retval LW_ERROR_AUTOENROLL_FAILED The request was rejected by the enrollment server.  For security reasons the server only returns a generic failure status.
 * @retval LW_ERROR_AUTOENROLL_HTTP_REQUEST_FAILED The HTTP request to the enrollment server failed.
 */
DWORD
LwAutoEnrollGetRequestStatus(
        IN PCSTR url,
        IN DWORD requestId,
        IN OPTIONAL PCSTR credentialsCache,
        OUT X509 **ppCertificate
        );

LW_END_EXTERN_C

#endif /* __LWAUTOENROLL_H */
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
