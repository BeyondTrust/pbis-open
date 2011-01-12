#include <lwautoenroll/lwautoenroll.h>
#include <bail.h>
#include <curl_util.h>
#include <ldap_util.h>
#include <mspki.h>
#include <soap_util.h>
#include <ssl_util.h>
#include <x509_util.h>

#include <krb5/krb5.h>

#include <lsa/lsa.h>

#include <lwerror.h>
#include <lwkrb5.h>
#include <lwldap.h>
#include <lwmem.h>
#include <lwstr.h>

#include <OpenSOAP/String.h>

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
    PSTR domainDn = NULL;
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

    error = LwLdapConvertDomainToDN(
                domainDnsName,
                &domainDn);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapConnect(
                domainDnsName,
                &ldapConnection);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapSearch(
                ldapConnection,
                domainDn,
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

    LW_SAFE_FREE_STRING(domainDn);

    *pUrl = enrollmentUrl;
    return error;
}

static DWORD
CheckSoapFault(
        IN OpenSOAPEnvelopePtr pSoapReply
        )
{
    OpenSOAPBlockPtr pFaultBlock = NULL;
    OpenSOAPXMLElmPtr pFaultReason = NULL;
    OpenSOAPXMLElmPtr pFaultReasonText = NULL;
    OpenSOAPStringPtr pFaultReasonString = NULL;
    OpenSOAPByteArrayPtr pFaultReasonBuffer = NULL;
    const unsigned char *faultReasonStr = NULL;
    size_t faultReasonSize = 0;
    int soapResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPEnvelopeGetBodyBlockMB(
                    pSoapReply,
                    "Fault",
                    &pFaultBlock);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pFaultBlock != NULL)
    {
        soapResult = OpenSOAPBlockGetChildMB(
                        pFaultBlock,
                        "Reason",
                        &pFaultReason);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPXMLElmGetChildMB(
                        pFaultReason,
                        "Text",
                        &pFaultReasonText);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPStringCreate(&pFaultReasonString);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPXMLElmGetValueMB(
                        pFaultReasonText,
                        "string",
                        &pFaultReasonString);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPByteArrayCreate(&pFaultReasonBuffer);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPStringGetStringUSASCII(
                        pFaultReasonString,
                        pFaultReasonBuffer);
        BAIL_ON_SOAP_ERROR(soapResult);

        soapResult = OpenSOAPByteArrayGetBeginSizeConst(
                        pFaultReasonBuffer,
                        &faultReasonStr,
                        &faultReasonSize);
        BAIL_ON_SOAP_ERROR(soapResult);

        BAIL_WITH_LW_ERROR(
            LW_ERROR_AUTOENROLL_FAILED,
            ": SOAP Fault: %.*s",
            faultReasonSize,
            faultReasonStr);
    }

cleanup:
    if (pFaultReasonString)
    {
        OpenSOAPStringRelease(pFaultReasonString);
    }

    if (pFaultReasonBuffer)
    {
        OpenSOAPByteArrayRelease(pFaultReasonBuffer);
    }

    return error;
}

static DWORD
GetSecurityTokenResponse(
        IN OpenSOAPEnvelopePtr pSoapReply,
        OUT OpenSOAPXMLElmPtr *ppSecurityTokenResponse
        )
{
    OpenSOAPBlockPtr pResponseCollection = NULL;
    OpenSOAPXMLElmPtr pSecurityTokenResponse = NULL;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPEnvelopeGetBodyBlockMB(
                    pSoapReply,
                    "RequestSecurityTokenResponseCollection",
                    &pResponseCollection);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pResponseCollection == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_MESSAGE,
            ": No RequestSecurityTokenResponseCollection element in response");
    }

    soapResult = OpenSOAPBlockGetChildMB(
                    pResponseCollection,
                    "RequestSecurityTokenResponse",
                    &pSecurityTokenResponse);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pSecurityTokenResponse == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_MESSAGE,
            ": No RequestSecurityTokenResponse element in response");
    }

cleanup:
    if (error)
    {
        pSecurityTokenResponse = NULL;
    }

    *ppSecurityTokenResponse = pSecurityTokenResponse;
    return error;
}

static DWORD
CheckResponseDisposition(
        IN OpenSOAPXMLElmPtr pSecurityTokenResponse
        )
{
    OpenSOAPXMLElmPtr pDispositionMessage = NULL;
    OpenSOAPStringPtr pDispositionMessageString = NULL;
    OpenSOAPByteArrayPtr pDispositionMessageBuffer = NULL;
    const unsigned char *dispositionMessageStr = NULL;
    size_t dispositionMessageSize = 0;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPXMLElmGetChildMB(
                    pSecurityTokenResponse,
                    "DispositionMessage",
                    &pDispositionMessage);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pDispositionMessage == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_MESSAGE,
            ": No DispositionMessage element in response");
    }

    soapResult = OpenSOAPStringCreate(&pDispositionMessageString);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPXMLElmGetValueMB(
                    pDispositionMessage,
                    "string",
                    &pDispositionMessageString);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPByteArrayCreate(&pDispositionMessageBuffer);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPStringGetStringUSASCII(
                    pDispositionMessageString,
                    pDispositionMessageBuffer);
    BAIL_ON_SOAP_ERROR(soapResult);

    soapResult = OpenSOAPByteArrayGetBeginSizeConst(
                    pDispositionMessageBuffer,
                    &dispositionMessageStr,
                    &dispositionMessageSize);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (dispositionMessageSize == 22 &&
            strncasecmp(
                (PCSTR) dispositionMessageStr,
                "Taken Under Submission",
                22) == 0)
    {
        BAIL_WITH_LW_ERROR(
            ERROR_CONTINUE,
            ": Certificate request is waiting for approval");
    }

    if (dispositionMessageSize < 6 ||
            strncasecmp((PCSTR) dispositionMessageStr, "Issued", 6) != 0)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_MESSAGE,
            ": Unsupported disposition messsage value '%.*s'",
            dispositionMessageSize,
            dispositionMessageStr);
    }

