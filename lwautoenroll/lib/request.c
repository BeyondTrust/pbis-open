#include <lwautoenroll/lwautoenroll.h>
#include <bail.h>
#include <curl_util.h>
#include <ldap_util.h>
#include <mspki.h>
#include <soap_util.h>
#include <ssl.h>
#include <x509_util.h>

#include <krb5/krb5.h>

#include <lsa/lsa.h>

#include <lwerror.h>
#include <lwkrb5.h>
#include <lwldap.h>
#include <lwmem.h>
#include <lwstr.h>

#include <sys/param.h>

#include <ctype.h>
#include <ldap.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#define LDAP_QUERY  "(objectClass=pKIEnrollmentService)"
#define LDAP_BASE   "cn=Public Key Services,cn=Services,cn=Configuration"

static DWORD
LwAutoEnrollGetUrl(
        IN PCSTR domainDnsName,
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        OUT PSTR* pUrl
        )
{
    HANDLE ldapConnection = (HANDLE) NULL;
    static PSTR attributeList[] =
    {
        "certificateTemplates",
        "msPKI-Enrollment-Servers",
        NULL
    };
    LDAP *pLdap = NULL;
    LDAPMessage *pLdapResults = NULL;
    LDAPMessage *pLdapResult = NULL;
    struct berval **ppValues = NULL;
    int numValues = 0;
    int value = 0;
    PSTR enrollmentUrl = NULL;
    int enrollmentServerPriority = INT_MAX;
    DWORD error = LW_ERROR_SUCCESS;

    error = LwAutoEnrollLdapConnect(
                domainDnsName,
                &ldapConnection);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapSearch(
                ldapConnection,
                domainDnsName,
                LDAP_BASE,
                LDAP_SCOPE_SUBTREE,
                LDAP_QUERY,
                attributeList,
                &pLdapResults);
    BAIL_ON_LW_ERROR(error);

    pLdap = LwLdapGetSession(ldapConnection);

    for (pLdapResult = LwLdapFirstEntry(
                            ldapConnection,
                            pLdapResults);
            pLdapResult != NULL;
            pLdapResult = LwLdapNextEntry(
                            ldapConnection,
                            pLdapResult)
        )
    {
        ldap_value_free_len(ppValues);
        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "certificateTemplates");
        numValues = ldap_count_values_len(ppValues);

        for (value = 0; value < numValues; ++value)
        {
            if (pTemplate->name[ppValues[value]->bv_len] == '\0' &&
                    !strncmp(
                        pTemplate->name,
                        ppValues[value]->bv_val,
                        ppValues[value]->bv_len))
            {
                ldap_value_free_len(ppValues);
                ppValues = ldap_get_values_len(
                                pLdap,
                                pLdapResult,
                                "msPKI-Enrollment-Servers");
                if (ppValues != NULL)
                {
                    PSTR serverInfo = ppValues[value]->bv_val;
                    int priority;
                    int authType;
                    int renewOnly;

                    priority = strtoul(serverInfo, &serverInfo, 10);
                    if (*serverInfo != '\n')
                    {
                        break;
                    }

                    if (priority > enrollmentServerPriority)
                    {
                        break;
                    }

                    ++serverInfo;
                    authType = strtoul(serverInfo, &serverInfo, 10);
                    if (*serverInfo != '\n')
                    {
                        break;
                    }

                    if (authType != 2) // kerberos
                    {
                        break;
                    }

                    ++serverInfo;
                    renewOnly = strtoul(serverInfo, &serverInfo, 10);
                    if (*serverInfo != '\n')
                    {
                        break;
                    }

                    ++serverInfo;
                    error = LwAllocateString(serverInfo, &enrollmentUrl);
                    BAIL_ON_LW_ERROR(error);
                }

                break;
            }
        }
    }

    if (enrollmentUrl == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_PARAMETER,
            ": No enrollment server found for template '%s'",
            pTemplate->name);
    }

cleanup:
    if (error)
    {
        LW_SAFE_FREE_STRING(enrollmentUrl);
    }

    if (ldapConnection)
    {
        LwLdapCloseDirectory(ldapConnection);
    }

    if (ppValues)
    {
        ldap_value_free_len(ppValues);
    }

    *pUrl = enrollmentUrl;
    return error;
}

