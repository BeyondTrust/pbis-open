#include "includes.h"

DWORD
ADUGetAllPolicies(
    HANDLE hDirectory,
    DWORD  dwPolicyType,
    PCSTR  pszDN,
    PSTR   pszClientGUID,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR szAttributeList[] = { "distinguishedName", NULL };
    PGROUP_POLICY_OBJECT pGPObjectList = NULL;
    PGROUP_POLICY_OBJECT pGPObject = NULL;
    LDAPMessage* pMessage = NULL;
    LDAPMessage* pLDAPMessage = NULL;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    DWORD dwCount = 0;
    PSTR pszValue = NULL;
    CHAR szQuery[PATH_MAX+1];

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    memset (szQuery,0,(PATH_MAX+1));

    if(dwPolicyType == MACHINE_GROUP_POLICY)
    {
        sprintf(szQuery,
                "(&(objectclass=groupPolicyContainer)(|(gPCMachineExtensionNames=*{%s}*)))",
                pszClientGUID);
    }
    else if(dwPolicyType == USER_GROUP_POLICY)
    {
        sprintf(szQuery,
                "(&(objectclass=groupPolicyContainer)(|(gPCUserExtensionNames=*{%s}*)))",
                pszClientGUID);
    }


    dwError = ADUDirectorySearch(
        hDirectory,
        (PSTR) pszDN,
        LDAP_SCOPE_ONELEVEL,
        szQuery,
        szAttributeList,
        &pMessage);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pMessage
        );

    if (dwCount > 0)
    {
        pLDAPMessage = ADUFirstLDAPEntry(hDirectory, pMessage);
    }

    while(pLDAPMessage != NULL)
    {
        dwError = ADUGetLDAPString(hDirectory,
                                   pLDAPMessage,
                                   "distinguishedName",
                                   &pszValue);
        BAIL_ON_LWUTIL_ERROR(dwError);

        dwError = LWAllocateMemory(sizeof(GROUP_POLICY_OBJECT), (PVOID*)&pGPObject);
        BAIL_ON_LWUTIL_ERROR(dwError);

        pGPObject->pszPolicyDN = pszValue;
        pszValue = NULL;

        if (pGPObjectList != NULL)
        {
            pGPObject->pNext = pGPObjectList;
            pGPObjectList = pGPObject;
        }
        else
        {
            pGPObjectList = pGPObject;
        }

        pGPObject = NULL;

        pLDAPMessage = ADUNextLDAPEntry(hDirectory, pLDAPMessage);
    }

    if (*ppGroupPolicyObjects != NULL)
    {
        (*ppGroupPolicyObjects)->pNext = pGPObjectList;
    }
    else
    {
        *ppGroupPolicyObjects = pGPObjectList;
    }

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    if (pLDAPMessage)
    {
        ldap_msgfree(pLDAPMessage);
    }

    return dwError;

cleanup:

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    if (pLDAPMessage)
    {
        ldap_msgfree(pLDAPMessage);
    }

    LW_SAFE_FREE_STRING(pszValue);
    ADU_SAFE_FREE_GPO_LIST (pGPObject);
    ADU_SAFE_FREE_GPO_LIST (pGPObjectList);

    return dwError;

error:

    if (ppGroupPolicyObjects)
        *ppGroupPolicyObjects = NULL;

    goto cleanup;
}

DWORD
ADUGetPolicy(
    HANDLE hDirectory,
    PCSTR pszDN,
    PCSTR pszGPOName,
    PGROUP_POLICY_OBJECT * ppGPO
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR szAttributeList[] = { "distinguishedName", NULL };
    char szQuery[512] = { 0 };
    PGROUP_POLICY_OBJECT pGPObject = NULL;
    LDAPMessage* pMessage = NULL;
    PADU_DIRECTORY_CONTEXT pDirectory = NULL;
    DWORD dwCount = 0;
    PSTR pszValue = NULL;

    pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    sprintf(szQuery, "(&(objectclass=groupPolicyContainer)(%s=%s))", ADU_DISPLAY_NAME_ATTR, pszGPOName);

    dwError = ADUDirectorySearch(
        hDirectory,
        (PSTR) pszDN,
        LDAP_SCOPE_ONELEVEL,
        szQuery,
        szAttributeList,
        &pMessage);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pMessage
        );

    if (dwCount < 0) {
        dwError = LWUTIL_ERROR_NO_SUCH_POLICY;
    } else if (dwCount == 0) {
        dwError = LWUTIL_ERROR_NO_SUCH_POLICY;
    } else if (dwCount > 1) {
        dwError = LWUTIL_ERROR_NO_SUCH_POLICY;
    }
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pMessage,
                               "distinguishedName",
                               &pszValue);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateMemory(sizeof(GROUP_POLICY_OBJECT), (PVOID*)&pGPObject);
    BAIL_ON_LWUTIL_ERROR(dwError);

    pGPObject->pszPolicyDN = pszValue;
    pszValue = NULL;

    *ppGPO = pGPObject;
    pGPObject = NULL;

