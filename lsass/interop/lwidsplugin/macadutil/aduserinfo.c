#include "../includes.h"


DWORD
GetUserAttributes(
    HANDLE hDirectory,
    PSTR pszUserSID,
    PSTR pszDomainName,
    PGPUSER_AD_ATTRS * ppUserADAttrs
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR pszDirectoryRoot = NULL;
    PCSTR szAttributeList[] = {"*", NULL};
    CHAR szQuery[1024];
    LDAPMessage *pUserMessage = NULL;
    LDAPMessage *pDomainMessage = NULL;
    DWORD dwCount = 0;
    PGPUSER_AD_ATTRS pUserADAttrs = NULL;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    dwError = ADUConvertDomainToDN(pszDomainName, &pszDirectoryRoot);
    BAIL_ON_MAC_ERROR(dwError);

    sprintf(szQuery, "(objectsid=%s)", pszUserSID);

    dwError = ADUDirectorySearch(
        hDirectory,
        pszDirectoryRoot,
        LDAP_SCOPE_SUBTREE,
        szQuery,
        szAttributeList,
        &pUserMessage);
    BAIL_ON_MAC_ERROR(dwError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pUserMessage
        );
    if (dwCount < 0) {
        dwError = MAC_AD_ERROR_INVALID_NAME;
    } else if (dwCount == 0) {
        dwError = MAC_AD_ERROR_INVALID_NAME;
    } else if (dwCount > 1) {
        dwError = MAC_AD_ERROR_INVALID_NAME;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUDirectorySearch(
        hDirectory,
        pszDirectoryRoot,
        LDAP_SCOPE_BASE,
        "(objectClass=*)",
        szAttributeList,
        &pDomainMessage);
    BAIL_ON_MAC_ERROR(dwError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pDomainMessage
        );
    if (dwCount < 0) {
        dwError = MAC_AD_ERROR_INVALID_NAME;
    } else if (dwCount == 0) {
        dwError = MAC_AD_ERROR_INVALID_NAME;
    } else if (dwCount > 1) {
        dwError = MAC_AD_ERROR_INVALID_NAME;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(GPUSER_AD_ATTRS), (PVOID *) &pUserADAttrs);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateString(pszDomainName, &pUserADAttrs->pszADDomain);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "displayName",
                               &pUserADAttrs->pszDisplayName);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "givenName",
                               &pUserADAttrs->pszFirstName);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "sn",
                               &pUserADAttrs->pszLastName);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "userPrincipalName",
                               &pUserADAttrs->pszKerberosPrincipal);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "mail",
                               &pUserADAttrs->pszEMailAddress);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "msExchHomeServerName",
                               &pUserADAttrs->pszMSExchHomeServerName);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "homeMDB",
                               &pUserADAttrs->pszMSExchHomeMDB);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "telephoneNumber",
                               &pUserADAttrs->pszTelephoneNumber);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "facsimileTelephoneNumber",
                               &pUserADAttrs->pszFaxTelephoneNumber);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "mobile",
                               &pUserADAttrs->pszMobileTelephoneNumber);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "streetAddress",
                               &pUserADAttrs->pszStreetAddress);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "postOfficeBox",
                               &pUserADAttrs->pszPostOfficeBox);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "l",
                               &pUserADAttrs->pszCity);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "st",
                               &pUserADAttrs->pszState);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "postalCode",
                               &pUserADAttrs->pszPostalCode);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "co",
                               &pUserADAttrs->pszCountry);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "title",
                               &pUserADAttrs->pszTitle);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "company",
                               &pUserADAttrs->pszCompany);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "department",
                               &pUserADAttrs->pszDepartment);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "homeDirectory",
                               &pUserADAttrs->pszHomeDirectory);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "homeDrive",
                               &pUserADAttrs->pszHomeDrive);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "pwdLastSet",
                               &pUserADAttrs->pszPasswordLastSet);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pUserMessage,
                               "userAccountControl",
                               &pUserADAttrs->pszUserAccountControl);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    /* The settings below are found on the domain container for the user */
    dwError = ADUGetLDAPString(hDirectory,
                               pDomainMessage,
                               "maxPwdAge",
                               &pUserADAttrs->pszMaxMinutesUntilChangePassword);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pDomainMessage,
                               "minPwdAge",
                               &pUserADAttrs->pszMinMinutesUntilChangePassword);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pDomainMessage,
                               "lockoutThreshhold",
                               &pUserADAttrs->pszMaxFailedLoginAttempts);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pDomainMessage,
                               "pwdHistoryLength",
                               &pUserADAttrs->pszAllowedPasswordHistory);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pDomainMessage,
                               "minPwdLength",
                               &pUserADAttrs->pszMinCharsAllowedInPassword);
    if (dwError == MAC_AD_ERROR_LDAP_NO_VALUE_FOUND)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    *ppUserADAttrs = pUserADAttrs;
    pUserADAttrs = NULL;

