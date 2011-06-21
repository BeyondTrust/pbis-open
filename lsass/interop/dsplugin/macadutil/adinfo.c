/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "../includes.h"


DWORD
ADUGetAllMCXPolicies(
    HANDLE hDirectory,
    PCSTR pszDN,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR szAttributeList[] = { "distinguishedName", NULL };
    PGROUP_POLICY_OBJECT pGPObjectList = NULL;
    PGROUP_POLICY_OBJECT pGPObject = NULL;
    LDAPMessage* pMessage = NULL;
    LDAPMessage* pLDAPMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszValue = NULL;

    dwError = LwLdapDirectorySearch(
        hDirectory,
        pszDN,
        LDAP_SCOPE_ONELEVEL,
        (PSTR)"(&(objectclass=groupPolicyContainer)(|(gPCMachineExtensionNames=*{B9BF896E-F9EB-49B5-8E67-11E2EDAED06C}*)(gPCUserExtensionNames=*{07E500C4-20FD-4829-8F38-B5FF63FA0493}*)))",
        szAttributeList,
        &pMessage);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwLdapCountEntries(
        hDirectory,
        pMessage,
        &dwCount
        );
    BAIL_ON_MAC_ERROR(dwError);

    if (dwCount > 0)
    {
        pLDAPMessage = LwLdapFirstEntry(hDirectory, pMessage);
    }

    while(pLDAPMessage != NULL)
    {
        dwError = LwLdapGetString(hDirectory,
                                   pLDAPMessage,
                                   "distinguishedName",
                                   &pszValue);
        BAIL_ON_MAC_ERROR(dwError);

        dwError = LwAllocateMemory(sizeof(GROUP_POLICY_OBJECT), (PVOID*)&pGPObject);
        BAIL_ON_MAC_ERROR(dwError);

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

        pLDAPMessage = LwLdapNextEntry(hDirectory, pLDAPMessage);
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
ADUGetMCXPolicy(
    HANDLE hDirectory,
    PCSTR pszDN,
    PCSTR pszGPOName,
    PGROUP_POLICY_OBJECT * ppGPO
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR szAttributeList[] = { "distinguishedName", NULL };
    char szQuery[512] = {0};
    PGROUP_POLICY_OBJECT pGPObject = NULL;
    LDAPMessage* pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszValue = NULL;

    sprintf(szQuery, "(&(objectclass=groupPolicyContainer)(%s=%s))", ADU_DISPLAY_NAME_ATTR, pszGPOName);

    dwError = LwLdapDirectorySearch(
        hDirectory,
        pszDN,
        LDAP_SCOPE_ONELEVEL,
        szQuery,
        szAttributeList,
        &pMessage);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwLdapCountEntries(
        hDirectory,
        pMessage,
        &dwCount
        );
    BAIL_ON_MAC_ERROR(dwError);

    if (dwCount < 0) {
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
    } else if (dwCount == 0) {
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
    } else if (dwCount > 1) {
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwLdapGetString(hDirectory,
                               pMessage,
                               "distinguishedName",
                               &pszValue);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(GROUP_POLICY_OBJECT), (PVOID*)&pGPObject);
    BAIL_ON_MAC_ERROR(dwError);

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
    PSTR* ppszGPTINIFilePath
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    char szSourceFilePath[PATH_MAX + 1];
    PSTR pszDestFilePath = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszPolicyIdentifier = NULL;

    dwError = LwAllocateMemory( PATH_MAX + 1,
                                (PVOID*)&pszDestFilePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError =  ADUCrackFileSysPath(pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    sprintf( szSourceFilePath,
             "%s\\%s",
             pszSourcePath,
             "GPT.INI");

    sprintf(pszDestFilePath,
            "%s/%s_GPT.INI",
            LWDS_ADMIN_CACHE_DIR,
            pszPolicyIdentifier);

    dwError = ADUSMBGetFile(pszDomainName,
                             szSourceFilePath,
                             pszDestFilePath);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszGPTINIFilePath = pszDestFilePath;
    pszDestFilePath = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszDestFilePath);

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
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
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
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUSMBPutFile(pszDomainName,
                            pszFolderPath,
                            (PSTR)"GPT.INI",
                            pszDestPath);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszDestPath);

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
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszValue = NULL;
    PCFGSECTION pSectionList = NULL;

    dwError = LWParseConfigFile(pszFilePath,
                                &pSectionList,
                                FALSE);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "General",
                                            "Version",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS) {
        if (!pszValue) {
            dwError = MAC_AD_ERROR_NO_SUCH_ATTRIBUTE;
        } else {
            *pdwGPTVersion = atoi(pszValue);
        }
    }

cleanup:

    if (pSectionList) {
        LWFreeConfigSectionList(pSectionList);
    }

    LW_SAFE_FREE_STRING(pszValue);

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
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    char szValue[256];
    PCFGSECTION pSectionList = NULL;

    memset(szValue, 0, sizeof(szValue));
	sprintf(szValue, "%d", dwGPTVersion);

    dwError = LWParseConfigFile(pszFilePath,
                                &pSectionList,
                                FALSE);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWSetConfigValueBySectionName(pSectionList,
                                            "General",
                                            "Version",
                                            szValue);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWSaveConfigSectionList(pszFilePath, pSectionList);
    BAIL_ON_MAC_ERROR(dwError);

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
    PDWORD pdwGPTVersion
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszGPTINIFilePath = NULL;

    dwError = ADUGetGPTINIFile(pszgPCFileSysPath, &pszGPTINIFilePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUParseAndGetGPTVersion(pszGPTINIFilePath, pdwGPTVersion);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszGPTFilePath = pszGPTINIFilePath;
    pszGPTINIFilePath = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszGPTINIFilePath);

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
    DWORD dwFileVersion
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    char szCachePath[PATH_MAX];
    char szGPOVersionFile[PATH_MAX];
    PSTR  pszGPTFilePath = NULL;

    memset(szCachePath, 0, sizeof(szCachePath));
    memset(szGPOVersionFile, 0, sizeof(szGPOVersionFile));

    dwError =  ADUCrackFileSysPath(pszgPCFileSysPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    strcpy(szCachePath, LWDS_ADMIN_CACHE_DIR);
    strcat(szCachePath, "/");
    strcat(szCachePath, pszPolicyIdentifier);

    strcpy(szGPOVersionFile, szCachePath);
    strcat(szGPOVersionFile, "/GPT.INI");

    dwError = ADUParseAndSetGPTVersion(szGPOVersionFile, dwFileVersion);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUPutGPTINIFile(szCachePath, pszgPCFileSysPath);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszGPTFilePath);

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
    PCSTR pszPolicyDN,
    PGROUP_POLICY_OBJECT pGroupPolicyObject
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
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

    dwError = LwLdapDirectorySearch(
        hDirectory,
        pszPolicyDN,
        LDAP_SCOPE_BASE,
        (PSTR)"(objectclass=*)",
        szAttributeList,
        &pMessage);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwLdapCountEntries(
        hDirectory,
        pMessage,
        &dwCount
        );
    BAIL_ON_MAC_ERROR(dwError);

    if (dwCount < 0) {
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
    } else if (dwCount == 0) {
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
    } else if (dwCount > 1) {
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
    }
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwLdapGetString(hDirectory,
                               pMessage,
                               ADU_DISPLAY_NAME_ATTR,
                               &pszValue);
    BAIL_ON_MAC_ERROR(dwError);
    pGroupPolicyObject->pszDisplayName = pszValue;
    pszValue = NULL;

    dwError = LwLdapGetUInt32(hDirectory,
                               pMessage,
                               ADU_FLAGS_ATTR,
                               &dwValue);
    BAIL_ON_MAC_ERROR(dwError);
    pGroupPolicyObject->dwFlags = dwValue;

    dwError = LwLdapGetString(hDirectory,
                               pMessage,
                               ADU_FILESYS_PATH_ATTR,
                               &pszValue);
    BAIL_ON_MAC_ERROR(dwError);
    pGroupPolicyObject->pszgPCFileSysPath = pszValue;
    pszValue = NULL;

    dwError = LwLdapGetUInt32(hDirectory,
                               pMessage,
                               ADU_FUNCTIONALITY_VERSION_ATTR,
                               &dwValue);
    BAIL_ON_MAC_ERROR(dwError);
    pGroupPolicyObject->gPCFunctionalityVersion = dwValue;

    dwError = LwLdapGetString(hDirectory,
                               pMessage,
                               ADU_MACHINE_EXTENSION_NAMES_ATTR,
                               &pszValue);
    if (dwError == LW_ERROR_INVALID_LDAP_ATTR_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_MAC_ERROR(dwError);
    pGroupPolicyObject->pszgPCMachineExtensionNames = pszValue;
    pszValue = NULL;

    dwError = LwLdapGetString(hDirectory,
                               pMessage,
                               ADU_USER_EXTENSION_NAMES_ATTR,
                               &pszValue);
    if (dwError == LW_ERROR_INVALID_LDAP_ATTR_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_MAC_ERROR(dwError);
    pGroupPolicyObject->pszgPCUserExtensionNames = pszValue;
    pszValue = NULL;

    dwError = LwLdapGetUInt32(hDirectory,
                               pMessage,
                               ADU_VERSION_NUMBER_ATTR,
                               &dwValue);
    BAIL_ON_MAC_ERROR(dwError);
    pGroupPolicyObject->dwVersion = dwValue;

cleanup:

    LW_SAFE_FREE_STRING(pszValue);

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    LOG_ERROR("Failed to find policy or read GPO attributes for policy (%s)", pszPolicyDN);

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
    PSTR pszPolicyIdentifier = NULL;
    HANDLE hDirectory = (HANDLE)NULL;

    dwError =  ADUCrackFileSysPath(pGPO->pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUOpenLwLdapDirectory(pszDomainName, &hDirectory);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwLdapPutUInt32(hDirectory, pGPO->pszPolicyDN, ADU_VERSION_NUMBER_ATTR, dwVersion);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    if (hDirectory)
        LwLdapCloseDirectory(hDirectory);

    return dwError;

error:

    goto cleanup;
}

