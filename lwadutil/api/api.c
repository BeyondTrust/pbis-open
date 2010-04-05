/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        adutils.c
 *
 * Abstract:
 *
 *       AD Utility API
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
GetADDomain(
    PSTR* ppszDomain
    )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;
    PSTR pszDomain_out = NULL;

    dwError = LWNetGetCurrentDomain(&pszDomain);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateString(pszDomain, &pszDomain_out);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *ppszDomain = pszDomain_out;

cleanup:

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
    }

    return dwError;

error:

    *ppszDomain = NULL;

    LW_SAFE_FREE_STRING(pszDomain_out);

    goto cleanup;
}

DWORD
EnumEnabledGPOs(
    DWORD                dwPolicyType,
    PCSTR                pszDomainName,
    PSTR                 pszClientGUID,
    PGROUP_POLICY_OBJECT* ppGPOs
    )
{
    DWORD dwError = 0;
    PSTR  pszDomainDN = NULL;
    PSTR  pszSearchDN = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PGROUP_POLICY_OBJECT pGroupPolicyObjects = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR              pszOrigCachePath = NULL;
    BOOLEAN           bDeactivateCredContext = FALSE;
    PSTR              pszUPN = NULL;

    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = EINVAL;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (getuid() != 0)
    {
        dwError = GetUserPrincipalName(getuid(), &pszUPN);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    pszUPN,
                    &pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUOpenDirectory(pszDomainName, &hDirectory);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUConvertDomainToDN(pszDomainName, &pszDomainDN);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszSearchDN,
                    "CN=Policies,CN=System,%s",
                    pszDomainDN);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = ADUGetAllMCXGPOList(
                    hDirectory,
                    dwPolicyType,
                    pszSearchDN,
                    pszClientGUID,
                    &pGroupPolicyObjects);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *ppGPOs = pGroupPolicyObjects;
    pGroupPolicyObjects = NULL;

cleanup:

    if (pCredContext)
    {
        if (bDeactivateCredContext)
        {
            ADUDeactivateCredContext(pCredContext);
        }

        ADUFreeCredContext(pCredContext);
    }

    if (hDirectory != (HANDLE)NULL)
    {
        ADUCloseDirectory(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszDomainDN);
    LW_SAFE_FREE_STRING(pszSearchDN);
    LW_SAFE_FREE_STRING(pszUPN);

    ADU_SAFE_FREE_GPO_LIST(pGroupPolicyObjects);

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            LWUTIL_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
    }

    return dwError;

error:

    *ppGPOs = NULL;

    goto cleanup;
}