cleanup:
    if (pDispositionMessageString)
    {
        OpenSOAPStringRelease(pDispositionMessageString);
    }

    if (pDispositionMessageBuffer)
    {
        OpenSOAPByteArrayRelease(pDispositionMessageBuffer);
    }

    return error;
}

static DWORD
GetResponseCertificate(
        IN OpenSOAPXMLElmPtr pSecurityTokenResponse,
        OUT X509 **ppCertificate
        )
{
    OpenSOAPXMLElmPtr pRequestedSecurityToken = NULL;
    OpenSOAPXMLElmPtr pBinarySecurityToken = NULL;
    OpenSOAPByteArrayPtr pCertificateBuffer = NULL;
    const unsigned char *certificateStr = NULL;
    size_t certificateSize = 0;
    BIO *pCertificateBio = NULL;
    X509* pCertificate = NULL;
    int soapResult = OPENSOAP_NO_ERROR;
    DWORD error = LW_ERROR_SUCCESS;

    soapResult = OpenSOAPXMLElmGetChildMB(
                    pSecurityTokenResponse,
                    "RequestedSecurityToken",
                    &pRequestedSecurityToken);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pRequestedSecurityToken == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_MESSAGE,
            ": No RequestedSecurityToken element in response");
    }

    soapResult = OpenSOAPXMLElmGetChildMB(
                    pRequestedSecurityToken,
                    "BinarySecurityToken",
                    &pBinarySecurityToken);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pBinarySecurityToken == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_MESSAGE,
            ": No BinarySecurityToken element in response");
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

    if (pCertificateBuffer)
    {
        OpenSOAPByteArrayRelease(pCertificateBuffer);
    }

    if (pCertificateBio)
    {
        BIO_free_all(pCertificateBio);
    }

    *ppCertificate = pCertificate;
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
    OpenSOAPXMLElmPtr pSecurityTokenResponse = NULL;
    OpenSOAPXMLElmPtr pRequestIdElement = NULL;
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

    error = CheckSoapFault(pSoapReply);
    BAIL_ON_LW_ERROR(error);

    error = GetSecurityTokenResponse(pSoapReply, &pSecurityTokenResponse);
    BAIL_ON_LW_ERROR(error);

    soapResult = OpenSOAPXMLElmGetChildMB(
                    pSecurityTokenResponse,
                    "RequestID",
                    &pRequestIdElement);
    BAIL_ON_SOAP_ERROR(soapResult);

    if (pRequestIdElement == NULL)
    {
        BAIL_WITH_LW_ERROR(
            LW_ERROR_INVALID_MESSAGE,
            ": No RequestID element in response");
    }

    soapResult = OpenSOAPXMLElmGetValueMB(
                    pRequestIdElement,
                    "int",
                    &requestId);
    BAIL_ON_SOAP_ERROR(soapResult);

    error = CheckResponseDisposition(pSecurityTokenResponse);
    BAIL_ON_LW_ERROR(error);

    error = GetResponseCertificate(pSecurityTokenResponse, &pCertificate);
    BAIL_ON_LW_ERROR(error);

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

    *ppCertificate = pCertificate;
    *pRequestId = requestId;

    return error;
}

DWORD
LwAutoEnrollGetRequestStatus(
        IN OPTIONAL PCSTR credentialsCache,
        IN PCSTR url,
        IN DWORD requestId,
        OUT X509 **ppCertificate
        )
{
    OpenSOAPEnvelopePtr pSoapRequest = NULL;
    OpenSOAPEnvelopePtr pSoapReply = NULL;
    OpenSOAPXMLElmPtr pSecurityTokenResponse = NULL;
    X509* pCertificate = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    if (credentialsCache)
    {
        error = LwKrb5SetDefaultCachePath(credentialsCache, NULL);
        BAIL_ON_LW_ERROR(error);
    }

    error = GenerateSoapStatusRequest(
                url,
                requestId,
                &pSoapRequest);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollCurlSoapRequest(
                url,
                pSoapRequest,
                &pSoapReply);
    BAIL_ON_LW_ERROR(error);

    error = CheckSoapFault(pSoapReply);
    BAIL_ON_LW_ERROR(error);

    error = GetSecurityTokenResponse(pSoapReply, &pSecurityTokenResponse);
    BAIL_ON_LW_ERROR(error);

    error = CheckResponseDisposition(pSecurityTokenResponse);
    BAIL_ON_LW_ERROR(error);

    error = GetResponseCertificate(pSecurityTokenResponse, &pCertificate);
    BAIL_ON_LW_ERROR(error);

cleanup:
    if (error)
    {
        if (pCertificate)
        {
            X509_free(pCertificate);
            pCertificate = NULL;
        }
    }

    if (pSoapRequest)
    {
        OpenSOAPEnvelopeRelease(pSoapRequest);
    }

    if (pSoapReply)
    {
        OpenSOAPEnvelopeRelease(pSoapReply);
    }

    *ppCertificate = pCertificate;

    return error;
}
