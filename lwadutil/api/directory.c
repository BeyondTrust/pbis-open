#include "includes.h"


DWORD
ADUOpenDirectory(
    PCSTR   pszDomain,
    PHANDLE phDirectory
    )
{
    DWORD dwError = 0;
    LDAP * ld = NULL;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    int rc = LDAP_VERSION3;
    PLWNET_DC_INFO pDCInfo = NULL;

    if ( IsNullOrEmptyString(pszDomain) ) {
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LWNetGetDCName(NULL,
                             pszDomain,
                             NULL,
                             DS_DIRECTORY_SERVICE_REQUIRED | DS_WRITABLE_REQUIRED,
                             &pDCInfo);
    BAIL_ON_LWUTIL_ERROR(dwError);

    ld = (LDAP *)ldap_open(pDCInfo->pszDomainControllerName, 389);
    if (!ld) {
        LWUTIL_LOG_ERROR("Failed to open LDAP connection to domain controller");
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
        LWUTIL_LOG_ERROR("Failed to get errno for failed open LDAP connection");
        dwError = LWUTIL_ERROR_LDAP_OPEN;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &rc);
    if (dwError) {
        LWUTIL_LOG_ERROR("Failed to set LDAP option protocol version");
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
        LWUTIL_LOG_ERROR("Failed to get errno for failed set LDAP option");
        dwError = LWUTIL_ERROR_LDAP_SET_OPTION;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ldap_set_option( ld, LDAP_OPT_REFERRALS, (void *)LDAP_OPT_OFF);
    if (dwError) {
        LWUTIL_LOG_ERROR("Failed to set LDAP option to not follow referrals");
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
        LWUTIL_LOG_ERROR("Failed to get errno for failed set LDAP option");
        dwError = LWUTIL_ERROR_LDAP_SET_OPTION;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ldap_set_option( ld, LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL, (void *)LDAP_OPT_ON);
    if (dwError) {
        LWUTIL_LOG_ERROR("Failed to set LDAP GSS-API option to allow"
                      " remote principals");
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
        LWUTIL_LOG_ERROR("Failed to get errno for failed set LDAP option");
        dwError = LWUTIL_ERROR_LDAP_SET_OPTION;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LWAllocateMemory(sizeof(ADU_DIRECTORY_CONTEXT), (PVOID *)&pDirectory);
    BAIL_ON_LWUTIL_ERROR(dwError);

    pDirectory->ld = ld;

    dwError = ldap_gssapi_bind_s(ld, NULL, NULL);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *phDirectory = (HANDLE)pDirectory;
    pDirectory = NULL;

error:

    LWNET_SAFE_FREE_DC_INFO(pDCInfo);

    if (pDirectory)
    {
        ADUCloseDirectory((HANDLE)pDirectory);
    }

    if (dwError)
    {
       *phDirectory = (HANDLE)NULL;
    }

    return dwError;
}

void
ADUCloseDirectory(
    HANDLE hDirectory
    )
{
    PADU_DIRECTORY_CONTEXT pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    if (pDirectory && pDirectory->ld) {
        // ldap_unbind_ext(pDirectory->ld, NULL, NULL);
        ldap_unbind_s(pDirectory->ld);
        LWFreeMemory(pDirectory);
    }

    return;
}


DWORD
ADUReadObject(
    HANDLE hDirectory,
    char * szObjectDN,
    char ** szAttributeList,
    LDAPMessage **res
    )
{
    DWORD dwError = 0;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    struct timeval timeout;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    dwError = ldap_search_st(pDirectory->ld,
                             szObjectDN,
                             LDAP_SCOPE_BASE,
                             "(objectClass=*)",
                             szAttributeList,
                             0,
                             &timeout,
                             res);
    BAIL_ON_LWUTIL_ERROR(dwError);

    return dwError;

error:

    *res = NULL;
    return dwError;
}

DWORD
ADUDirectorySearch(
    HANDLE hDirectory,
    char * szObjectDN,
    int scope,
    char * szQuery,
    char ** szAttributeList,
    LDAPMessage **res
    )
{
    DWORD dwError = 0;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    struct timeval timeout;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    dwError = ldap_search_st(pDirectory->ld,
                             szObjectDN,
                             scope,
                             szQuery,
                             szAttributeList,
                             0,
                             &timeout,
                             res);
    if (dwError) {
        if (dwError == LDAP_NO_SUCH_OBJECT) {
            LWUTIL_LOG_INFO("Caught LDAP_NO_SUCH_OBJECT Error on ldap search");
            goto error;
        }
        if (dwError == LDAP_REFERRAL) {
            LWUTIL_LOG_INFO("Caught LDAP_REFERRAL Error on ldap search");
            LWUTIL_LOG_INFO("LDAP Search Info: DN: [%s]", IsNullOrEmptyString(szObjectDN) ? "<null>" : szObjectDN);
            LWUTIL_LOG_INFO("LDAP Search Info: scope: [%d]", scope);
            LWUTIL_LOG_INFO("LDAP Search Info: query: [%s]", IsNullOrEmptyString(szQuery) ? "<null>" : szQuery);
            if (szAttributeList) {
                size_t i;
                for (i = 0; szAttributeList[i] != NULL; i++) {
                    LWUTIL_LOG_INFO("LDAP Search Info: attribute: [%s]", szAttributeList[i]);
                }
            }
            else {
                LWUTIL_LOG_INFO("Error: LDAP Search Info: no attributes were specified");
            }
        }
        else
        {
            LWUTIL_LOG_ERROR("Error: LDAP Search failed with result: %d", dwError);
            LWUTIL_LOG_INFO("LDAP Search Info: DN: [%s]", IsNullOrEmptyString(szObjectDN) ? "<null>" : szObjectDN);
            LWUTIL_LOG_INFO("LDAP Search Info: scope: [%d]", scope);
            LWUTIL_LOG_INFO("LDAP Search Info: query: [%s]", IsNullOrEmptyString(szQuery) ? "<null>" : szQuery);
            if (szAttributeList) {
                size_t i;
                for (i = 0; szAttributeList[i] != NULL; i++) {
                    LWUTIL_LOG_INFO("LDAP Search Info: attribute: [%s]", szAttributeList[i]);
                }
            }
            else {
                LWUTIL_LOG_INFO("Error: LDAP Search Info: no attributes were specified");
            }
            dwError = LWUTIL_ERROR_LDAP_QUERY_FAILED;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
    }

    return dwError;

error:

    *res = NULL;
    return dwError;
}

LDAPMessage*
ADUFirstLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    )
{
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    return ldap_first_entry(pDirectory->ld, res );
}

LDAPMessage*
ADUNextLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    )
{
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    return ldap_next_entry(pDirectory->ld, res );
}

LDAP *
ADUGetLDAPSession(
    HANDLE hDirectory
    )
{
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;
    return(pDirectory->ld);
}

DWORD
ADUGetLDAPString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PSTR* ppszValue
    )
{
    DWORD dwError = 0;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszValue = NULL;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    ppszValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszValues && ppszValues[0]) {
        dwError = LWAllocateString(ppszValues[0], &pszValue);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    *ppszValue = pszValue;

error:

    if (ppszValues) {
        ldap_value_free(ppszValues);
    }

    return dwError;
}

DWORD
ADUPutLDAPString(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PSTR   pszFieldName,
    PSTR   pszValue
    )
{
    DWORD dwError = 0;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    char* modvals[2];
    LDAPMod  mod;
    LDAPMod* mods[2];
    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    modvals[0] = pszValue;
    modvals[1] = NULL;
    
    mod.mod_op = LDAP_MOD_REPLACE;
    mod.mod_type = pszFieldName;
    mod.mod_values = modvals;
    
    mods[0] = &mod;
    mods[1] = NULL;
    
    dwError = ldap_modify_s(pDirectory->ld, pszDN, mods);
    if (dwError)
    {
        LWUTIL_LOG_ERROR("Failed to update LDAP object attribute string with error: %d, errno: %d", dwError, errno);
    }
    BAIL_ON_LWUTIL_ERROR(dwError);
    
error:

    return dwError;
}

DWORD
ADUGetLDAPUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;

    dwError = ADUGetLDAPString(hDirectory, pMessage, pszFieldName, &pszValue);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if (pszValue) {
        *pdwValue = atoi(pszValue);
    }

error:

    if (pszValue) {
        LWFreeString(pszValue);
    }

    return dwError;
}

DWORD
ADUPutLDAPUInt32(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PSTR pszFieldName,
    DWORD dwValue
    )
{
    DWORD dwError = 0;
    char szValue[256];

    memset(szValue, 0, sizeof(szValue));
    sprintf(szValue, "%d", dwValue);
    
    dwError = ADUPutLDAPString(hDirectory, pszDN, pszFieldName, szValue);
    if (dwError)
    {
        LWUTIL_LOG_ERROR("Failed to update LDAP object attribute integer with error: %d, errno: %d", dwError, errno);
    }
    BAIL_ON_LWUTIL_ERROR(dwError);

error:

    return dwError;
}