DWORD
LwAutoEnrollRequestCertificate(
        IN OPTIONAL PCSTR credentialsCache,
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN OUT PSTR *pUrl,
        IN OUT EVP_PKEY **ppKeyPair,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestId
        )
{
    HANDLE lsaConnection = (HANDLE) NULL;
    krb5_context krbContext = NULL;
    krb5_ccache krbCache = NULL;
    krb5_principal krbPrincipal = NULL;
    PSTR krbUpn = NULL;
    LSA_QUERY_LIST QueryList = { 0 };
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PCSTR domainDn = NULL;
    PSTR domainDnsName = NULL;
    X509_REQ *pX509Request = NULL;
    OpenSOAPEnvelopePtr pSoapRequest = NULL;
    OpenSOAPEnvelopePtr pSoapReply = NULL;
    OpenSOAPBlockPtr pReplyBody = NULL;
    OpenSOAPByteArrayPtr pSoapFaultBuffer = NULL;
    const unsigned char *soapFaultStr = NULL;
    size_t soapFaultSize = 0;
    OpenSOAPXMLElmPtr pSecurityTokenResponse = NULL;
    OpenSOAPXMLElmPtr pRequestIdElement = NULL;
    OpenSOAPXMLElmPtr pRequestedSecurityToken = NULL;
    OpenSOAPXMLElmPtr pBinarySecurityToken = NULL;
    OpenSOAPByteArrayPtr pCertificateBuffer = NULL;
    const unsigned char *certificateStr = NULL;
    size_t certificateSize = 0;
    BIO *pCertificateBio = NULL;
    DWORD requestId = 0;
    X509* pCertificate = NULL;
    krb5_error_code krbResult = 0;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    krbResult = krb5_init_context(&krbContext);
    BAIL_ON_KRB_ERROR(krbResult, krbContext);

    if (credentialsCache == NULL)
    {
        krbResult = krb5_cc_default(krbContext, &krbCache);
        BAIL_ON_KRB_ERROR(krbResult, krbContext);
    }
    else
    {
        krbResult = krb5_cc_resolve(krbContext, credentialsCache, &krbCache);
        BAIL_ON_KRB_ERROR(krbResult, krbContext);

        error = LwKrb5SetDefaultCachePath(credentialsCache, NULL);
        BAIL_ON_LW_ERROR(error);
    }

    krbResult = krb5_cc_get_principal(krbContext, krbCache, &krbPrincipal);
    BAIL_ON_KRB_ERROR(krbResult, krbContext);

    krbResult = krb5_unparse_name(krbContext, krbPrincipal, &krbUpn);
    BAIL_ON_KRB_ERROR(krbResult, krbContext);

    error = LsaOpenServer(&lsaConnection);
    BAIL_ON_LW_ERROR(error);

    QueryList.ppszStrings = (PCSTR*) &krbUpn;

    error = LsaFindObjects(
                lsaConnection,
                NULL,
                0,
                LSA_OBJECT_TYPE_UNDEFINED,
                LSA_QUERY_TYPE_BY_UPN,
                1,
                QueryList,
                &ppObjects);
    BAIL_ON_LW_ERROR(error);

    error = GenerateX509Request(
                pTemplate,
                ppKeyPair,
                ppObjects[0]->pszNetbiosDomainName,
                ppObjects[0]->pszSamAccountName,
                &pX509Request);
    BAIL_ON_LW_ERROR(error);

    if (*pUrl == NULL)
    {
        error = LwAutoEnrollFindDomainDn(
                    ppObjects[0]->pszDN,
                    &domainDn);
        BAIL_ON_LW_ERROR(error);

        error = LwLdapConvertDNToDomain(
                    domainDn,
                    &domainDnsName);
        BAIL_ON_LW_ERROR(error);

        error = LwAutoEnrollGetUrl(
                    domainDnsName,
                    pTemplate,
                    pUrl);
        BAIL_ON_LW_ERROR(error);
    }

    error = GenerateSoapRequest(
                *pUrl,
                pX509Request,
                &pSoapRequest);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollCurlSoapRequest(
                *pUrl,
                pSoapRequest,
                &pSoapReply);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPEnvelopeGetBodyBlockMB(
                    pSoapReply,
                    "Fault",
                    &pReplyBody);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pReplyBody != NULL)
    {
        soapResult = OpenSOAPByteArrayCreate(&pSoapFaultBuffer);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPBlockGetCharEncodingString(
                        pReplyBody,
                        NULL,
                        pSoapFaultBuffer);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPByteArrayGetBeginSizeConst(
                        pSoapFaultBuffer,
                        &soapFaultStr,
                        &soapFaultSize);
        BAIL_ON_SOAP_ERROR(soapResult);

        LW_LOG_ERROR("SOAP Fault: %.*s", soapFaultSize, soapFaultStr);
        BAIL_WITH_LW_ERROR(LW_ERROR_AUTOENROLL_FAILED);
    }

    soapResult = OpenSOAPEnvelopeGetBodyBlockMB(
                    pSoapReply,
                    "RequestSecurityTokenResponseCollection",
                    &pReplyBody);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pReplyBody == NULL)
    {
        BAIL_WITH_LW_ERROR(LW_ERROR_INVALID_MESSAGE);
    }

    soapResult = OpenSOAPBlockGetChildMB(
                    pReplyBody,
                    "RequestSecurityTokenResponse",
                    &pSecurityTokenResponse);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pSecurityTokenResponse == NULL)
    {
        BAIL_WITH_LW_ERROR(LW_ERROR_INVALID_MESSAGE);
    }

    soapResult = OpenSOAPXMLElmGetChildMB(
                    pSecurityTokenResponse,
                    "RequestID",
                    &pRequestIdElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pRequestIdElement == NULL)
    {
        BAIL_WITH_LW_ERROR(LW_ERROR_INVALID_MESSAGE);
    }

    soapResult = OpenSOAPXMLElmGetValueMB(
                    pRequestIdElement,
                    "int",
                    &requestId);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPXMLElmGetChildMB(
                    pSecurityTokenResponse,
                    "RequestedSecurityToken",
                    &pRequestedSecurityToken);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pRequestedSecurityToken == NULL)
    {
        BAIL_WITH_LW_ERROR(LW_ERROR_INVALID_MESSAGE);
    }

    soapResult = OpenSOAPXMLElmGetChildMB(
                    pRequestedSecurityToken,
                    "BinarySecurityToken",
                    &pBinarySecurityToken);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pBinarySecurityToken == NULL)
    {
        BAIL_WITH_LW_ERROR(LW_ERROR_INVALID_MESSAGE);
    }

    soapResult = OpenSOAPByteArrayCreate(&pCertificateBuffer);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPXMLElmGetValueMB(
                    pBinarySecurityToken,
                    "base64Binary",
                    &pCertificateBuffer);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPByteArrayGetBeginSizeConst(
                    pCertificateBuffer,
                    &certificateStr,
                    &certificateSize);
    BAIL_ON_SOAP_ERROR(soapResult);

    pCertificateBio = BIO_new_mem_buf(
                        (char *) certificateStr,
                        certificateSize);
    BAIL_ON_SSL_ERROR(pCertificateBio == NULL);

    pCertificate = d2i_X509_bio(pCertificateBio, NULL);
    BAIL_ON_SSL_ERROR(pCertificate == NULL);