cleanup:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    LW_SAFE_FREE_STRING(pszValue);
    ADU_SAFE_FREE_GPO_LIST (pGPObject);

    return dwError;

error:

    if (ppGPO)
        *ppGPO = NULL;

    goto cleanup;
}

static
DWORD
ADUGetGPTINIFile(
    PSTR  pszgPCFileSysPath,
    PSTR* ppszGPTINIFilePath,
    PSTR  pszPath
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    char szSourceFilePath[PATH_MAX + 1];
    PSTR pszDestFilePath = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszPolicyIdentifier = NULL;

    dwError = LWAllocateMemory( PATH_MAX + 1,
                                (PVOID*)&pszDestFilePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError =  ADUCrackFileSysPath(pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_LWUTIL_ERROR(dwError);

    sprintf( szSourceFilePath,
             "%s\\%s",
             pszSourcePath,
             "GPT.INI");

    sprintf(pszDestFilePath,
            "%s/%s_GPT.INI",
            pszPath,
            pszPolicyIdentifier);

    dwError = ADUSMBGetFile(pszDomainName,
                             szSourceFilePath,
                             pszDestFilePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *ppszGPTINIFilePath = pszDestFilePath;
    pszDestFilePath = NULL;

cleanup:

    if (pszDomainName) {
        LWFreeString(pszDomainName);
    }

    if (pszSourcePath) {
        LWFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier) {
        LWFreeString(pszPolicyIdentifier);
    }

    if (pszDestFilePath) {
        LWFreeString(pszDestFilePath);
    }

    return dwError;

error:

    if (ppszGPTINIFilePath)
        *ppszGPTINIFilePath = NULL;

    goto cleanup;
}

static
DWORD
ADUPutGPTINIFile(
    PSTR  pszFolderPath,
    PSTR  pszgPCFileSysPath
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR pszDestPath = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszPolicyIdentifier = NULL;

    /* This routine expects the local layout of cached policy files to be found under LWDS_ADMIN_CACHE_DIR,
       here there should be sub-directories for each policy identifier and a file GPT.ini which has
       been updated to reflect the version information that needs to be copied to the AD DC.  This
       function will determine the policy identifier and build the path to the local GPT.INI file and
       the path to the GPT.INI file in the syvol location for the specific GPO.*/
    dwError =  ADUCrackFileSysPath(pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszDestPath,
                                    &pszPolicyIdentifier);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUSMBPutFile(pszDomainName,
                            pszFolderPath,
                            "GPT.INI",
                            pszDestPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pszDomainName) {
        LWFreeString(pszDomainName);
    }

    if (pszPolicyIdentifier) {
        LWFreeString(pszPolicyIdentifier);
    }

    if (pszDestPath) {
        LWFreeString(pszDestPath);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUParseAndGetGPTVersion(
    PSTR   pszFilePath,
    PDWORD pdwGPTVersion
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR pszValue = NULL;
    PCFGSECTION pSectionList = NULL;

    dwError = LWParseConfigFile(pszFilePath,
                                &pSectionList,
                                FALSE);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "General",
                                            "Version",
                                            &pszValue);
    if (dwError == LWUTIL_ERROR_SUCCESS) {
        if (!pszValue) {
            dwError = LWUTIL_ERROR_NO_SUCH_ATTRIBUTE;
        } else {
            *pdwGPTVersion = atoi(pszValue);
        }
    }

cleanup:

    if (pSectionList) {
        LWFreeConfigSectionList(pSectionList);
    }

    if (pszValue) {
        LWFreeString(pszValue);
    }

    return dwError;

error:

    if (pdwGPTVersion)
        *pdwGPTVersion = 0;

    goto cleanup;
}

DWORD
ADUParseAndSetGPTVersion(
    PSTR  pszFilePath,
    DWORD dwGPTVersion
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    char szValue[256];
    PCFGSECTION pSectionList = NULL;

    memset(szValue, 0, sizeof(szValue));
    sprintf(szValue, "%d", dwGPTVersion);

    dwError = LWParseConfigFile(pszFilePath,
                                &pSectionList,
                                FALSE);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWSetConfigValueBySectionName(pSectionList,
                                            "General",
                                            "Version",
                                            szValue);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWSaveConfigSectionList(pszFilePath, pSectionList);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pSectionList) {
        LWFreeConfigSectionList(pSectionList);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUGetGPTFileAndVersionNumber(
    PSTR   pszgPCFileSysPath,
    PSTR * ppszGPTFilePath,
    PDWORD pdwGPTVersion,
    PSTR   pszPath
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR pszGPTINIFilePath = NULL;

    dwError = ADUGetGPTINIFile(pszgPCFileSysPath, &pszGPTINIFilePath, pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUParseAndGetGPTVersion(pszGPTINIFilePath, pdwGPTVersion);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *ppszGPTFilePath = pszGPTINIFilePath;
    pszGPTINIFilePath = NULL;

cleanup:

    if (pszGPTINIFilePath) {
        LWFreeString(pszGPTINIFilePath);
    }

    return dwError;

error:

    if (ppszGPTFilePath)
        *ppszGPTFilePath = NULL;

    if (pdwGPTVersion)
        *pdwGPTVersion = 0;

    goto cleanup;
}

DWORD
ADUSetGPTVersionNumber(
    PSTR  pszgPCFileSysPath,
    DWORD dwFileVersion,
    PSTR  pszPath
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    char szCachePath[PATH_MAX];
    char szGPOVersionFile[PATH_MAX];
    PSTR  pszGPTFilePath = NULL;

    memset(szCachePath, 0, sizeof(szCachePath));
    memset(szGPOVersionFile, 0, sizeof(szGPOVersionFile));

    dwError =  ADUCrackFileSysPath(pszgPCFileSysPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   NULL);
    BAIL_ON_LWUTIL_ERROR(dwError);

    strcpy(szCachePath, pszPath);

    strcpy(szGPOVersionFile, szCachePath);
    strcat(szGPOVersionFile, "/GPT.INI");

    dwError = ADUParseAndSetGPTVersion(szGPOVersionFile, dwFileVersion);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUPutGPTINIFile(szCachePath, pszgPCFileSysPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pszDomainName)
        LWFreeString(pszDomainName);

    if (pszSourcePath)
        LWFreeString(pszSourcePath);

    if (pszGPTFilePath)
        LWFreeString(pszGPTFilePath);

    return dwError;

error:

    goto cleanup;
}

VOID
ADUGetComputerAndUserVersionNumbers(
    DWORD dwVersion,
    PWORD pwUserVersion,
    PWORD pwComputerVersion
    )
{
    dwVersion = CONVERT_ENDIAN_DWORD(dwVersion);

    *pwUserVersion = (WORD)((dwVersion & 0xFFFF0000)>>16);
    *pwComputerVersion = (WORD)(dwVersion & 0x0000FFFF);
}

DWORD
ADUGetVersionFromUserAndComputer(
    WORD wUser,
    WORD wComputer
    )
{
    DWORD dwResult =
     ((((DWORD)(wUser) & 0xFFFF) << 16) | ((DWORD)(wComputer) & 0xFFFF));

    dwResult = CONVERT_ENDIAN_DWORD(dwResult);

    return dwResult;
}

DWORD
ADUGetPolicyInformation(
    HANDLE hDirectory,
    PSTR pszPolicyDN,
    PGROUP_POLICY_OBJECT pGroupPolicyObject
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR szAttributeList[] =
        {ADU_DISPLAY_NAME_ATTR,
         ADU_FLAGS_ATTR,
         ADU_FILESYS_PATH_ATTR,
         ADU_FUNCTIONALITY_VERSION_ATTR,
         ADU_MACHINE_EXTENSION_NAMES_ATTR,
         ADU_USER_EXTENSION_NAMES_ATTR,
         ADU_WQL_FILTER_ATTR,
         ADU_VERSION_NUMBER_ATTR,
         NULL
        };
    LDAPMessage* pMessage = NULL;
    DWORD dwCount = 0;
    PSTR  pszValue = NULL;
    DWORD dwValue = 0;
    PADU_DIRECTORY_CONTEXT pDirectory = (PADU_DIRECTORY_CONTEXT)hDirectory;

    dwError = ADUDirectorySearch(
        hDirectory,
        pszPolicyDN,
        LDAP_SCOPE_BASE,
        "(objectclass=*)",
        szAttributeList,
        &pMessage);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pMessage
        );

    if (dwCount < 0) {
        dwError = LWUTIL_ERROR_NO_SUCH_POLICY;
    } else if (dwCount == 0) {
        dwError = LWUTIL_ERROR_NO_SUCH_POLICY;
    } else if (dwCount > 1) {
        dwError = LWUTIL_ERROR_NO_SUCH_POLICY;
    }
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUGetLDAPString(hDirectory,
                               pMessage,
                               ADU_DISPLAY_NAME_ATTR,
                               &pszValue);
    BAIL_ON_LWUTIL_ERROR(dwError);
    pGroupPolicyObject->pszDisplayName = pszValue;
    pszValue = NULL;

    dwError = ADUGetLDAPUInt32(hDirectory,
                               pMessage,
                               ADU_FLAGS_ATTR,
                               &dwValue);
    BAIL_ON_LWUTIL_ERROR(dwError);
    pGroupPolicyObject->dwFlags = dwValue;

    dwError = ADUGetLDAPString(hDirectory,
                               pMessage,
                               ADU_FILESYS_PATH_ATTR,
                               &pszValue);
    BAIL_ON_LWUTIL_ERROR(dwError);
    pGroupPolicyObject->pszgPCFileSysPath = pszValue;
    pszValue = NULL;

    dwError = ADUGetLDAPUInt32(hDirectory,
                               pMessage,
                               ADU_FUNCTIONALITY_VERSION_ATTR,
                               &dwValue);
    BAIL_ON_LWUTIL_ERROR(dwError);
    pGroupPolicyObject->gPCFunctionalityVersion = dwValue;

    dwError = ADUGetLDAPString(hDirectory,
                               pMessage,
                               ADU_MACHINE_EXTENSION_NAMES_ATTR,
                               &pszValue);
    BAIL_ON_LWUTIL_ERROR(dwError);
    pGroupPolicyObject->pszgPCMachineExtensionNames = pszValue;
    pszValue = NULL;

    dwError = ADUGetLDAPString(hDirectory,
                               pMessage,
                               ADU_USER_EXTENSION_NAMES_ATTR,
                               &pszValue);
    BAIL_ON_LWUTIL_ERROR(dwError);
    pGroupPolicyObject->pszgPCUserExtensionNames = pszValue;
    pszValue = NULL;

    dwError = ADUGetLDAPUInt32(hDirectory,
                               pMessage,
                               ADU_VERSION_NUMBER_ATTR,
                               &dwValue);
    BAIL_ON_LWUTIL_ERROR(dwError);
    pGroupPolicyObject->dwVersion = dwValue;

cleanup:

    if (pszValue) {
        LWFreeString(pszValue);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUSetPolicyVersionInAD(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD dwVersion)
{
    DWORD dwError = 0;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    HANDLE hDirectory = (HANDLE)NULL;

    dwError =  ADUCrackFileSysPath(pGPO->pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    NULL);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUOpenDirectory(pszDomainName, &hDirectory);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = ADUPutLDAPUInt32(hDirectory, pGPO->pszPolicyDN, ADU_VERSION_NUMBER_ATTR, dwVersion);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);

    if (hDirectory)
        ADUCloseDirectory(hDirectory);

    return dwError;

error:

    goto cleanup;
}