DWORD
GetSpecificGPO(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    )
{
    DWORD dwError = 0;
    PSTR pszDomainDN = NULL;
    PSTR pszDomain = NULL;
    PSTR pszSearchDN = NULL;
    PSTR pszComputerCache = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PGROUP_POLICY_OBJECT pFound = NULL;

    /* pszDomainName can be NULL sometimes, here we will default to our
       configured joined domain */
    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = GetADDomain(&pszDomain);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    else
    {
        dwError = LWAllocateString(pszDomainName, &pszDomain);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszGPOName))
    {
        dwError = EINVAL;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ADUOpenDirectory(pszDomain, &hDirectory);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUConvertDomainToDN(pszDomain, &pszDomainDN);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszSearchDN,
                    "CN=Policies,CN=System,%s",
                    pszDomainDN);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUGetMCXGPO(
                    hDirectory,
                    pszSearchDN,
                    pszGPOName,
                    &pFound);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if (!pFound)
    {
        dwError = LWUTIL_ERROR_INVALID_NAME;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    *ppGPO = pFound;
    pFound = NULL;

cleanup:

    if (hDirectory != (HANDLE)NULL)
    {
        ADUCloseDirectory(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszDomainDN);
    LW_SAFE_FREE_STRING(pszSearchDN);
    LW_SAFE_FREE_STRING(pszComputerCache);
    LW_SAFE_FREE_STRING(pszDomain);

    return dwError;

error:

    *ppGPO = NULL;

    ADU_SAFE_FREE_GPO_LIST(pFound);

    goto cleanup;
}

DWORD
IsSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PSTR                 pszClientGUID,
    PBOOLEAN             pbEnabled
    )
{
    DWORD dwError = 0;
    PGROUP_POLICY_OBJECT pMatchedPolicy = NULL;

    switch(dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:
            dwError = ADUComputeCSEList(
                            MACHINE_GROUP_POLICY,
                            pszClientGUID,
                            pGPO,
                            &pMatchedPolicy);
            BAIL_ON_LWUTIL_ERROR(dwError);
            break;

        case USER_GROUP_POLICY:
            dwError = ADUComputeCSEList(
                            USER_GROUP_POLICY,
                            pszClientGUID,
                            pGPO,
                            &pMatchedPolicy);
            BAIL_ON_LWUTIL_ERROR(dwError);
            break;
        default:

            dwError = LWUTIL_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWUTIL_ERROR(dwError);
    }

    *pbEnabled = (pMatchedPolicy != NULL);
    LWUTIL_LOG_INFO("ADUAdapter_IsSettingEnabledForGPO(type %s): %s",
                    dwPolicyType == MACHINE_GROUP_POLICY ? "Computer" : "User",
                    pMatchedPolicy ? "yes" : "no");

cleanup:

    ADU_SAFE_FREE_GPO_LIST(pMatchedPolicy);

    return dwError;

error:

    *pbEnabled = FALSE;

    goto cleanup;
}