cleanup:
    if (error)
    {
        if (pCertificate)
        {
            X509_free(pCertificate);
            pCertificate = NULL;
        }
    }

    if (krbContext)
    {
        if (krbUpn)
        {
            krb5_free_unparsed_name(krbContext, krbUpn);
        }

        if (krbPrincipal)
        {
            krb5_free_principal(krbContext, krbPrincipal);
        }

        if (krbCache)
        {
            krb5_cc_close(krbContext, krbCache);
        }

        krb5_free_context(krbContext);
    }

    LW_SAFE_FREE_STRING(domainDnsName);

    if (lsaConnection)
    {
        LsaCloseServer(lsaConnection);
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }

    if (pX509Request)
    {
        X509_REQ_free(pX509Request);
    }

    if (pSoapRequest)
    {
        OpenSOAPEnvelopeRelease(pSoapRequest);
    }

    if (pSoapReply)
    {
        OpenSOAPEnvelopeRelease(pSoapReply);
    }

    if (pSoapFaultBuffer)
    {
        OpenSOAPByteArrayRelease(pSoapFaultBuffer);
    }

    if (pCertificateBuffer)
    {
        OpenSOAPByteArrayRelease(pSoapFaultBuffer);
    }

    if (pCertificateBio)
    {
        BIO_free_all(pCertificateBio);
    }

    *ppCertificate = pCertificate;
    *pRequestId = requestId;

    return error;
}

DWORD
LwAutoEnrollGetRequestStatus(
        IN OPTIONAL PCSTR credentialsCache,
        IN PCSTR url,
        IN DWORD RequestId,
        OUT X509 **ppCertificate
        )
{
    return 0;
}