error:

    FreeUserAttributes(pUserADAttrs);

    if (pszDirectoryRoot) {
        LwFreeString(pszDirectoryRoot);
    }

    if (pUserMessage) {
        ldap_msgfree(pUserMessage);
    }

    if (pDomainMessage) {
        ldap_msgfree(pDomainMessage);
    }

    return dwError;
}

VOID
FreeUserAttributes(
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

DWORD
CacheUserAttributes(
    uid_t uid,
    PGPUSER_AD_ATTRS pUserADAttrs
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszFileDir = NULL;
    PSTR pszFilePath = NULL;
    PCFGSECTION pUserSettingsList = NULL;
    PCFGSECTION pADSection_Name = NULL;
    PCFGSECTION pADSection_EMail = NULL;
    PCFGSECTION pADSection_Phone = NULL;
    PCFGSECTION pADSection_Address = NULL;
    PCFGSECTION pADSection_Work = NULL;
    PCFGSECTION pADSection_Network = NULL;
    BOOLEAN bDirExists = FALSE;

    LOG("Saving user attributes to user logon cache [uid: %ld, display name: %s]",
        (long)uid,
        pUserADAttrs->pszDisplayName ? pUserADAttrs->pszDisplayName : "<null>");

    dwError = LwAllocateStringPrintf(&pszFileDir, "/var/lib/likewise/lwedsplugin/user-cache/%ld", (long) uid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(&pszFilePath, "/var/lib/likewise/lwedsplugin/user-cache/%ld/ad-user-attrs", (long) uid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwCheckFileTypeExists(pszFileDir, LWFILE_DIRECTORY, &bDirExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (bDirExists == FALSE)
    {
        dwError = LwCreateDirectory(pszFileDir, S_IRUSR|S_IRGRP|S_IROTH);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LWCreateConfigSection(&pUserSettingsList,
                                    &pADSection_Name,
                                    "User AD Name Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_EMail,
                                    &pADSection_EMail,
                                    "User AD EMail Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Phone,
                                    &pADSection_Phone,
                                    "User AD Phone Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Address,
                                    &pADSection_Address,
                                    "User AD Address Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Work,
                                    &pADSection_Work,
                                    "User AD Work Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Network,
                                    &pADSection_Network,
                                    "User AD Network Settings Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    pADSection_Name->pNext = pADSection_EMail;
    pADSection_EMail->pNext = pADSection_Phone;
    pADSection_Phone->pNext = pADSection_Address;
    pADSection_Address->pNext = pADSection_Work;
    pADSection_Work->pNext = pADSection_Network;

    if (pUserADAttrs->pszDisplayName)
    {
        dwError = LWSetConfigValueBySection(pUserSettingsList,
                                            "displayName",
                                            pUserADAttrs->pszDisplayName);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszFirstName)
    {
        dwError = LWSetConfigValueBySection(pADSection_Name,
                                            "givenName",
                                            pUserADAttrs->pszFirstName);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszLastName)
    {
        dwError = LWSetConfigValueBySection(pADSection_Name,
                                            "sn",
                                            pUserADAttrs->pszLastName);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszADDomain)
    {
        dwError = LWSetConfigValueBySection(pADSection_Name,
                                            "userDomain",
                                            pUserADAttrs->pszADDomain);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszKerberosPrincipal)
    {
        dwError = LWSetConfigValueBySection(pADSection_Name,
                                            "userPrincipalName",
                                            pUserADAttrs->pszKerberosPrincipal);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszEMailAddress)
    {
        dwError = LWSetConfigValueBySection(pADSection_EMail,
                                            "mail",
                                            pUserADAttrs->pszEMailAddress);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszMSExchHomeServerName)
    {
        dwError = LWSetConfigValueBySection(pADSection_EMail,
                                            "msExchHomeServerName",
                                            pUserADAttrs->pszMSExchHomeServerName);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszMSExchHomeMDB)
    {
        dwError = LWSetConfigValueBySection(pADSection_EMail,
                                            "homeMDB",
                                            pUserADAttrs->pszMSExchHomeMDB);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszTelephoneNumber)
    {
        dwError = LWSetConfigValueBySection(pADSection_Phone,
                                            "telephoneNumber",
                                            pUserADAttrs->pszTelephoneNumber);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszFaxTelephoneNumber)
    {
        dwError = LWSetConfigValueBySection(pADSection_Phone,
                                            "facsimileTelephoneNumber",
                                            pUserADAttrs->pszFaxTelephoneNumber);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszMobileTelephoneNumber)
    {
        dwError = LWSetConfigValueBySection(pADSection_Phone,
                                            "mobile",
                                            pUserADAttrs->pszMobileTelephoneNumber);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszStreetAddress)
    {
        dwError = LWSetConfigValueBySection(pADSection_Address,
                                            "streetAddress",
                                            pUserADAttrs->pszStreetAddress);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszPostOfficeBox)
    {
        dwError = LWSetConfigValueBySection(pADSection_Address,
                                            "postOfficeBox",
                                            pUserADAttrs->pszPostOfficeBox);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszCity)
    {
        dwError = LWSetConfigValueBySection(pADSection_Address,
                                            "l",
                                            pUserADAttrs->pszCity);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszState)
    {
        dwError = LWSetConfigValueBySection(pADSection_Address,
                                            "st",
                                            pUserADAttrs->pszState);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszPostalCode)
    {
        dwError = LWSetConfigValueBySection(pADSection_Address,
                                            "postalCode",
                                            pUserADAttrs->pszPostalCode);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszCountry)
    {
        dwError = LWSetConfigValueBySection(pADSection_Address,
                                            "co",
                                            pUserADAttrs->pszCountry);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszTitle)
    {
        dwError = LWSetConfigValueBySection(pADSection_Work,
                                            "title",
                                            pUserADAttrs->pszTitle);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszCompany)
    {
        dwError = LWSetConfigValueBySection(pADSection_Work,
                                            "company",
                                            pUserADAttrs->pszCompany);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszDepartment)
    {
        dwError = LWSetConfigValueBySection(pADSection_Work,
                                            "department",
                                            pUserADAttrs->pszDepartment);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszHomeDirectory)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "homeDirectory",
                                            pUserADAttrs->pszHomeDirectory);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszHomeDrive)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "homeDrive",
                                            pUserADAttrs->pszHomeDrive);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszPasswordLastSet)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "pwdLastSet",
                                            pUserADAttrs->pszPasswordLastSet);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszUserAccountControl)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "userAccountControl",
                                            pUserADAttrs->pszUserAccountControl);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszMaxMinutesUntilChangePassword)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "maxPwdAge",
                                            pUserADAttrs->pszMaxMinutesUntilChangePassword);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszMinMinutesUntilChangePassword)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "minPwdAge",
                                            pUserADAttrs->pszMinMinutesUntilChangePassword);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszMaxFailedLoginAttempts)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "lockoutThreshhold",
                                            pUserADAttrs->pszMaxFailedLoginAttempts);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszAllowedPasswordHistory)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "pwdHistoryLength",
                                            pUserADAttrs->pszAllowedPasswordHistory);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->pszMinCharsAllowedInPassword)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "minPwdLength",
                                            pUserADAttrs->pszMinCharsAllowedInPassword);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LWSaveConfigSectionList(pszFilePath, pUserSettingsList);
    BAIL_ON_MAC_ERROR(dwError);

