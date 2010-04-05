
#include "includes.h"

CENTERROR
GPOGetADDomain(
    char ** ppszDomain
    )
{
	return GPAGetCurrentDomain(ppszDomain);
}

CENTERROR
GPOisJoinedToAD(BOOLEAN *pbIsJoinedToAD)
{
	return GPAIsJoinedToAD(pbIsJoinedToAD);
}

CENTERROR
GPOOpenDirectory(
    char *szDomain,
    char *szServerName,
    HANDLE * phDirectory
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LDAP * ld = NULL;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    int rc = LDAP_VERSION3;

    if ( IsNullOrEmptyString(szServerName) || IsNullOrEmptyString(szDomain) ) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ld = (LDAP *)ldap_open(szServerName, 389);
    if (!ld) {
        ceError = errno;
        GPA_LOG_VERBOSE("Failed to open LDAP connection to domain controller. errno: %d", ceError);
        if (ceError) goto error;
        GPA_LOG_ERROR("Failed to get errno for failed open LDAP connection");
        ceError = CENTERROR_GP_LDAP_ERROR;
        if (ceError) goto error;
    }

    ceError = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &rc);
    if (ceError) {
        ceError = errno;
        GPA_LOG_ERROR("Failed to set LDAP option protocol version. errno: %d", ceError);
        if (ceError) goto error;
        GPA_LOG_ERROR("Failed to get errno for failed set LDAP option");
        ceError = CENTERROR_GP_LDAP_ERROR;
        if (ceError) goto error;
    }

    ceError = ldap_set_option( ld, LDAP_OPT_REFERRALS, (void *)LDAP_OPT_OFF);
    if (ceError) {
        ceError = errno;
        GPA_LOG_ERROR("Failed to set LDAP option to not follow referrals. errno: %d", ceError);
        if (ceError) goto error;
        GPA_LOG_ERROR("Failed to get errno for failed set LDAP option");
        ceError = CENTERROR_GP_LDAP_ERROR;
        if (ceError) goto error;
    }

    ceError = ldap_set_option(ld, LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL,
                              LDAP_OPT_ON);
    if (ceError) {
        ceError = errno;
        GPA_LOG_ERROR("Failed to set LDAP GSS-API option to allow remote principals. errno: %d", ceError);
        if (ceError) goto error;
        GPA_LOG_ERROR("Failed to get errno for failed set LDAP option");
        ceError = CENTERROR_GP_LDAP_ERROR;
        if (ceError) goto error;
    }

    ceError = LwAllocateMemory(sizeof(GPO_DIRECTORY_CONTEXT), (PVOID *)&pDirectory);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pDirectory->ld = ld;

    ceError = ldap_gssapi_bind_s(ld, NULL, NULL);
    if (ceError) {
        GPA_LOG_VERBOSE("Failed to bind to LDAP directory. error: %d", ceError);
        goto error;
    }

    *phDirectory = (HANDLE)pDirectory;
    pDirectory = NULL;

error:

    if (pDirectory)
    {
        GPOCloseDirectory((HANDLE)pDirectory);
    }

    if (ceError)
    {
       *phDirectory = (HANDLE)NULL;
    }

    return(ceError);
}