DWORD
SaveValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    PSTR                 pszClientGUID,
    DWORD                dwPolicyType,
    PSTR                 pszPath
    )
{
    DWORD   dwError = 0;
    BOOLEAN bPolicyExists = FALSE;
    PSTR    pszSourceFolder = NULL;
    PSTR    pszAllMCXFiles = NULL;
    DWORD   dwFileVersion = 0;
    DWORD   dwVersion = 0;
    WORD    wUserFileVersion = 0;
    WORD    wComputerFileVersion = 0;
    WORD    wUserVersion = 0;
    WORD    wComputerVersion = 0;
    char    szCseGuid[256];
    PSTR    pszDomainName = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR    pszOrigCachePath = NULL;
    BOOLEAN bCurrent = FALSE;
    BOOLEAN bDeactivateCredContext = FALSE;
    PSTR    pszUPN = NULL;

    LWUTIL_LOG_INFO("Saving %s Settings for GPO (%s)",
        dwPolicyType == MACHINE_GROUP_POLICY ? "machine" :
        dwPolicyType == USER_GROUP_POLICY ? "user" :
        "unknown",
        pGPO->pszDisplayName);

    BAIL_ON_INVALID_POINTER(pGPO);

    memset(szCseGuid, 0, sizeof(szCseGuid));

    dwError = LWNetGetCurrentDomain(&pszDomainName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if (getuid() != 0)
    {
        dwError = GetUserPrincipalName(getuid(), &pszUPN);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    pszUPN,
                    &pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = IsCacheDataCurrentForGPO(
                    pGPO,
                    &dwVersion,
                    &dwFileVersion,
                    &bCurrent,
                    pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    ADUGetComputerAndUserVersionNumbers(dwVersion, &wUserVersion, &wComputerVersion);
    ADUGetComputerAndUserVersionNumbers(dwFileVersion, &wUserFileVersion, &wComputerFileVersion);

    if (wUserVersion < wUserFileVersion)
    {
        wUserVersion = wUserFileVersion;
    }

    if (wComputerVersion < wComputerFileVersion)
    {
        wComputerVersion = wComputerFileVersion;
    }

    switch(dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:

            strcpy(szCseGuid, pszClientGUID);

            dwError = GetCachedPolicyFiles(
                            MACHINE_GROUP_POLICY,
                            pGPO->pszgPCFileSysPath,
                            szCseGuid,
                            pszPath,
                            &pszSourceFolder,
                            &bPolicyExists);
            BAIL_ON_LWUTIL_ERROR(dwError);

            wComputerVersion++;

            break;

        case USER_GROUP_POLICY:

            strcpy(szCseGuid, pszClientGUID);

            dwError = GetCachedPolicyFiles(
                            USER_GROUP_POLICY,
                            pGPO->pszgPCFileSysPath,
                            szCseGuid,
                            NULL,
                            &pszSourceFolder,
                            &bPolicyExists);
            BAIL_ON_LWUTIL_ERROR(dwError);

            wUserVersion++;

            break;

        default:

            dwError = LWUTIL_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwVersion = ADUGetVersionFromUserAndComputer(wUserVersion, wComputerVersion);

    dwError = ADUDeactivateCredContext(pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    bDeactivateCredContext = FALSE;

    ADUFreeCredContext(pCredContext);
    pCredContext = NULL;

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    pszUPN,
                    &pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            LWUTIL_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LW_SAFE_FREE_STRING(pszOrigCachePath);
    }

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = ADUPutPolicyFiles(
                    pszPath,
                    TRUE /* replace destination */,
                    dwPolicyType,
                    pGPO->pszgPCFileSysPath,
                    szCseGuid);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUSetGPTVersionNumber(pGPO->pszgPCFileSysPath, dwVersion, pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUSetPolicyVersionInAD(pGPO, dwVersion);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszSourceFolder);
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszAllMCXFiles);
    LW_SAFE_FREE_STRING(pszUPN);

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            LWUTIL_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
    }

    if (pCredContext)
    {
        if (bDeactivateCredContext)
        {
            ADUDeactivateCredContext(pCredContext);
        }

        ADUFreeCredContext(pCredContext);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
GetValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PSTR                 pszClientGUID,
    PSTR                 pszPath
    )
{
    DWORD     dwError = 0;
    PSTR      pszMachinePolicyPath = NULL;
    PSTR      pszUserPolicyPath = NULL;
    PSTR      pszDomainName = NULL;
    BOOLEAN   bMachinePolicyExists = FALSE;
    BOOLEAN   bUserPolicyExists = FALSE;
    PADU_CRED_CONTEXT pCredContext = NULL;
    BOOLEAN   bDeactivateCredContext = FALSE;
    PSTR      pszOrigCachePath = NULL;
    PSTR      pszUPN = NULL;

    /* Set default credentials to the machine's */
    dwError = LWNetGetCurrentDomain(&pszDomainName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if (getuid() != 0)
    {
        dwError = GetUserPrincipalName(getuid(), &pszUPN);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    pszUPN,
                    &pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_LWUTIL_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = GetCurrentSettingsForGPO(pGPO,
                                       pszClientGUID,
                                       &bMachinePolicyExists,
                                       &bUserPolicyExists,
                                       &pszMachinePolicyPath,
                                       &pszUserPolicyPath,
                                       pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pCredContext)
    {
        ADUDeactivateCredContext(pCredContext);

        ADUFreeCredContext(pCredContext);
    }

    LW_SAFE_FREE_STRING(pszMachinePolicyPath);
    LW_SAFE_FREE_STRING(pszUserPolicyPath);
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszUPN);

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            LWUTIL_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AuthenticateUser(
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    HANDLE hLsaServer = (HANDLE)NULL;

    dwError = LsaOpenServer(&hLsaServer);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LsaAuthenticateUser(
                  hLsaServer,
                  pszUsername,
                  pszPassword);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (hLsaServer != (HANDLE)NULL) {
       LsaCloseServer(hLsaServer);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
GetUserPrincipalName(
    uid_t  uid,
    PSTR * ppszUserPrincipalName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;
    PSTR pszUPN = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LsaFindUserById(hLsaConnection,
                              uid,
                              dwUserInfoLevel,
                              &pUserInfo);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if (IsNullOrEmptyString(((PLSA_USER_INFO_1)pUserInfo)->pszUPN))
    {
        dwError = LWUTIL_ERROR_UPN_NOT_FOUND;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LWAllocateString(((PLSA_USER_INFO_1)pUserInfo)->pszUPN, &pszUPN);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *ppszUserPrincipalName = pszUPN;

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    *ppszUserPrincipalName = NULL;

    if (pszUPN)
    {
        LWFreeString(pszUPN);
    }

    goto cleanup;
}

DWORD
NotifyUserLogon(
    PCSTR pszUserName
    )
{
    DWORD dwError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;

    dwError = GPOClientOpenContext(&hGPConnection);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = GPOClientProcessLogin(hGPConnection, pszUserName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (hGPConnection != (HANDLE)NULL) {
        GPOClientCloseContext(hGPConnection);
    }

    return dwError;

error:

    LWUTIL_LOG_INFO("Failed to notify group policy about user logon (%s) with error: %d", pszUserName, dwError);

    goto cleanup;
}

DWORD
NotifyUserLogoff(
    PCSTR pszUserName
    )
{
    DWORD dwError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;

    dwError = GPOClientOpenContext(&hGPConnection);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = GPOClientProcessLogout(hGPConnection, pszUserName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (hGPConnection != (HANDLE)NULL) {
        GPOClientCloseContext(hGPConnection);
    }

    return dwError;

error:

    LWUTIL_LOG_INFO("Failed to notify group policy about user logoff (%s) with error: %d", pszUserName, dwError);

    goto cleanup;
}


static
void
FreeADUserInfo(
    PLWUTIL_USER_ATTRIBUTES pUserADAttrs
    )
{
    if (pUserADAttrs)
    {
        if (pUserADAttrs->pszDisplayName)
            LWFreeString(pUserADAttrs->pszDisplayName);

        if (pUserADAttrs->pszFirstName)
            LWFreeString(pUserADAttrs->pszFirstName);

        if (pUserADAttrs->pszLastName)
            LWFreeString(pUserADAttrs->pszLastName);

        if (pUserADAttrs->pszADDomain)
            LWFreeString(pUserADAttrs->pszADDomain);

        if (pUserADAttrs->pszKerberosPrincipal)
            LWFreeString(pUserADAttrs->pszKerberosPrincipal);

        if (pUserADAttrs->pszEMailAddress)
            LWFreeString(pUserADAttrs->pszEMailAddress);

        if (pUserADAttrs->pszMSExchHomeServerName)
            LWFreeString(pUserADAttrs->pszMSExchHomeServerName);

        if (pUserADAttrs->pszMSExchHomeMDB)
            LWFreeString(pUserADAttrs->pszMSExchHomeMDB);

        if (pUserADAttrs->pszTelephoneNumber)
            LWFreeString(pUserADAttrs->pszTelephoneNumber);

        if (pUserADAttrs->pszFaxTelephoneNumber)
            LWFreeString(pUserADAttrs->pszFaxTelephoneNumber);

        if (pUserADAttrs->pszMobileTelephoneNumber)
            LWFreeString(pUserADAttrs->pszMobileTelephoneNumber);

        if (pUserADAttrs->pszStreetAddress)
            LWFreeString(pUserADAttrs->pszStreetAddress);

        if (pUserADAttrs->pszPostOfficeBox)
            LWFreeString(pUserADAttrs->pszPostOfficeBox);

        if (pUserADAttrs->pszCity)
            LWFreeString(pUserADAttrs->pszCity);

        if (pUserADAttrs->pszState)
            LWFreeString(pUserADAttrs->pszState);

        if (pUserADAttrs->pszPostalCode)
            LWFreeString(pUserADAttrs->pszPostalCode);

        if (pUserADAttrs->pszCountry)
            LWFreeString(pUserADAttrs->pszCountry);

        if (pUserADAttrs->pszTitle)
            LWFreeString(pUserADAttrs->pszTitle);

        if (pUserADAttrs->pszCompany)
            LWFreeString(pUserADAttrs->pszCompany);

        if (pUserADAttrs->pszDepartment)
            LWFreeString(pUserADAttrs->pszDepartment);

        if (pUserADAttrs->pszHomeDirectory)
            LWFreeString(pUserADAttrs->pszHomeDirectory);

        if (pUserADAttrs->pszHomeDrive)
            LWFreeString(pUserADAttrs->pszHomeDrive);

        if (pUserADAttrs->pszPasswordLastSet)
            LWFreeString(pUserADAttrs->pszPasswordLastSet);

        if (pUserADAttrs->pszUserAccountControl)
            LWFreeString(pUserADAttrs->pszUserAccountControl);

        if (pUserADAttrs->pszMaxMinutesUntilChangePassword)
            LWFreeString(pUserADAttrs->pszMaxMinutesUntilChangePassword);

        if (pUserADAttrs->pszMinMinutesUntilChangePassword)
            LWFreeString(pUserADAttrs->pszMinMinutesUntilChangePassword);

        if (pUserADAttrs->pszMaxFailedLoginAttempts)
            LWFreeString(pUserADAttrs->pszMaxFailedLoginAttempts);

        if (pUserADAttrs->pszAllowedPasswordHistory)
            LWFreeString(pUserADAttrs->pszAllowedPasswordHistory);

        if (pUserADAttrs->pszMinCharsAllowedInPassword)
            LWFreeString(pUserADAttrs->pszMinCharsAllowedInPassword);

        LWFreeMemory(pUserADAttrs);
    }
}

DWORD
GetADUserInfo(
    uid_t uid,
    PLWUTIL_USER_ATTRIBUTES * ppadUserInfo
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PLWUTIL_USER_ATTRIBUTES padUserInfo = NULL;
    PSTR pszValue = NULL;
    PCFGSECTION pSectionList = NULL;
    char      szUserAttrCacheFile[PATH_MAX] = { 0 };

    if (!uid)
    {
        LWUTIL_LOG_ERROR("Called with invalid parameter");
        goto cleanup;
    }

    dwError = LWAllocateMemory(sizeof(LWUTIL_USER_ATTRIBUTES), (PVOID *) &padUserInfo);
    BAIL_ON_LWUTIL_ERROR(dwError);

    sprintf(szUserAttrCacheFile, "/var/lib/likewise/grouppolicy/user-cache/%ld/ad-user-attrs", (long) uid);

    /* Get user attributes that apply to user by parsing ad-user-attrs for specific user*/
    dwError = LWParseConfigFile(szUserAttrCacheFile,
                                &pSectionList,
                                FALSE);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "displayName",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszDisplayName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "givenName",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszFirstName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "sn",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszLastName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "userDomain",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszADDomain = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "userPrincipalName",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszKerberosPrincipal = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "mail",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszEMailAddress = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "msExchHomeServerName",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMSExchHomeServerName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "homeMDB",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMSExchHomeMDB = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "telephoneNumber",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "facsimileTelephoneNumber",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszFaxTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "mobile",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMobileTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "streetAddress",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszStreetAddress = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "postOfficeBox",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPostOfficeBox = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "l",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCity = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "st",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszState = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "postalCode",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPostalCode = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "co",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCountry = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "title",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszTitle = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "company",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCompany = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "department",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszDepartment = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "homeDirectory",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszHomeDirectory = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "homeDrive",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszHomeDrive = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "pwdLastSet",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPasswordLastSet = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "userAccountControl",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszUserAccountControl = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "maxPwdAge",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMaxMinutesUntilChangePassword = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "minPwdAge",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMinMinutesUntilChangePassword = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "lockoutThreshhold",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMaxFailedLoginAttempts = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "pwdHistoryLength",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszAllowedPasswordHistory = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "minPwdLength",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMinCharsAllowedInPassword = pszValue;
        pszValue = NULL;
    }

    dwError = LWUTIL_ERROR_SUCCESS;
    *ppadUserInfo = padUserInfo;
    padUserInfo = NULL;

cleanup:

    if (padUserInfo)
    {
        FreeADUserInfo(padUserInfo);
    }

    if (pSectionList)
    {
        LWFreeConfigSectionList(pSectionList);
    }

    if (pszValue)
    {
        LWFreeString(pszValue);
    }

    return dwError;

error:

    *ppadUserInfo = NULL;
    dwError = LWUTIL_ERROR_SUCCESS;

    goto cleanup;
}


#if 0
DWORD
GetAccessCheckData(
    PSTR    pszAllowList,
    PVOID * ppAccessData
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PVOID pAccessData = NULL;
    DWORD  dwCount = 0;
    DWORD  dwIndex = 0;
    PCSTR  cp = NULL;
    PCSTR  cp2 = NULL;
    PSTR   cp3 = NULL;
    PSTR * ppczStrArray = NULL;

    for (cp = pszAllowList; *cp !=  0; cp++)
    {
        if (*cp == ',') dwCount++;
    }

    dwCount++;

    dwError = LWAllocateMemory((dwCount+1)*sizeof(PCSTR), (PVOID *)&ppczStrArray);
    BAIL_ON_LWUTIL_ERROR(dwError);

    cp = pszAllowList;
    for ( ;; )
    {
         cp2 = strchr(cp, ',');
         if (cp2)
         {
             dwError = LWStrndup( cp, cp2 - cp, &cp3 );
             BAIL_ON_LWUTIL_ERROR(dwError);
         }
         else
         {
             dwError = LWStrndup( cp, strlen(cp), &cp3 );
             BAIL_ON_LWUTIL_ERROR(dwError);
         }

         LWStripWhitespace(cp3);

         if ( strlen(cp3) > 0 )
         {
             ppczStrArray[dwIndex++] = cp3;
         }
         else
         {
             LWFreeMemory(cp3);
         }

         if (!cp2) break;

         cp = ++cp2;
    }

    if ( dwIndex == 0 )
    {
        *ppAccessData = NULL;
        goto cleanup;
    }

    if (gpfnLsaAccessGetData)
    {
        dwError = gpfnLsaAccessGetData((PCSTR *)ppczStrArray, &pAccessData);
    }
    else
    {
        dwError = LWUTIL_ERROR_LOOKUP_SYMBOL_FAILED;
    }
    BAIL_ON_LWUTIL_ERROR(dwError);

    *ppAccessData = pAccessData;
    pAccessData = NULL;

cleanup:

    if ( ppczStrArray )
    {
        for ( dwIndex = 0 ; ppczStrArray[dwIndex] != NULL ; dwIndex++ )
        {
            LWFreeString(ppczStrArray[dwIndex]);
        }

        LWFreeMemory(ppczStrArray);
    }

    return dwError;

error:

    *ppAccessData = NULL;

    goto cleanup;
}

DWORD
CheckUserForAccess(
    PCSTR  pszUsername,
    PCVOID pAccessData
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;

    if (gpfnLsaAccessCheckData)
    {
        dwError = gpfnLsaAccessCheckData(pszUsername, pAccessData);
    }
    else
    {
        dwError = LWUTIL_ERROR_LOOKUP_SYMBOL_FAILED;
    }
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
FreeAccessCheckData(
    PVOID pAccessData
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;

    if (gpfnLsaAccessFreeData)
    {
        dwError = gpfnLsaAccessFreeData(pAccessData);
    }
    else
    {
        dwError = LWUTIL_ERROR_LOOKUP_SYMBOL_FAILED;
    }
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}
#endif