error:

    LW_SAFE_FREE_STRING(pszFilePath);
    LW_SAFE_FREE_STRING(pszFileDir);

    LWFreeConfigSectionList(pUserSettingsList);
    pUserSettingsList = NULL;

    return dwError;
}

DWORD
CollectCurrentADAttributesForUser(
    PSTR pszUserUPN,
    PSTR pszUserDomain
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGPUSER_AD_ATTRS pUserADAttrs = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    BOOLEAN bDeactivateCredContext = FALSE;
    PSTR    pszOrigCachePath = NULL;
    HANDLE hDirectory = (HANDLE)NULL;

    /* Set default credentials to the user's */
    dwError = ADUInitKrb5(pszUserDomain);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    NULL,
                    pszUserUPN,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    LOG("Connecting to AD using these credentials: path: %s, user: %s, domain: %s", pCredContext->pszCachePath, pszUserUPN, pszUserDomain);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = ADUOpenDirectory(pszUserDomain, &hDirectory);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = GetUserAttributes(hDirectory,
                                pCredContext->pszSID,
                                pszUserDomain,
                                &pUserADAttrs);
    if (dwError)
    {
        LOG("Error (%d) while reading user AD attributes from domain DC", dwError);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = CacheUserAttributes(pCredContext->uid, pUserADAttrs);
    if (dwError)
    {
        LOG("Error (%d) while saving user AD attributes to cache", dwError);
        BAIL_ON_MAC_ERROR(dwError);
    }
   
    dwError = FlushDirectoryServiceCache();
    if (dwError)
    {
        LOG("Failed to flush the Mac DirectoryService cache. Error: %d", dwError);
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    FreeUserAttributes(pUserADAttrs);

    if (hDirectory != (HANDLE)NULL)
    {
        ADUCloseDirectory(hDirectory);
    }

    if (pCredContext)
    {
        if (bDeactivateCredContext)
        {
            ADUDeactivateCredContext(pCredContext);
        }

        ADUFreeCredContext(pCredContext);
    }

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            LOG_ERROR("Failed to revert kerberos cache path [code:%d]", dwError2);
        }

        LwFreeMemory(pszOrigCachePath);
    }

    return LWGetMacError(dwError);

error:

    goto cleanup;

}

DWORD
FlushDirectoryServiceCache(
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    BOOLEAN exists = FALSE;

    dwError = LwCheckFileTypeExists("/usr/sbin/lookupd", LWFILE_REGULAR, &exists);
    BAIL_ON_MAC_ERROR(dwError);

    if (!exists)
    {
        system("/usr/sbin/lookupd -flushcache");
    }

    dwError = LwCheckFileTypeExists("/usr/bin/dscacheutil", LWFILE_REGULAR, &exists);
    BAIL_ON_MAC_ERROR(dwError);

    if (exists)
    {
        system("/usr/bin/dscacheutil -flushcache");
    }

error:

    return dwError;
}