CENTERROR
GPOOpenDirectory2(
    HANDLE* phDirectory,
    PSTR*   ppszMachineName,
    PSTR*   ppszADDomain
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszADDomain = NULL;
    PSTR pszDomainControllerName = NULL;
    PSTR pszMachineName = NULL;

    ceError = GPAGetDnsSystemNames(NULL, &pszMachineName, NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOGetADDomain(&pszADDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetDomainController(pszADDomain, &pszDomainControllerName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOOpenDirectory(pszADDomain,
                               pszDomainControllerName,
                               phDirectory);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(ppszMachineName != NULL)
    {
        *ppszMachineName = pszMachineName;
        pszMachineName = NULL;
    }

    if(ppszADDomain != NULL)
    {
        *ppszADDomain = pszADDomain;
        pszADDomain = NULL;
    }

error:

    if (pszMachineName) {
        LwFreeString(pszMachineName);
    }

    if (pszADDomain) {
        LwFreeString(pszADDomain);
    }

    if (pszDomainControllerName) {
        LwFreeString(pszDomainControllerName);
    }

    return ceError;
}

void
GPOCloseDirectory(
    HANDLE hDirectory
    )
{
    PGPO_DIRECTORY_CONTEXT pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    if (pDirectory && pDirectory->ld) {
        // ldap_unbind_ext(pDirectory->ld, NULL, NULL);
        ldap_unbind_s(pDirectory->ld);
        LwFreeMemory(pDirectory);
    }

    return;
}


CENTERROR
GPOReadObject(
    HANDLE hDirectory,
    char * szObjectDN,
    char ** szAttributeList,
    LDAPMessage **res
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    struct timeval timeout;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = ldap_search_st(pDirectory->ld,
                             szObjectDN,
                             LDAP_SCOPE_BASE,
                             "(objectClass=*)",
                             szAttributeList,
                             0,
                             &timeout,
                             res);
    BAIL_ON_CENTERIS_ERROR(ceError);

    return(ceError);

error:

    *res = NULL;
    return(ceError);
}


CENTERROR
GPOGetParentDN(
    char * pszObjectDN,
    char ** ppszParentDN
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszParentDN = NULL;
    PSTR pComma = NULL;

    if (!pszObjectDN || !*pszObjectDN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pComma = strchr(pszObjectDN,',');
    if (!pComma) {
        ceError = CENTERROR_GP_NO_PARENT_DN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pComma++;

    ceError= LwAllocateString(pComma, &pszParentDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszParentDN = pszParentDN;

    return(ceError);

error:

    *ppszParentDN = NULL;
    return(ceError);
}

CENTERROR
GPODirectorySearch(
    HANDLE hDirectory,
    char * szObjectDN,
    int scope,
    char * szQuery,
    char ** szAttributeList,
    LDAPMessage **res
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    struct timeval timeout;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = ldap_search_st(pDirectory->ld,
                             szObjectDN,
                             scope,
                             szQuery,
                             szAttributeList,
                             0,
                             &timeout,
                             res);
    if (!CENTERROR_IS_OK(ceError)) {
       if (CENTERROR_EQUAL(ceError, LDAP_NO_SUCH_OBJECT)) {
          GPA_LOG_VERBOSE("Caught LDAP_NO_SUCH_OBJECT Error on ldap search");
          goto error;
       } else if (CENTERROR_EQUAL(ceError, LDAP_INVALID_DN_SYNTAX)) {
          GPA_LOG_VERBOSE("Caught LDAP_INVALID_DN_SYNTAX Error on ldap search");
          goto error;
       } else if (CENTERROR_EQUAL(ceError, LDAP_REFERRAL)) {
          GPA_LOG_ERROR("Caught LDAP_REFERRAL Error on ldap search");
          GPA_LOG_ERROR("LDAP Search Info: DN: [%s]", IsNullOrEmptyString(szObjectDN) ? "<null>" : szObjectDN);
          GPA_LOG_ERROR("LDAP Search Info: scope: [%d]", scope);
          GPA_LOG_ERROR("LDAP Search Info: query: [%s]", IsNullOrEmptyString(szQuery) ? "<null>" : szQuery);
          if (szAttributeList) {
             size_t i;
             for (i = 0; szAttributeList[i] != NULL; i++) {
                 GPA_LOG_ERROR("LDAP Search Info: attribute: [%s]", szAttributeList[i]);
             }
          }
          else {
             GPA_LOG_ERROR("Error: LDAP Search Info: no attributes were specified");
          }
       }
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    return(ceError);

error:

    *res = NULL;
    return(ceError);
}

LDAPMessage*
GPOFirstLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    )
{
//  CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    return ldap_first_entry(pDirectory->ld, res );
}

LDAPMessage*
GPONextLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    )
{
//  CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    return ldap_next_entry(pDirectory->ld, res );
}

LDAP *
GPOGetLDAPSession(
    HANDLE hDirectory
    )
{
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;
    return(pDirectory->ld);
}

CENTERROR
GPAGetLDAPString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PSTR* ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszValue = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ppszValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszValues && ppszValues[0]) {
        ceError = LwAllocateString(ppszValues[0], &pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        ceError = CENTERROR_GP_LDAP_NO_VALUE_FOUND;
        goto error;
    }
    *ppszValue = pszValue;

error:

    if (ppszValues) {
        ldap_value_free(ppszValues);
    }

    return ceError;
}

CENTERROR
GPAGetLDAPUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PDWORD pdwValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszValue = NULL;

    ceError = GPAGetLDAPString(hDirectory, pMessage, pszFieldName, &pszValue);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND) {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszValue) {
        *pdwValue = atoi(pszValue);
    }

error:

    if (pszValue) {
        LwFreeString(pszValue);
    }

    return ceError;
}

CENTERROR
GPAGetLDAPStrings(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PSTR** pppszValues,
    PDWORD pdwNumValues
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszLDAPValues = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszValue = NULL;
    DWORD dwNumValues = 0;
    int   iValue = 0;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ppszLDAPValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszLDAPValues) {
        dwNumValues = ldap_count_values(ppszLDAPValues);
        if (dwNumValues < 0) {

            ceError = CENTERROR_GP_LDAP_NO_VALUE_FOUND;
            goto error;

        } else if (dwNumValues > 0) {

            ceError = LwAllocateMemory((dwNumValues+1)*sizeof(PSTR),
                                       (PVOID*)&ppszValues);
            BAIL_ON_CENTERIS_ERROR(ceError);

            for (iValue = 0; iValue < dwNumValues; iValue++) {

                ceError = LwAllocateString(ppszLDAPValues[iValue], &pszValue);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ppszValues[iValue] = pszValue;

                pszValue = NULL;
            }
            ppszValues[iValue] = NULL;
        }
    } else {
        ceError = CENTERROR_GP_LDAP_NO_VALUE_FOUND;
        goto error;
    }

    if (ppszLDAPValues) {
        ldap_value_free(ppszLDAPValues);
    }

    *pppszValues = ppszValues;
    *pdwNumValues = dwNumValues;

    return ceError;

error:

    if (ppszValues && dwNumValues > 0) {
        for (iValue = 0; iValue < dwNumValues; iValue++) {
            if (ppszValues[iValue]) {
                LwFreeString(ppszValues[iValue]);
            }
        }
        LwFreeMemory(ppszValues);
    }

    if (ppszLDAPValues) {
        ldap_value_free(ppszLDAPValues);
    }

    *pppszValues = NULL;
    *pdwNumValues = 0;

    return ceError;
}

CENTERROR
GPAGetLDAPGUID(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR   pszFieldName,
    PSTR*  ppszGUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszValues = NULL;
    uint8_t rawGUIDValue[16];
    PSTR pszValue = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ppszValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszValues && ppszValues[0]) {

        memcpy(rawGUIDValue, ppszValues[0], sizeof(rawGUIDValue));

        ceError = LwAllocateMemory(37, (PVOID*)&pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf(pszValue,
                "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                rawGUIDValue[3],
                rawGUIDValue[2],
                rawGUIDValue[1],
                rawGUIDValue[0],
                rawGUIDValue[5],
                rawGUIDValue[4],
                rawGUIDValue[7],
                rawGUIDValue[6],
                rawGUIDValue[8],
                rawGUIDValue[9],
                rawGUIDValue[10],
                rawGUIDValue[11],
                rawGUIDValue[12],
                rawGUIDValue[13],
                rawGUIDValue[14],
                rawGUIDValue[15]);
        *ppszGUID = pszValue;
        pszValue = NULL;

    } else {
        ceError = CENTERROR_GP_LDAP_NO_VALUE_FOUND;
        goto error;
    }

error:

    if (ppszValues) {
        ldap_value_free(ppszValues);
    }

    if (pszValue) {
        LwFreeString(pszValue);
    }

    return ceError;
}

CENTERROR
GPAFindComputerDN(
    HANDLE hDirectory,
    PSTR pszHostName,
    PSTR pszDomainName,
    PSTR *ppszComputerDN
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeList[] = {"*", "nTSecurityDescriptor", NULL};
    CHAR szQuery[1024];
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszValue = NULL;
    PSTR pszComputerDN = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = ConvertDomainToDN(pszDomainName, &pszDirectoryRoot);
    BAIL_ON_CENTERIS_ERROR(ceError);

    LwStrToUpper(pszHostName);

    sprintf(szQuery, "(sAMAccountName=%s)", pszHostName);

    ceError = GPODirectorySearch(
        hDirectory,
        pszDirectoryRoot,
        LDAP_SCOPE_SUBTREE,
        szQuery,
        szAttributeList,
        &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pMessage
        );
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;
    } else if (dwCount == 0) {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        ceError = CENTERROR_DUP_DOMAINNAME;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszValue = ldap_get_dn(pDirectory->ld, pMessage);
    if (!pszValue) {
        ceError = CENTERROR_GP_LDAP_GETDN_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString(pszValue, &pszComputerDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszComputerDN = pszComputerDN;

error:

    if (pszValue) {
        ber_memfree(pszValue);
    }

    if (pszDirectoryRoot) {
        LwFreeString(pszDirectoryRoot);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return ceError;
}

CENTERROR
GPAFindUserDN(
    HANDLE hDirectory,
    PSTR pszUserSID,
    PSTR pszDomainName,
    PSTR *ppszUserDN
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeList[] = {"*", "nTSecurityDescriptor", NULL};
    CHAR szQuery[1024];
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszValue = NULL;
    PSTR pszUserDN = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = ConvertDomainToDN(pszDomainName, &pszDirectoryRoot);
    BAIL_ON_CENTERIS_ERROR(ceError);

    // LwStrToUpper(pszUsername);

    sprintf(szQuery, "(objectsid=%s)", pszUserSID);

    ceError = GPODirectorySearch(
        hDirectory,
        pszDirectoryRoot,
        LDAP_SCOPE_SUBTREE,
        szQuery,
        szAttributeList,
        &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pMessage
        );
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;
    } else if (dwCount == 0) {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        ceError = CENTERROR_DUP_DOMAINNAME;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszValue = ldap_get_dn(pDirectory->ld, pMessage);
    if (!pszValue) {
        ceError = CENTERROR_GP_LDAP_GETDN_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString(pszValue, &pszUserDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszUserDN = pszUserDN;

error:

    if (pszValue) {
        ber_memfree(pszValue);
    }

    if (pszDirectoryRoot) {
        LwFreeString(pszDirectoryRoot);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return ceError;
}

VOID
GPAFreeUserAttributes(
    PGPUSER_AD_ATTRS pUserADAttrs
    )
{
    if (pUserADAttrs)
    {
        if (pUserADAttrs->pszDisplayName)
            LwFreeString(pUserADAttrs->pszDisplayName);

        if (pUserADAttrs->pszFirstName)
            LwFreeString(pUserADAttrs->pszFirstName);

        if (pUserADAttrs->pszLastName)
            LwFreeString(pUserADAttrs->pszLastName);

        if (pUserADAttrs->pszADDomain)
            LwFreeString(pUserADAttrs->pszADDomain);

        if (pUserADAttrs->pszKerberosPrincipal)
            LwFreeString(pUserADAttrs->pszKerberosPrincipal);

        if (pUserADAttrs->pszEMailAddress)
            LwFreeString(pUserADAttrs->pszEMailAddress);

        if (pUserADAttrs->pszMSExchHomeServerName)
            LwFreeString(pUserADAttrs->pszMSExchHomeServerName);

        if (pUserADAttrs->pszMSExchHomeMDB)
            LwFreeString(pUserADAttrs->pszMSExchHomeMDB);

        if (pUserADAttrs->pszTelephoneNumber)
            LwFreeString(pUserADAttrs->pszTelephoneNumber);

        if (pUserADAttrs->pszFaxTelephoneNumber)
            LwFreeString(pUserADAttrs->pszFaxTelephoneNumber);

        if (pUserADAttrs->pszMobileTelephoneNumber)
            LwFreeString(pUserADAttrs->pszMobileTelephoneNumber);

        if (pUserADAttrs->pszStreetAddress)
            LwFreeString(pUserADAttrs->pszStreetAddress);

        if (pUserADAttrs->pszPostOfficeBox)
            LwFreeString(pUserADAttrs->pszPostOfficeBox);

        if (pUserADAttrs->pszCity)
            LwFreeString(pUserADAttrs->pszCity);

        if (pUserADAttrs->pszState)
            LwFreeString(pUserADAttrs->pszState);

        if (pUserADAttrs->pszPostalCode)
            LwFreeString(pUserADAttrs->pszPostalCode);

        if (pUserADAttrs->pszCountry)
            LwFreeString(pUserADAttrs->pszCountry);

        if (pUserADAttrs->pszTitle)
            LwFreeString(pUserADAttrs->pszTitle);

        if (pUserADAttrs->pszCompany)
            LwFreeString(pUserADAttrs->pszCompany);

        if (pUserADAttrs->pszDepartment)
            LwFreeString(pUserADAttrs->pszDepartment);

        if (pUserADAttrs->pszHomeDirectory)
            LwFreeString(pUserADAttrs->pszHomeDirectory);

        if (pUserADAttrs->pszHomeDrive)
            LwFreeString(pUserADAttrs->pszHomeDrive);

        if (pUserADAttrs->pszPasswordLastSet)
            LwFreeString(pUserADAttrs->pszPasswordLastSet);

        if (pUserADAttrs->pszUserAccountControl)
            LwFreeString(pUserADAttrs->pszUserAccountControl);

        if (pUserADAttrs->pszMaxMinutesUntilChangePassword)
            LwFreeString(pUserADAttrs->pszMaxMinutesUntilChangePassword);

        if (pUserADAttrs->pszMinMinutesUntilChangePassword)
            LwFreeString(pUserADAttrs->pszMinMinutesUntilChangePassword);

        if (pUserADAttrs->pszMaxFailedLoginAttempts)
            LwFreeString(pUserADAttrs->pszMaxFailedLoginAttempts);

        if (pUserADAttrs->pszAllowedPasswordHistory)
            LwFreeString(pUserADAttrs->pszAllowedPasswordHistory);

        if (pUserADAttrs->pszMinCharsAllowedInPassword)
            LwFreeString(pUserADAttrs->pszMinCharsAllowedInPassword);

        LwFreeMemory(pUserADAttrs);
    }
}

CENTERROR
GPAGetUserAttributes(
    HANDLE hDirectory,
    PSTR pszUserSID,
    PSTR pszDomainName,
    PGPUSER_AD_ATTRS * ppUserADAttrs
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeList[] = {"*", NULL};
    CHAR szQuery[1024];
    LDAPMessage *pUserMessage = NULL;
    LDAPMessage *pDomainMessage = NULL;
    DWORD dwCount = 0;
    PGPUSER_AD_ATTRS pUserADAttrs = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = ConvertDomainToDN(pszDomainName, &pszDirectoryRoot);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szQuery, "(objectsid=%s)", pszUserSID);

    ceError = GPODirectorySearch(
        hDirectory,
        pszDirectoryRoot,
        LDAP_SCOPE_SUBTREE,
        szQuery,
        szAttributeList,
        &pUserMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pUserMessage
        );
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;
    } else if (dwCount == 0) {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        ceError = CENTERROR_DUP_DOMAINNAME;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPODirectorySearch(
        hDirectory,
        pszDirectoryRoot,
        LDAP_SCOPE_BASE,
        "(objectClass=*)",
        szAttributeList,
        &pDomainMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pDomainMessage
        );
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;
    } else if (dwCount == 0) {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        ceError = CENTERROR_DUP_DOMAINNAME;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory(sizeof(GPUSER_AD_ATTRS), (PVOID *) &pUserADAttrs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString(pszDomainName, &pUserADAttrs->pszADDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "displayName",
                               &pUserADAttrs->pszDisplayName);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "givenName",
                               &pUserADAttrs->pszFirstName);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "sn",
                               &pUserADAttrs->pszLastName);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "userPrincipalName",
                               &pUserADAttrs->pszKerberosPrincipal);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "mail",
                               &pUserADAttrs->pszEMailAddress);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "msExchHomeServerName",
                               &pUserADAttrs->pszMSExchHomeServerName);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "homeMDB",
                               &pUserADAttrs->pszMSExchHomeMDB);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "telephoneNumber",
                               &pUserADAttrs->pszTelephoneNumber);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "facsimileTelephoneNumber",
                               &pUserADAttrs->pszFaxTelephoneNumber);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "mobile",
                               &pUserADAttrs->pszMobileTelephoneNumber);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "streetAddress",
                               &pUserADAttrs->pszStreetAddress);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "postOfficeBox",
                               &pUserADAttrs->pszPostOfficeBox);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "l",
                               &pUserADAttrs->pszCity);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "st",
                               &pUserADAttrs->pszState);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "postalCode",
                               &pUserADAttrs->pszPostalCode);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "co",
                               &pUserADAttrs->pszCountry);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "title",
                               &pUserADAttrs->pszTitle);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "company",
                               &pUserADAttrs->pszCompany);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "department",
                               &pUserADAttrs->pszDepartment);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "homeDirectory",
                               &pUserADAttrs->pszHomeDirectory);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "homeDrive",
                               &pUserADAttrs->pszHomeDrive);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "pwdLastSet",
                               &pUserADAttrs->pszPasswordLastSet);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pUserMessage,
                               "userAccountControl",
                               &pUserADAttrs->pszUserAccountControl);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* The settings below are found on the domain container for the user */
    ceError = GPAGetLDAPString(hDirectory,
                               pDomainMessage,
                               "maxPwdAge",
                               &pUserADAttrs->pszMaxMinutesUntilChangePassword);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pDomainMessage,
                               "minPwdAge",
                               &pUserADAttrs->pszMinMinutesUntilChangePassword);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pDomainMessage,
                               "lockoutThreshhold",
                               &pUserADAttrs->pszMaxFailedLoginAttempts);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pDomainMessage,
                               "pwdHistoryLength",
                               &pUserADAttrs->pszAllowedPasswordHistory);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pDomainMessage,
                               "minPwdLength",
                               &pUserADAttrs->pszMinCharsAllowedInPassword);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppUserADAttrs = pUserADAttrs;
    pUserADAttrs = NULL;

error:

    if (pUserADAttrs) {
        GPAFreeUserAttributes(pUserADAttrs);
    }

    if (pszDirectoryRoot) {
        LwFreeString(pszDirectoryRoot);
    }

    if (pUserMessage) {
        ldap_msgfree(pUserMessage);
    }

    if (pDomainMessage) {
        ldap_msgfree(pDomainMessage);
    }

    return ceError;
}


