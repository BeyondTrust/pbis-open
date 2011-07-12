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
#include "cfgparser.h"
#include <lsa/ad.h>

// Globals
//pthread_mutex_t              g_ADULock = PTHREAD_MUTEX_INITIALIZER;
//time_t                       gdwKrbTicketExpiryTime = 0;
//const double                 gdwExpiryGraceSeconds = (60 * 60);

static
DWORD
LsaAccessFreeData(
    PVOID pAccessData
    );

typedef struct {
    DWORD   dwUidCount;
    uid_t * pUids;
    DWORD   dwGidCount;
    gid_t * pGids;
} LSA_ACCESS_DATA, *PLSA_ACCESS_DATA;

static
DWORD
LsaAccessGetData(
    PCSTR * pczConfigData,
    PVOID * ppAccessData
    )
{
    DWORD            dwError = 0;
    PLSA_ACCESS_DATA pAccessData = NULL;
    DWORD            dwAllocUid = 0;
    DWORD            dwAllocGid = 0;
    DWORD            dwCount = 0;
    HANDLE           hLsaConnection = (HANDLE)NULL;
    DWORD            dwInfoLevel = 0;
    PVOID            pUserInfo = NULL;
    PVOID            pGroupInfo = NULL;

    if ( pczConfigData == NULL )
    {
        *ppAccessData = NULL;
        goto cleanup;
    }

    dwError = LwAllocateMemory(sizeof(LSA_ACCESS_DATA),
                  (PVOID*)&pAccessData);
    BAIL_ON_MAC_ERROR(dwError);

    dwAllocUid = 8;
    dwError = LwAllocateMemory(sizeof(uid_t) * dwAllocUid,
                  (PVOID*)&pAccessData->pUids);
    BAIL_ON_MAC_ERROR(dwError);

    dwAllocGid = 16;
    dwError = LwAllocateMemory(sizeof(uid_t) * dwAllocGid,
                  (PVOID*)&pAccessData->pGids);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    for ( dwCount = 0 ; pczConfigData[dwCount] ; dwCount++ )
    {
        dwError = LsaFindGroupByName(
                      hLsaConnection,
                      pczConfigData[dwCount],
                      0,
                      dwInfoLevel,
                      &pGroupInfo);
        if ( !dwError )
        {
            if ( pAccessData->dwGidCount == dwAllocGid )
            {
                dwAllocGid *= 2;
                dwError = LwReallocMemory(
                              (PVOID)pAccessData->pGids,
                              (PVOID *)&pAccessData->pGids,
                              dwAllocGid * sizeof(gid_t) );
                BAIL_ON_MAC_ERROR(dwError);
            }

            pAccessData->pGids[pAccessData->dwGidCount++] =
                ((PLSA_GROUP_INFO_0)pGroupInfo)->gid;

            LsaFreeGroupInfo(
                dwInfoLevel,
                pGroupInfo);
            pGroupInfo = NULL;
        }
        else
        {
            dwError = LsaFindUserByName(
                          hLsaConnection,
                          pczConfigData[dwCount],
                          dwInfoLevel,
                          &pUserInfo);
            if ( dwError )
            {
                continue;
            }
            if ( pAccessData->dwUidCount == dwAllocUid )
            {
                dwAllocUid *= 2;
                dwError = LwReallocMemory(
                              (PVOID)pAccessData->pUids,
                              (PVOID *)&pAccessData->pUids,
                              dwAllocUid * sizeof(uid_t) );
                BAIL_ON_MAC_ERROR(dwError);
            }

            pAccessData->pUids[pAccessData->dwUidCount++] =
                ((PLSA_USER_INFO_0)pUserInfo)->uid;

            LsaFreeUserInfo(
                dwInfoLevel,
                pUserInfo);
            pUserInfo = NULL;
        }
    }

    *ppAccessData = pAccessData;

cleanup:
    if ( pUserInfo )
    {
        LsaFreeUserInfo(
            dwInfoLevel,
            pUserInfo);
    }
    if ( pGroupInfo )
    {
        LsaFreeGroupInfo(
            dwInfoLevel,
            pGroupInfo);
    }
    if ( hLsaConnection != (HANDLE)NULL )
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:
    if ( pAccessData )
        LsaAccessFreeData( (PVOID)pAccessData );

    goto cleanup;
}

static
DWORD
LsaAccessCheckData(
    PCSTR pczUserName,
    PCVOID pAccessData
    )
{
    DWORD dwError = 0;
    PLSA_ACCESS_DATA pAccessDataLocal = NULL;
    HANDLE           hLsaConnection = (HANDLE)NULL;
    DWORD            dwInfoLevel = 0;
    PVOID            pUserInfo = NULL;
    gid_t *          pGid = NULL;
    DWORD            dwNumGroups = 0;
    DWORD            dwCount = 0;
    DWORD            dwCount2 = 0;
    BOOLEAN          bUserIsOK = FALSE;

    if ( !pAccessData )
    {
        dwError = LW_ERROR_AUTH_ERROR;
    }
    BAIL_ON_MAC_ERROR(dwError);

    pAccessDataLocal = (PLSA_ACCESS_DATA)pAccessData;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindUserByName(
                  hLsaConnection,
                  pczUserName,
                  dwInfoLevel,
                  &pUserInfo);
    BAIL_ON_MAC_ERROR(dwError);
    
    for ( dwCount = 0 ; dwCount < pAccessDataLocal->dwUidCount ; dwCount++ )
    {
        if ( ((PLSA_USER_INFO_0)pUserInfo)->uid == pAccessDataLocal->pUids[dwCount] )
        {
            bUserIsOK = TRUE;
            break;
        }
    }

    if ( !bUserIsOK )
    {
        dwError = LsaGetGidsForUserByName(
                      hLsaConnection,
                      pczUserName,
                      &dwNumGroups,
                      &pGid);
        BAIL_ON_MAC_ERROR(dwError);

        for ( dwCount = 0 ; (dwCount < dwNumGroups) && !bUserIsOK ; dwCount++ )
        {
            for ( dwCount2 = 0 ; dwCount2 < pAccessDataLocal->dwGidCount ; dwCount2++ )
            {
                if ( pAccessDataLocal->pGids[dwCount2] == pGid[dwCount] )
                {
                    bUserIsOK = TRUE;
                    break;
                }
            }
        }
    }

    if ( !bUserIsOK )
    {
        dwError = LW_ERROR_AUTH_ERROR;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pGid);

    if ( pUserInfo )
    {
        LsaFreeUserInfo(
            dwInfoLevel,
            pUserInfo);
    }
    if ( hLsaConnection != (HANDLE)NULL )
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LsaAccessFreeData(
    PVOID pAccessData
    )
{
    DWORD dwError = 0;

    if ( pAccessData )
    {
        LW_SAFE_FREE_MEMORY(((PLSA_ACCESS_DATA)pAccessData)->pUids);
        LW_SAFE_FREE_MEMORY(((PLSA_ACCESS_DATA)pAccessData)->pGids);
        LW_SAFE_FREE_MEMORY(pAccessData);
    }

    return dwError;
}

LONG
GetCurrentDomain(
    OUT PSTR* ppszDnsDomainName
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    HANDLE hLsaConnection = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaAdGetMachineAccountInfo(
                    hLsaConnection,
                    NULL,
                    &pAccountInfo);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateString(pAccountInfo->DnsDomainName, &pszDnsDomainName);
    BAIL_ON_MAC_ERROR(dwError);

error:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszDnsDomainName);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pAccountInfo)
    {
        LsaAdFreeMachineAccountInfo(pAccountInfo);
    }

    *ppszDnsDomainName = pszDnsDomainName;

    return LWGetMacError(dwError);
}

LONG
EnumWorkgroupManagerEnabledGPOs(
    PCSTR                 pszDomainName,
    PGROUP_POLICY_OBJECT* ppMCXGPOs
    )
{
    DWORD dwError = 0;
    PSTR  pszDomainDN = NULL;
    PSTR  pszSearchDN = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PGROUP_POLICY_OBJECT pGroupPolicyObjects = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    BOOLEAN bDeactivateCredContext = FALSE;
    PSTR    pszOrigCachePath = NULL;

    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = EINVAL;
        BAIL_ON_MAC_ERROR(dwError);
    }

    /* Set default credentials to the machine's */
    dwError = ADUInitKrb5(pszDomainName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    NULL,
                    TRUE,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = ADUOpenLwLdapDirectory(pszDomainName, &hDirectory);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUConvertDomainToDN(pszDomainName, &pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszSearchDN,
                    "CN=Policies,CN=System,%s",
                    pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetAllMCXGPOList(
                    hDirectory,
                    pszSearchDN,
                    &pGroupPolicyObjects);
    BAIL_ON_MAC_ERROR(dwError);

    *ppMCXGPOs = pGroupPolicyObjects;
    pGroupPolicyObjects = NULL;

cleanup:

    if (hDirectory != (HANDLE)NULL)
    {
        LwLdapCloseDirectory(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszDomainDN);
    LW_SAFE_FREE_STRING(pszSearchDN);

    ADU_SAFE_FREE_GPO_LIST(pGroupPolicyObjects);

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

    *ppMCXGPOs = NULL;

    goto cleanup;
}

LONG
GetSpecificGPO(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR    pszOrigCachePath = NULL;
    BOOLEAN bDeactivateCredContext = FALSE;

    /* pszDomainName can be NULL sometimes, here we will default to our
       configured joined domain */
    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = GetCurrentDomain(&pszDomain);
        BAIL_ON_MAC_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateString(pszDomainName, &pszDomain);
        BAIL_ON_MAC_ERROR(dwError);
    }

    /* Set default credentials to the machine's */
    dwError = ADUInitKrb5(pszDomain);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomain,
                    NULL,
                    TRUE,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = GetSpecificGPO_authenticated(
                    pszDomain,
                    pszGPOName,
                    &pGPO);
    BAIL_ON_MAC_ERROR(dwError);

    *ppGPO = pGPO;

cleanup:

    LW_SAFE_FREE_STRING(pszDomain);

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

    *ppGPO = NULL;

    ADU_SAFE_FREE_GPO_LIST(pGPO);

    goto cleanup;
}

LONG
GetSpecificGPO_authenticated(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    )
{
    DWORD dwError = 0;
    PSTR pszDomainDN = NULL;
    PSTR pszSearchDN = NULL;
    PSTR  pszDomainLocal = NULL;
    PCSTR pszDomain = pszDomainName;
    HANDLE hDirectory = (HANDLE)NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    if (IsNullOrEmptyString(pszGPOName))
    {
        dwError = EINVAL;
        BAIL_ON_MAC_ERROR(dwError);
    }

    /* pszDomainName can be NULL sometimes, here we will default to our
       configured joined domain */
    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = GetCurrentDomain(&pszDomainLocal);
        BAIL_ON_MAC_ERROR(dwError);

        pszDomain = pszDomainLocal;
    }

    dwError = ADUOpenLwLdapDirectory(pszDomain, &hDirectory);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUConvertDomainToDN(pszDomain, &pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszSearchDN,
                    "CN=Policies,CN=System,%s",
                    pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetMCXGPO(
                    hDirectory,
                    pszSearchDN,
                    pszGPOName,
                    &pGPO);
    BAIL_ON_MAC_ERROR(dwError);

    if (!pGPO)
    {
        dwError = MAC_AD_ERROR_INVALID_NAME;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *ppGPO = pGPO;

cleanup:

    if (hDirectory != (HANDLE)NULL)
    {
        LwLdapCloseDirectory(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszDomainDN);
    LW_SAFE_FREE_STRING(pszSearchDN);
    LW_SAFE_FREE_STRING(pszDomainLocal);

    return LWGetMacError(dwError);

error:

    *ppGPO = NULL;

    ADU_SAFE_FREE_GPO_LIST(pGPO);

    goto cleanup;
}

BOOLEAN
IsMCXSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType
    )
{
    DWORD dwError = 0;
    PGROUP_POLICY_OBJECT pMatchedPolicy = NULL;
    BOOLEAN bEnabled = FALSE;

    switch(dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:
            dwError = ADUComputeCSEList(
                            MACHINE_GROUP_POLICY,
                            COMPUTER_MCX_CSE_GUID,
                            pGPO,
                            &pMatchedPolicy);
            BAIL_ON_MAC_ERROR(dwError);
            break;

        case USER_GROUP_POLICY:
            dwError = ADUComputeCSEList(
                            USER_GROUP_POLICY,
                            USER_MCX_CSE_GUID,
                            pGPO,
                            &pMatchedPolicy);
            BAIL_ON_MAC_ERROR(dwError);
            break;

        default:

            dwError = MAC_AD_ERROR_INVALID_PARAMETER;
            BAIL_ON_MAC_ERROR(dwError);
    }

    bEnabled = (pMatchedPolicy != NULL);
    LOG("ADUAdapter_IsMCXSettingEnabledForGPO(type %s): %s",
        dwPolicyType == MACHINE_GROUP_POLICY ? "Computer" : "User",
        bEnabled ? "yes" : "no");

cleanup:

    ADU_SAFE_FREE_GPO_LIST(pMatchedPolicy);

    return bEnabled;

error:

    goto cleanup;
}

LONG
ConvertMCXSettingsToMCXValues(
    PCSTR pszPolicyPath,
    DWORD dwPolicyType,
    PMCXVALUE * ppMCXValueList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PMCXVALUE pMCXValues = NULL;
    PMCXVALUE pNew = NULL;
    PMCXVALUE pPrev = NULL;
    PSTR pszMCXFile = NULL;
    char szMCXValueFile[PATH_MAX];
    int iIter = 0;

    if (!ppMCXValueList)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = GetFileNameForMCXSettings(dwPolicyType, &pszMCXFile);
    BAIL_ON_MAC_ERROR(dwError);

    while (TRUE)
    {
        memset(szMCXValueFile, 0, sizeof(szMCXValueFile));
        sprintf(szMCXValueFile, "%s/%s-%d.mcx", pszPolicyPath, pszMCXFile, iIter+1);

        dwError = ReadMCXValueFromFile(szMCXValueFile, &pNew);
        if (dwError == MAC_AD_ERROR_INVALID_NAME)
        {
            dwError = MAC_AD_ERROR_SUCCESS;
            break;
        }
        BAIL_ON_MAC_ERROR(dwError);

        if (pPrev)
        {
            pPrev->pNext = pNew;
        }
        else
        {
            pMCXValues = pNew;
        }

        pPrev = pNew;
        pNew = NULL;

        iIter++;
    }

    if (iIter && pMCXValues)
    {
        *ppMCXValueList = pMCXValues;
        pMCXValues = NULL;
    }
    else
    {
        *ppMCXValueList = NULL;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszMCXFile);

    if (pMCXValues)
        FreeMCXValueList(pMCXValues);

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
ConvertDSAttributeValuesToMCXValues(
    PDSATTRIBUTEVALUE    pValues,
    PMCXVALUE *          ppMCXValues
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PDSATTRIBUTEVALUE pAttrValue = NULL;
    PMCXVALUE pNew = NULL;
    PMCXVALUE pPrev = NULL;
    PMCXVALUE pValueList = NULL;

    if (!ppMCXValues)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    pAttrValue = pValues;
    while (pAttrValue)
    {
        dwError = LwAllocateMemory(sizeof(MCXVALUE), (PVOID*) &pNew);
        BAIL_ON_MAC_ERROR(dwError);

        pNew->iValLen = pAttrValue->valLen;

        dwError = LwAllocateMemory(pNew->iValLen, (PVOID*)&pNew->pValueData);
        BAIL_ON_MAC_ERROR(dwError);

        memcpy(pNew->pValueData, pAttrValue->pszValue, pNew->iValLen);

        if (pPrev)
        {
            pPrev->pNext = pNew;
        }
        else
        {
            pValueList = pNew;
        }

        pPrev = pNew;
        pNew = NULL;

        pAttrValue = pAttrValue->pNext;
    }

    *ppMCXValues = pValueList;

cleanup:

    return LWGetMacError(dwError);

error:

    if (ppMCXValues)
    {
        *ppMCXValues = NULL;
    }

    if (pValueList)
    {
        FreeMCXValueList(pValueList);
    }

    goto cleanup;
}

static
DWORD
RemoveFiles(
    PSTR pszPath,
    BOOLEAN fDirectory,
    BOOLEAN fRecursive
    )
{
    DWORD dwError = 0;
    CHAR  szCommand[2 * PATH_MAX + 1];

    sprintf(szCommand, "/bin/rm -f %s %s %s",
            fDirectory ? "-d" : "",
            fRecursive ? "-r" : "",
            pszPath);

    if (system(szCommand) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

LONG
SaveMCXValuesForGPOSettingType(
    PMCXVALUE            pMCXValueList,
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PCSTR                pszUserUPN
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
    BOOLEAN bCurrent = FALSE;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR    pszOrigCachePath = NULL;
    BOOLEAN bDeactivateCredContext = FALSE;

    LOG("Saving %s MCX Settings for GPO (%s)",
        dwPolicyType == MACHINE_GROUP_POLICY ? "machine" :
        dwPolicyType == USER_GROUP_POLICY ? "user" :
        "unknown",
        pGPO->pszDisplayName);

    if (!pGPO)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    memset(szCseGuid, 0, sizeof(szCseGuid));

    /* Set default credentials to the machine's */
    dwError = GetCurrentDomain(&pszDomainName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    NULL,
                    TRUE,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = IsCacheDataCurrentForGPO(
                    pGPO,
                    &dwVersion,
                    &dwFileVersion,
                    &bCurrent);
    BAIL_ON_MAC_ERROR(dwError);

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

            strcpy(szCseGuid, (PSTR) COMPUTER_MCX_CSE_GUID);

            dwError = GetCachedPolicyFiles(
                            MACHINE_GROUP_POLICY,
                            pGPO->pszgPCFileSysPath,
                            szCseGuid,
                            NULL,
                            &pszSourceFolder,
                            &bPolicyExists);
            BAIL_ON_MAC_ERROR(dwError);

            wComputerVersion++;

            break;

        case USER_GROUP_POLICY:

            strcpy(szCseGuid, (PSTR) USER_MCX_CSE_GUID);

            dwError = GetCachedPolicyFiles(
                            USER_GROUP_POLICY,
                            pGPO->pszgPCFileSysPath,
                            szCseGuid,
                            NULL,
                            &pszSourceFolder,
                            &bPolicyExists);
            BAIL_ON_MAC_ERROR(dwError);

            wUserVersion++;

            break;

        default:

            dwError = MAC_AD_ERROR_INVALID_PARAMETER;
            BAIL_ON_MAC_ERROR(dwError);
    }

    if (bPolicyExists)
    {
        dwError = LwAllocateStringPrintf(
                        &pszAllMCXFiles,
                        "%s/%s",
                        pszSourceFolder,
                        "*.mcx");
        BAIL_ON_MAC_ERROR(dwError);

        dwError = RemoveFiles(pszAllMCXFiles, FALSE, FALSE);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = ConvertMCXValuesToMCXSettings(
                    pMCXValueList,
                    pszSourceFolder,
                    dwPolicyType);
    BAIL_ON_MAC_ERROR(dwError);

    dwVersion = ADUGetVersionFromUserAndComputer(wUserVersion, wComputerVersion);
    BAIL_ON_MAC_ERROR(dwError);

    /* Change thread to user's creds */
    dwError = ADUDeactivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = FALSE;

    ADUFreeCredContext(pCredContext);
    pCredContext = NULL;

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    pszUserUPN,
                    TRUE,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            LOG_ERROR("Failed to revert kerberos cache path [code:%d]", dwError2);
        }

        LwFreeMemory(pszOrigCachePath);
        pszOrigCachePath = NULL;
    }

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = ADUPutPolicyFiles(
                    NULL,
                    TRUE /* replace destination */,
                    dwPolicyType,
                    pGPO->pszgPCFileSysPath,
                    szCseGuid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUSetGPTVersionNumber(pGPO->pszgPCFileSysPath, dwVersion);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUSetPolicyVersionInAD(pGPO, dwVersion);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszSourceFolder);
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszAllMCXFiles);

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
            LOG_ERROR("Failed to revert kerberos cache path [code:%d]",dwError2);
        }

        LwFreeMemory(pszOrigCachePath);
    }

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
GetMCXValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PMCXVALUE *          ppMCXValueList
    )
{
    DWORD     dwError = 0;
    PSTR      pszMachinePolicyPath = NULL;
    PSTR      pszUserPolicyPath = NULL;
    PSTR      pszDomainName = NULL;
    BOOLEAN   bMachinePolicyExists = FALSE;
    BOOLEAN   bUserPolicyExists = FALSE;
    PMCXVALUE pMCXValues = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR      pszOrigCachePath = NULL;
    BOOLEAN   bDeactivateCredContext = FALSE;

    if (!ppMCXValueList)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    /* Set default credentials to the machine's */
    dwError = GetCurrentDomain(&pszDomainName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    NULL,
                    TRUE,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = GetCurrentMCXSettingsForGPO(pGPO,
                                          &bMachinePolicyExists,
                                          &bUserPolicyExists,
                                          &pszMachinePolicyPath,
                                          &pszUserPolicyPath);
    BAIL_ON_MAC_ERROR(dwError);

    switch (dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:

            if (bMachinePolicyExists && pszMachinePolicyPath)
            {
                dwError = ConvertMCXSettingsToMCXValues(
                                pszMachinePolicyPath,
                                MACHINE_GROUP_POLICY,
                                &pMCXValues);
                BAIL_ON_MAC_ERROR(dwError);
            }

            break;

       case USER_GROUP_POLICY:

           if (bUserPolicyExists && pszUserPolicyPath )
           {
               dwError = ConvertMCXSettingsToMCXValues(
                               pszUserPolicyPath,
                               USER_GROUP_POLICY,
                               &pMCXValues);
               BAIL_ON_MAC_ERROR(dwError);
           }

           break;

       default:

           dwError = MAC_AD_ERROR_INVALID_PARAMETER;
           BAIL_ON_MAC_ERROR(dwError);
    }

    *ppMCXValueList = pMCXValues;

cleanup:

    LW_SAFE_FREE_STRING(pszMachinePolicyPath);
    LW_SAFE_FREE_STRING(pszUserPolicyPath);
    LW_SAFE_FREE_STRING(pszDomainName);

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

    if (ppMCXValueList)
    {
        *ppMCXValueList = NULL;
    }

    goto cleanup;
}

LONG
LookupComputerGroupGPO(
    PCSTR pszName,
    PSTR* ppszGPOGUID
    )
{
    DWORD dwError = 0;
    FILE * fp = NULL;
    PSTR pszGPOGUID = NULL;
    char szGPOName[256];
    char szGPOGUID[256];

    if (!ppszGPOGUID)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    fp = fopen("/var/lib/likewise/grouppolicy/mcx/computer/.lwe-computer-mcx", "r");
    if (!fp)
    {
        LOG_ERROR("LookupComputerGroupsGPO(%s) failed to find file with list of computer GPOs",
                  pszName ? pszName : "<null>");
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while (1)
    {
        if ( NULL == fgets( szGPOName,
                            sizeof(szGPOName),
                            fp) )
        {
            if (feof(fp))
            {
                break;
            }

            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        LwStripWhitespace(szGPOName, TRUE, TRUE);

        if ( NULL == fgets( szGPOGUID,
                            sizeof(szGPOGUID),
                            fp) )
        {
            if (feof(fp)) {
                break;
            }

            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        LwStripWhitespace(szGPOGUID, TRUE, TRUE);

        if (!strcmp(szGPOName, pszName))
        {
            LOG("LookupComputerGroupGPO(%s) found group (Name: %s GUID: %s)",
                pszName ? pszName : "<null>",
                szGPOName,
                szGPOGUID);

            dwError = LwAllocateString(szGPOGUID, &pszGPOGUID);
            BAIL_ON_MAC_ERROR(dwError);

            break;
        }
    }

    if (!pszGPOGUID)
    {
        LOG("LookupComputerGroupGPO function did not find GPO, returning record not found");
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *ppszGPOGUID = pszGPOGUID;

cleanup:

    if (fp)
    {
        fclose(fp);
    }

    return LWGetMacError(dwError);

error:

    if (ppszGPOGUID)
    {
        *ppszGPOGUID = NULL;
    }

    LW_SAFE_FREE_STRING(pszGPOGUID);

    goto cleanup;
}

LONG
LookupComputerListGPO(
    PCSTR  pszName,
    PSTR * ppszGPOGUID
    )
{
    DWORD dwError = 0;

    dwError = LookupComputerGroupGPO(
                    pszName,
                    ppszGPOGUID);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return LWGetMacError(dwError);

error:

    *ppszGPOGUID = NULL;

    goto cleanup;
}

LONG
AuthenticateUser(
    PCSTR pszUsername,
    PCSTR pszPassword,
    BOOLEAN fAuthOnly,
    PBOOLEAN pIsOnlineLogon,
    PSTR * ppszMessage
    )
{
    DWORD dwError = 0;
    HANDLE hLsaServer = (HANDLE)NULL;
    LSA_AUTH_USER_PAM_PARAMS params = {0};
    PLSA_AUTH_USER_PAM_INFO pInfo = NULL;
    PSTR pszMessage = NULL;

    *pIsOnlineLogon = TRUE;
    *ppszMessage = NULL;

    params.dwFlags = LSA_AUTH_USER_PAM_FLAG_RETURN_MESSAGE;
    params.pszLoginName = pszUsername;
    params.pszPassword = pszPassword;

    dwError = LsaOpenServer(&hLsaServer);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaAuthenticateUserPam(hLsaServer, &params, &pInfo);
    BAIL_ON_MAC_ERROR(dwError);

    if (pInfo && pInfo->pszMessage)
    {
        dwError = LwAllocateString(pInfo->pszMessage, &pszMessage);
        BAIL_ON_MAC_ERROR(dwError);

        *ppszMessage = pszMessage;
        pszMessage = NULL;
    }

    if (pInfo && !pInfo->bOnlineLogon)
    {
        *pIsOnlineLogon = FALSE;
    }

    dwError = LsaCheckUserInList(hLsaServer,
                                 pszUsername,
                                 NULL);
    BAIL_ON_MAC_ERROR(dwError);

    if (fAuthOnly == FALSE)
    {
        dwError = LsaOpenSession(hLsaServer,
                                 pszUsername);
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    if (hLsaServer != (HANDLE)NULL) {
       LsaCloseServer(hLsaServer);
    }

    LW_SAFE_FREE_STRING(pszMessage);

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
ChangePassword(
    PCSTR pszUsername,
    PCSTR pszOldPassword,
    PCSTR pszNewPassword
    )
{
    DWORD dwError = 0;
    HANDLE hLsaServer = (HANDLE)NULL;

    dwError = LsaOpenServer(&hLsaServer);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaChangePassword(
                  hLsaServer,
                  pszUsername,
                  pszNewPassword,
                  pszOldPassword);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (hLsaServer != (HANDLE)NULL) {
       LsaCloseServer(hLsaServer);
    }

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
GetUserPrincipalNames(
    PCSTR pszUserName,
    PSTR * ppszUserPrincipalName,
    PSTR * ppszUserSamAccount,
    PSTR * ppszUserDomainFQDN
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;
    PSTR pszUPN = NULL;
    PLSA_SID_INFO pSIDInfoList = NULL;
    PLSASTATUS pLsaStatus = NULL;
    PSTR pszUserSamAccount = NULL;
    PSTR pszUserDomain = NULL;
    DWORD i = 0, j = 0;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindUserByName(hLsaConnection,
                                pszUserName,
                                dwUserInfoLevel,
                                &pUserInfo);
    BAIL_ON_MAC_ERROR(dwError);

    if (((PLSA_USER_INFO_1)pUserInfo)->pszUPN)
    {
        LOG("Got UPN (%s) for user: %s", ((PLSA_USER_INFO_1)pUserInfo)->pszUPN,
            pszUserName ? pszUserName : "<null>");
    }
    else
    {
        dwError = MAC_AD_ERROR_UPN_NOT_FOUND;
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (((PLSA_USER_INFO_1)pUserInfo)->pszSid)
    {
        LOG("Got SID (%s) for user: %s", ((PLSA_USER_INFO_1)pUserInfo)->pszSid,
            pszUserName ? pszUserName : "<null>");
    }
    else
    {
        dwError = MAC_AD_ERROR_UPN_NOT_FOUND;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LsaGetNamesBySidList(hLsaConnection,
                                   1,
                                   &((PLSA_USER_INFO_1)pUserInfo)->pszSid,
                                   &pSIDInfoList,
                                   NULL);
    BAIL_ON_MAC_ERROR(dwError);

    if (pSIDInfoList[0].accountType != AccountType_User)
    {
        LOG("Could not get names for SID (%s) of user: %s, authentication subsystem maybe offline",
            ((PLSA_USER_INFO_1)pUserInfo)->pszSid,
            pszUserName ? pszUserName : "<null>");
        dwError = MAC_AD_ERROR_UPN_NOT_FOUND;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LsaGetStatus(hLsaConnection,
                           &pLsaStatus);
    BAIL_ON_MAC_ERROR(dwError);

    if (pSIDInfoList[0].pszDomainName != NULL)
    {
        for (i = 0; i < pLsaStatus->dwCount; i++)
        {
            if (!strcmp(pLsaStatus->pAuthProviderStatusList[i].pszId, "lsa-activedirectory-provider"))
            {
                for (j = 0; j < pLsaStatus->pAuthProviderStatusList[i].dwNumTrustedDomains; j++)
                {
                    if (pLsaStatus->pAuthProviderStatusList[i].pTrustedDomainInfoArray[j].pszNetbiosDomain != NULL &&
                        !strcmp(pLsaStatus->pAuthProviderStatusList[i].pTrustedDomainInfoArray[j].pszNetbiosDomain,
                                pSIDInfoList[0].pszDomainName))
                    {
                        LwAllocateString(pLsaStatus->pAuthProviderStatusList[i].pTrustedDomainInfoArray[j].pszDnsDomain,
                                         &pszUserDomain);
                        BAIL_ON_MAC_ERROR(dwError);
                        break;
                    }
                }
            }
        }
    }

    if (pszUserDomain)
    {
        LOG("Got domain (%s) for user: %s",
            pszUserDomain,
            pszUserName ? pszUserName : "<null>");
    }

    LwAllocateString(pSIDInfoList->pszSamAccountName, &pszUserSamAccount);
    BAIL_ON_MAC_ERROR(dwError);

    LwAllocateString(((PLSA_USER_INFO_1)pUserInfo)->pszUPN, &pszUPN);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszUserPrincipalName = pszUPN;
    pszUPN = NULL;
    *ppszUserSamAccount = pszUserSamAccount;
    pszUserSamAccount = NULL;
    *ppszUserDomainFQDN = pszUserDomain;
    pszUserDomain = NULL;

cleanup:

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

    if (pSIDInfoList)
    {
        LsaFreeSIDInfoList(pSIDInfoList, 1);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszUPN);
    LW_SAFE_FREE_STRING(pszUserSamAccount);
    LW_SAFE_FREE_STRING(pszUserDomain);

    return LWGetMacError(dwError);

error:

    LOG("Failed to get UPN for user (%s) with error: %d",
        pszUserName ? pszUserName : "<null>",
        dwError);

    goto cleanup;
}

LONG
GetUserAccountPolicy(
    PCSTR pszUserName,
    PDWORD pdwDaysToPasswordExpiry,
    PBOOLEAN pbDisabled,
    PBOOLEAN pbExpired,
    PBOOLEAN pbLocked,
    PBOOLEAN pbPasswordNeverExpires,
    PBOOLEAN pbPasswordExpired,
    PBOOLEAN pbPromptForPasswordChange,
    PBOOLEAN pbUserCanChangePassword,
    PBOOLEAN pbLogonRestriction
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 2;
    DWORD dwDaysToPasswordExpiry = 0;
    BOOLEAN bDisabled = FALSE;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bPasswordNeverExpires = FALSE;
    BOOLEAN bPasswordExpired = FALSE;
    BOOLEAN bPromptForPasswordChange = FALSE;
    BOOLEAN bUserCanChangePassword = FALSE;
    BOOLEAN bLogonRestriction = FALSE;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindUserByName(hLsaConnection,
                                pszUserName,
                                dwUserInfoLevel,
                                &pUserInfo);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaCheckUserInList(hLsaConnection,
                                 ((PLSA_USER_INFO_0)pUserInfo)->pszName,
                                 NULL);
    if (dwError)
    {
        bLogonRestriction = TRUE;
        dwError = 0;
    }

    dwDaysToPasswordExpiry = ((PLSA_USER_INFO_2)pUserInfo)->dwDaysToPasswordExpiry;
    bDisabled = ((PLSA_USER_INFO_2)pUserInfo)->bAccountDisabled;
    bExpired = ((PLSA_USER_INFO_2)pUserInfo)->bAccountExpired;
    bLocked = ((PLSA_USER_INFO_2)pUserInfo)->bAccountLocked;
    bPasswordNeverExpires = ((PLSA_USER_INFO_2)pUserInfo)->bPasswordNeverExpires;
    bPasswordExpired = ((PLSA_USER_INFO_2)pUserInfo)->bPasswordExpired;
    bPromptForPasswordChange = ((PLSA_USER_INFO_2)pUserInfo)->bPromptPasswordChange;
    bUserCanChangePassword = ((PLSA_USER_INFO_2)pUserInfo)->bUserCanChangePassword;

    *pdwDaysToPasswordExpiry = dwDaysToPasswordExpiry;
    *pbDisabled = bDisabled;
    *pbExpired = bExpired;
    *pbLocked = bLocked;
    *pbPasswordNeverExpires = bPasswordNeverExpires;
    *pbPasswordExpired = bPasswordExpired;
    *pbPromptForPasswordChange = bPromptForPasswordChange;
    *pbUserCanChangePassword = bUserCanChangePassword;
    *pbLogonRestriction = bLogonRestriction;

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return LWGetMacError(dwError);

error:

    LOG("Failed to get account policy for user (%s) with error: %d",
        pszUserName ? pszUserName : "<null>",
        dwError);

    goto cleanup;
}

void
GetLsaStatus(
    PBOOLEAN pbIsStarted
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;
    BOOLEAN IsStarted = FALSE;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaGetStatus(hLsaConnection,
                           &pLsaStatus);
    BAIL_ON_MAC_ERROR(dwError);

    IsStarted = TRUE;

cleanup:

    *pbIsStarted = IsStarted;

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return;

error:

    LOG("Failed to get lsassd status with error: %d", dwError);

    goto cleanup;
}

LONG
GetHomeDirectoryDockMCXValue(
    PMCXVALUE * ppMCXValueList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PMCXVALUE pMCXValues = NULL;
    char szMCXValueFile[PATH_MAX] = { 0 };

    if (!ppMCXValueList)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    sprintf(szMCXValueFile, "/etc/likewise/user-home-directory-in-dock.plist");

    dwError = ReadMCXValueFromFile(szMCXValueFile, &pMCXValues);
    BAIL_ON_MAC_ERROR(dwError);

    *ppMCXValueList = pMCXValues;
    pMCXValues = NULL;

cleanup:

    if (pMCXValues)
        FreeMCXValueList(pMCXValues);

    return LWGetMacError(dwError);

error:

    if (ppMCXValueList)
    {
        *ppMCXValueList = NULL;
    }

    goto cleanup;
}

void
FreeADUserInfo(
    PAD_USER_ATTRIBUTES pUserADAttrs
    )
{
    if (pUserADAttrs)
    {
        LW_SAFE_FREE_STRING(pUserADAttrs->pszDisplayName);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszFirstName);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszLastName);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszADDomain);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszKerberosPrincipal);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszEMailAddress);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszMSExchHomeServerName);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszMSExchHomeMDB);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszTelephoneNumber);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszFaxTelephoneNumber);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszMobileTelephoneNumber);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszStreetAddress);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszPostOfficeBox);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszCity);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszState);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszPostalCode);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszCountry);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszTitle);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszCompany);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszDepartment);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszHomeDirectory);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszHomeDrive);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszPasswordLastSet);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszUserAccountControl);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszMaxMinutesUntilChangePassword);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszMinMinutesUntilChangePassword);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszMaxFailedLoginAttempts);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszAllowedPasswordHistory);
        LW_SAFE_FREE_STRING(pUserADAttrs->pszMinCharsAllowedInPassword);

        LwFreeMemory(pUserADAttrs);
    }
}

LONG
GetADUserInfo(
    uid_t uid,
    PAD_USER_ATTRIBUTES * ppadUserInfo
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PAD_USER_ATTRIBUTES padUserInfo = NULL;
    PSTR pszValue = NULL;
    PCFGSECTION pSectionList = NULL;
    char      szUserAttrCacheFile[PATH_MAX] = { 0 };

    if (!uid)
    {
        LOG_ERROR("Called with invalid parameter");
        goto cleanup;
    }

    dwError = LwAllocateMemory(sizeof(AD_USER_ATTRIBUTES), (PVOID *) &padUserInfo);
    BAIL_ON_MAC_ERROR(dwError);

    sprintf(szUserAttrCacheFile, "/var/lib/likewise/lwedsplugin/user-cache/%ld/ad-user-attrs", (long) uid);

    /* Get user attributes that apply to user by parsing ad-user-attrs for specific user*/
    dwError = LWParseConfigFile(szUserAttrCacheFile,
                                &pSectionList,
                                FALSE);
    if (dwError)
    {
        goto cleanup;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "displayName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszDisplayName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "givenName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszFirstName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "sn",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszLastName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "userDomain",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszADDomain = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "userPrincipalName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszKerberosPrincipal = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "mail",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszEMailAddress = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "msExchHomeServerName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMSExchHomeServerName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "homeMDB",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMSExchHomeMDB = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "telephoneNumber",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "facsimileTelephoneNumber",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszFaxTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "mobile",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMobileTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "streetAddress",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszStreetAddress = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "postOfficeBox",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPostOfficeBox = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "l",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCity = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "st",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszState = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "postalCode",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPostalCode = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "co",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCountry = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "title",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszTitle = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "company",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCompany = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "department",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszDepartment = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "homeDirectory",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszHomeDirectory = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "homeDrive",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszHomeDrive = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "pwdLastSet",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPasswordLastSet = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "userAccountControl",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszUserAccountControl = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "maxPwdAge",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMaxMinutesUntilChangePassword = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "minPwdAge",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMinMinutesUntilChangePassword = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "lockoutThreshhold",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMaxFailedLoginAttempts = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "pwdHistoryLength",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszAllowedPasswordHistory = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "minPwdLength",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMinCharsAllowedInPassword = pszValue;
        pszValue = NULL;
    }

    dwError = MAC_AD_ERROR_SUCCESS;
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

    LW_SAFE_FREE_STRING(pszValue);

    return LWGetMacError(dwError);

error:

    *ppadUserInfo = NULL;
    dwError = MAC_AD_ERROR_SUCCESS;

    goto cleanup;
}

static
DWORD
ReadConfigDword(
    HANDLE hConnection,
    HANDLE hKey,
    PCSTR  pszName,
    DWORD  dwMin,
    DWORD  dwMax,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;

    BOOLEAN bGotValue = FALSE;
    DWORD dwValue;
    DWORD dwSize;
    DWORD dwType;

    dwSize = sizeof(dwValue);
    dwError = RegGetValueA(
                hConnection,
                hKey,
                LWDSPLUGIN_POLICIES,
                pszName,
                RRF_RT_REG_DWORD,
                &dwType,
                (PBYTE)&dwValue,
                &dwSize);
    if (!dwError)
    {
        bGotValue = TRUE;
    }

    if (!bGotValue)
    {
        dwSize = sizeof(dwValue);
        dwError = RegGetValueA(
                    hConnection,
                    hKey,
                    LWDSPLUGIN_SETTINGS,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!dwError)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        if ( dwMin <= dwValue && dwValue <= dwMax)
            *pdwValue = dwValue;
    }

    dwError = 0;

    return dwError;
}

static
DWORD
ReadConfigString(
    HANDLE  hConnection,
    HANDLE  hKey,
    PCSTR   pszName,
    PSTR    *ppszValue
    )
{
    DWORD dwError = 0;

    BOOLEAN bGotValue = FALSE;
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH];
    DWORD dwType = 0;
    DWORD dwSize = 0;

    dwSize = sizeof(szValue);
    memset(szValue, 0, dwSize);

    dwError = RegGetValueA(
                 hConnection,
                 hKey,
                 LWDSPLUGIN_POLICIES,
                 pszName,
                 RRF_RT_REG_SZ,
                 &dwType,
                 szValue,
                 &dwSize);
    if (!dwError)
        bGotValue = TRUE;

    if (!bGotValue )
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        dwError = RegGetValueA(
                    hConnection,
                    hKey,
                    LWDSPLUGIN_SETTINGS,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!dwError)
            bGotValue = TRUE;
    }

    if (bGotValue)
    {
        dwError = LwAllocateString(szValue, &pszValue);
        BAIL_ON_MAC_ERROR(dwError);

        LW_SAFE_FREE_STRING(*ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;
    }

    dwError = 0;

cleanup:

    LW_SAFE_FREE_STRING(pszValue);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
ReadConfigBoolean(
    HANDLE   hConnection,
    HANDLE   hKey,
    PCSTR    pszName,
    PBOOLEAN pbValue
    )
{

    DWORD dwError = 0;

    DWORD dwValue = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    dwError = ReadConfigDword(
                hConnection,
                hKey,
                pszName,
                0,
                -1,
                &dwValue);
    BAIL_ON_MAC_ERROR(dwError);

    *pbValue = dwValue ? TRUE : FALSE;

cleanup:

    return dwError;

error:
    goto cleanup;
}

LONG
GetConfigurationSettings(
    BOOLEAN * pbMergeModeMCX,
    BOOLEAN * pbEnableForceHomedirOnStartupDisk,
    BOOLEAN * pbUseADUNCForHomeLocation,
    PSTR *    ppszUNCProtocolForHomeLocation,
    PSTR *    ppszAllowAdministrationBy,
    BOOLEAN * pbMergeAdmins,
    DWORD *   pdwCacheLifeTime
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hConnection = NULL;
    HANDLE hKey = NULL;
    PSTR pszUNCProtocolForHomeLocation = NULL;
    PSTR pszAllowAdministrationBy = NULL;

    dwError = RegOpenServer(&hConnection);
    if (dwError)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
        goto error;
    }

    dwError = RegOpenKeyExA(
                hConnection,
                NULL,
                HKEY_THIS_MACHINE,
                (DWORD) 0,
                (REGSAM) KEY_READ,
                (PHKEY) &hKey);
    if (dwError)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
        goto error;
    }

    dwError = ReadConfigBoolean(hConnection,
                                hKey,
                                "EnableMergeModeMCX",
                                pbMergeModeMCX);
    if (dwError)
    {
        *pbMergeModeMCX = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = ReadConfigBoolean(hConnection,
                                hKey,
                                "EnableForceHomedirOnStartupDisk",
                                pbEnableForceHomedirOnStartupDisk);
    if (dwError)
    {
        *pbEnableForceHomedirOnStartupDisk = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = ReadConfigBoolean(hConnection,
                                hKey,
                                "UseADUncForHomeLocation",
                                pbUseADUNCForHomeLocation);
    if (dwError)
    {
        *pbUseADUNCForHomeLocation = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = ReadConfigString(hConnection,
                                hKey,
                                "UncProtocolForHomeLocation",
                                &pszUNCProtocolForHomeLocation);
    if (dwError || !pszUNCProtocolForHomeLocation)
    {
        dwError = LwAllocateString("smb", &pszUNCProtocolForHomeLocation);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pszUNCProtocolForHomeLocation)
    {
        // Make sure that the value is valid
        if (strcmp(pszUNCProtocolForHomeLocation, "afp") || strcmp(pszUNCProtocolForHomeLocation, "AFP") ||
            strcmp(pszUNCProtocolForHomeLocation, "smb") || strcmp(pszUNCProtocolForHomeLocation, "SMB"))
        {
            LW_SAFE_FREE_STRING(pszUNCProtocolForHomeLocation);
            pszUNCProtocolForHomeLocation = NULL;
        }
    }

    dwError = ReadConfigString(hConnection,
                                hKey,
                                "AllowAdministrationBy",
                                &pszAllowAdministrationBy);
    if (dwError)
    {
        pszAllowAdministrationBy = NULL;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = ReadConfigBoolean(hConnection,
                                hKey,
                                "EnableMergeAdmins",
                                pbMergeAdmins);
    if (dwError)
    {
        *pbMergeAdmins = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = ReadConfigDword(hConnection,
                                hKey,
                                "CacheLifeTime",
                                0,
                                INT_MAX,
                                pdwCacheLifeTime);
    if (dwError)
    {
        *pdwCacheLifeTime = 10; /* default is 10 seconds. */
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    *ppszUNCProtocolForHomeLocation = pszUNCProtocolForHomeLocation;
    pszUNCProtocolForHomeLocation = NULL;
    *ppszAllowAdministrationBy = pszAllowAdministrationBy;
    pszAllowAdministrationBy = NULL;

cleanup:

    if (hKey)
    {
        RegCloseKey(hConnection, hKey);
    }

    if (hConnection)
    {
        RegCloseServer(hConnection);
    }

    return LWGetMacError(dwError);

error:

    *pbMergeModeMCX = FALSE;
    *pbEnableForceHomedirOnStartupDisk = FALSE;
    *pbUseADUNCForHomeLocation = FALSE;
    *ppszUNCProtocolForHomeLocation = NULL;
    *ppszAllowAdministrationBy = NULL;
    *pbMergeAdmins = FALSE;

    dwError = 0;

    goto cleanup;
}

static
DWORD
LWStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    )
{
    DWORD dwError = 0;
    size_t copylen = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    copylen = strlen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = LwAllocateMemory(copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_MAC_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}

LONG
GetAccessCheckData(
    PSTR    pszAllowList,
    PVOID * ppAccessData
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
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

    dwError = LwAllocateMemory((dwCount+1)*sizeof(PCSTR), (PVOID *)&ppczStrArray);
    BAIL_ON_MAC_ERROR(dwError);

    cp = pszAllowList;
    for ( ;; )
    {
         cp2 = strchr(cp, ',');
         if (cp2)
         {
             dwError = LWStrndup( cp, cp2 - cp, &cp3 );
             BAIL_ON_MAC_ERROR(dwError);
         }
         else
         {
             dwError = LWStrndup( cp, strlen(cp), &cp3 );
             BAIL_ON_MAC_ERROR(dwError);
         }

         LwStripWhitespace(cp3, TRUE, TRUE);

         if ( strlen(cp3) > 0 )
         {
             ppczStrArray[dwIndex++] = cp3;
         }
         else
         {
             LwFreeMemory(cp3);
         }

         if (!cp2) break;

         cp = ++cp2;
    }

    if ( dwIndex == 0 )
    {
        *ppAccessData = NULL;
        goto cleanup;
    }

    dwError = LsaAccessGetData((PCSTR *)ppczStrArray, &pAccessData);
    BAIL_ON_MAC_ERROR(dwError);

    *ppAccessData = pAccessData;
    pAccessData = NULL;

cleanup:

    if ( ppczStrArray )
    {
        for ( dwIndex = 0 ; ppczStrArray[dwIndex] != NULL ; dwIndex++ )
        {
            LW_SAFE_FREE_STRING(ppczStrArray[dwIndex]);
        }

        LwFreeMemory(ppczStrArray);
    }

    return LWGetMacError(dwError);

error:

    *ppAccessData = NULL;

    goto cleanup;
}

LONG
CheckUserForAccess(
    PCSTR  pszUsername,
    PCVOID pAccessData
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;

    dwError = LsaAccessCheckData(pszUsername, pAccessData);

    return LWGetMacError(dwError);
}

LONG
FreeAccessCheckData(
    PVOID pAccessData
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;

    dwError = LsaAccessFreeData(pAccessData);

    return LWGetMacError(dwError);
}

LONG
CopyADUserInfo(
	PAD_USER_ATTRIBUTES pUserADInfo,
	PAD_USER_ATTRIBUTES * ppUserADInfoCopy
	)
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PAD_USER_ATTRIBUTES pNew = NULL;
	
    if (pUserADInfo)
    {
	dwError = LwAllocateMemory(sizeof(AD_USER_ATTRIBUTES), (PVOID *) &pNew);
	BAIL_ON_MAC_ERROR(dwError);
		
        if (pUserADInfo->pszDisplayName)
        {
            dwError = LwAllocateString(pUserADInfo->pszDisplayName, &pNew->pszDisplayName);
            BAIL_ON_MAC_ERROR(dwError);
        }
		
        if (pUserADInfo->pszFirstName)
        {
            dwError = LwAllocateString(pUserADInfo->pszFirstName, &pNew->pszFirstName);
            BAIL_ON_MAC_ERROR(dwError);
        }
		
        if (pUserADInfo->pszLastName)
        {
            dwError = LwAllocateString(pUserADInfo->pszLastName, &pNew->pszLastName);
            BAIL_ON_MAC_ERROR(dwError);
        }
		
        if (pUserADInfo->pszADDomain)
        {
            dwError = LwAllocateString(pUserADInfo->pszADDomain, &pNew->pszADDomain);
            BAIL_ON_MAC_ERROR(dwError);
	}
            
        if (pUserADInfo->pszKerberosPrincipal)
        {
            dwError = LwAllocateString(pUserADInfo->pszKerberosPrincipal, &pNew->pszKerberosPrincipal);
            BAIL_ON_MAC_ERROR(dwError);
	}
            
        if (pUserADInfo->pszEMailAddress)
        {
            dwError = LwAllocateString(pUserADInfo->pszEMailAddress, &pNew->pszEMailAddress);
            BAIL_ON_MAC_ERROR(dwError);
	}
            
        if (pUserADInfo->pszMSExchHomeServerName)
        {
            dwError = LwAllocateString(pUserADInfo->pszMSExchHomeServerName, &pNew->pszMSExchHomeServerName);
            BAIL_ON_MAC_ERROR(dwError);
	}
            
        if (pUserADInfo->pszMSExchHomeMDB)
        {
            dwError = LwAllocateString(pUserADInfo->pszMSExchHomeMDB, &pNew->pszMSExchHomeMDB);
            BAIL_ON_MAC_ERROR(dwError);
	}
            
        if (pUserADInfo->pszTelephoneNumber)
        {
            dwError = LwAllocateString(pUserADInfo->pszTelephoneNumber, &pNew->pszTelephoneNumber);
            BAIL_ON_MAC_ERROR(dwError);
	}
            
        if (pUserADInfo->pszFaxTelephoneNumber)
        {
            dwError = LwAllocateString(pUserADInfo->pszFaxTelephoneNumber, &pNew->pszFaxTelephoneNumber);
            BAIL_ON_MAC_ERROR(dwError);
        }
        
        if (pUserADInfo->pszMobileTelephoneNumber)
        {
            dwError = LwAllocateString(pUserADInfo->pszMobileTelephoneNumber, &pNew->pszMobileTelephoneNumber);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszStreetAddress)
        {
            dwError = LwAllocateString(pUserADInfo->pszStreetAddress, &pNew->pszStreetAddress);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszPostOfficeBox)
        {
            dwError = LwAllocateString(pUserADInfo->pszPostOfficeBox, &pNew->pszPostOfficeBox);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszCity)
        {
            dwError = LwAllocateString(pUserADInfo->pszCity, &pNew->pszCity);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszState)
        {
            dwError = LwAllocateString(pUserADInfo->pszState, &pNew->pszState);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszPostalCode)
        {
            dwError = LwAllocateString(pUserADInfo->pszPostalCode, &pNew->pszPostalCode);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszCountry)
        {
            dwError = LwAllocateString(pUserADInfo->pszCountry, &pNew->pszCountry);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszTitle)
        {
            dwError = LwAllocateString(pUserADInfo->pszTitle, &pNew->pszTitle);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszCompany)
        {
            dwError = LwAllocateString(pUserADInfo->pszCompany, &pNew->pszCompany);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszDepartment)
        {
            dwError = LwAllocateString(pUserADInfo->pszDepartment, &pNew->pszDepartment);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszHomeDirectory)
        {
            dwError = LwAllocateString(pUserADInfo->pszHomeDirectory, &pNew->pszHomeDirectory);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszHomeDrive)
        {
            dwError = LwAllocateString(pUserADInfo->pszHomeDrive, &pNew->pszHomeDrive);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszPasswordLastSet)
        {
            dwError = LwAllocateString(pUserADInfo->pszPasswordLastSet, &pNew->pszPasswordLastSet);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszUserAccountControl)
        {
            dwError = LwAllocateString(pUserADInfo->pszUserAccountControl, &pNew->pszUserAccountControl);
            BAIL_ON_MAC_ERROR(dwError);
	}
 
        if (pUserADInfo->pszMaxMinutesUntilChangePassword)
        {
            dwError = LwAllocateString(pUserADInfo->pszMaxMinutesUntilChangePassword, &pNew->pszMaxMinutesUntilChangePassword);
            BAIL_ON_MAC_ERROR(dwError);
	}
 
        if (pUserADInfo->pszMinMinutesUntilChangePassword)
        {
            dwError = LwAllocateString(pUserADInfo->pszMinMinutesUntilChangePassword, &pNew->pszMinMinutesUntilChangePassword);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszMaxFailedLoginAttempts)
        {
            dwError = LwAllocateString(pUserADInfo->pszMaxFailedLoginAttempts, &pNew->pszMaxFailedLoginAttempts);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszAllowedPasswordHistory)
        {
            dwError = LwAllocateString(pUserADInfo->pszAllowedPasswordHistory, &pNew->pszAllowedPasswordHistory);
            BAIL_ON_MAC_ERROR(dwError);
	}
        
        if (pUserADInfo->pszMinCharsAllowedInPassword)
        {
            dwError = LwAllocateString(pUserADInfo->pszMinCharsAllowedInPassword, &pNew->pszMinCharsAllowedInPassword);
            BAIL_ON_MAC_ERROR(dwError);
        }
    }
    
    *ppUserADInfoCopy = pNew;
    pNew = NULL;

cleanup:

    if (pNew)
    {
        FreeADUserInfo(pNew);
    }

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
GetUserObjects(
    PLSA_SECURITY_OBJECT** pppUserObjects,
    PDWORD                 pdwNumUsersFound
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    HANDLE hEnum = (HANDLE)NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    const DWORD dwMaxCount = 1000;
    DWORD  dwCount = 0;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaOpenEnumObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  &hEnum,
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_USER,
                  NULL /* DomainName */);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaEnumObjects(
                  hLsaConnection,
                  hEnum,
                  dwMaxCount,
                  &dwCount,
                  &ppObjects);
    if (dwError == ERROR_NO_MORE_ITEMS)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    if (dwCount == 0)
    {
        *pppUserObjects = NULL;
        *pdwNumUsersFound = 0;
        dwError = LW_ERROR_NO_MORE_USERS;
        goto cleanup;
    }

    LOG("Found %d users", dwCount);

    *pppUserObjects = ppObjects;
    ppObjects = NULL;

    *pdwNumUsersFound = dwCount;

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(dwCount, ppObjects);
    }

    if (hEnum != (HANDLE)NULL) {
        LsaCloseEnum(hLsaConnection, hEnum);
    }

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("Failed with error: %d", dwError);

    goto cleanup;
}

LONG
GetGroupObjects(
    PLSA_SECURITY_OBJECT** pppGroupObjects,
    PDWORD                 pdwNumGroupsFound
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    HANDLE hEnum = (HANDLE)NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    const DWORD dwMaxCount = 1000;
    DWORD  dwCount = 0;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaOpenEnumObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  &hEnum,
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_GROUP,
                  NULL /* DomainName */);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaEnumObjects(
                  hLsaConnection,
                  hEnum,
                  dwMaxCount,
                  &dwCount,
                  &ppObjects);
    if (dwError == ERROR_NO_MORE_ITEMS)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    if (dwCount == 0)
    {
        *pppGroupObjects = NULL;
        *pdwNumGroupsFound = 0;
        dwError = LW_ERROR_NO_MORE_GROUPS;
        goto cleanup;
    }

    LOG("Found %d groups", dwCount);

    *pppGroupObjects = ppObjects;
    ppObjects = NULL;

    *pdwNumGroupsFound = dwCount;

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(dwCount, ppObjects);
    }

    if (hEnum != (HANDLE)NULL) {
        LsaCloseEnum(hLsaConnection, hEnum);
    }

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("Failed with error: %d", dwError);

    goto cleanup;
}

LONG
GetUserObjectFromId(
    uid_t                  uid,
    PLSA_SECURITY_OBJECT** pppUserObject
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_QUERY_LIST QueryList = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LwAllocateMemory(sizeof(QueryList.pdwIds) * 2, (PVOID*)&QueryList.pdwIds);
    BAIL_ON_MAC_ERROR(dwError);

    QueryList.pdwIds[0] = (DWORD)uid;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_USER,
                  LSA_QUERY_TYPE_BY_UNIX_ID,
                  1,
                  QueryList,
                  &ppObjects);
    BAIL_ON_MAC_ERROR(dwError);

    if (ppObjects && ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        goto cleanup;
    }

    LOG("(Id: %d) found user (Name: %s, SID: %s) ", uid, ppObjects[0]->userInfo.pszUnixName, ppObjects[0]->pszObjectSid);

    *pppUserObject = ppObjects;
    ppObjects = NULL;

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(1, ppObjects);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (QueryList.pdwIds)
    {
        LwFreeMemory(QueryList.pdwIds);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("(Id: %d). Failed with error: %d", uid, dwError);

    goto cleanup;
}

LONG
GetUserObjectFromName(
    PCSTR                  pszName,
    PLSA_SECURITY_OBJECT** pppUserObject
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_QUERY_LIST QueryList = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    QueryList.ppszStrings = (PCSTR*)&pszName;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_USER,
                  LSA_QUERY_TYPE_BY_NAME,
                  1,
                  QueryList,
                  &ppObjects);
    BAIL_ON_MAC_ERROR(dwError);

    if (ppObjects && ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        goto cleanup;
    }

    LOG("(Name: %s) found user (uid: %d, SID: %s) ", pszName, ppObjects[0]->userInfo.uid, ppObjects[0]->pszObjectSid);

    *pppUserObject = ppObjects;
    ppObjects = NULL;

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(1, ppObjects);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("(Name: %s). Failed with error: %d", pszName, dwError);

    goto cleanup;
}

LONG
GetUserGroups(
    PCSTR                  pszUserSid,
    PLSA_SECURITY_OBJECT** pppGroups,
    PDWORD                 pdwNumGroupsFound
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PSTR* ppszGroups = NULL;
    DWORD dwCount = 0;
    LSA_QUERY_LIST QueryList = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaQueryMemberOf(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  1,
                  (PSTR*)&pszUserSid,
                  &dwCount,
                  &ppszGroups);
    BAIL_ON_MAC_ERROR(dwError);

    if (dwCount == 0)
    {
        *pppGroups = NULL;
        *pdwNumGroupsFound = 0;
        goto cleanup;
    }

    QueryList.ppszStrings = (PCSTR*)ppszGroups;

    dwError = LsaFindObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_GROUP,
                  LSA_QUERY_TYPE_BY_SID,
                  dwCount,
                  QueryList,
                  &ppObjects);
    BAIL_ON_MAC_ERROR(dwError);

    LOG("(SID %s) found %d groups", pszUserSid, dwCount);

    *pppGroups = ppObjects;
    ppObjects = NULL;
    *pdwNumGroupsFound = dwCount;

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(dwCount, ppObjects);
    }

    if (ppszGroups)
    {
        LsaFreeSidList(dwCount, ppszGroups);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("(SID: %s). Failed with error: %d", pszUserSid, dwError);

    goto cleanup;
}

LONG
GetGroupObjectFromId(
    gid_t                  gid,
    PLSA_SECURITY_OBJECT** pppGroupObject
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_QUERY_LIST QueryList = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LwAllocateMemory(sizeof(QueryList.pdwIds) * 2, (PVOID*)&QueryList.pdwIds);
    BAIL_ON_MAC_ERROR(dwError);

    QueryList.pdwIds[0] = gid;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_GROUP,
                  LSA_QUERY_TYPE_BY_UNIX_ID,
                  1,
                  QueryList,
                  &ppObjects);
    BAIL_ON_MAC_ERROR(dwError);

    if (ppObjects && ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        goto cleanup;
    }

    LOG("(Id: %d) found group (Name: %s, SID: %s) ", gid, ppObjects[0]->groupInfo.pszUnixName, ppObjects[0]->pszObjectSid);

    *pppGroupObject = ppObjects;
    ppObjects = NULL;

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(1, ppObjects);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (QueryList.pdwIds)
    {
        LwFreeMemory(QueryList.pdwIds);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("(Id: %d). Failed with error: %d", gid, dwError);

    goto cleanup;
}

LONG
GetGroupObjectFromName(
    PCSTR                  pszName,
    PLSA_SECURITY_OBJECT** pppGroupObject
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_QUERY_LIST QueryList = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    QueryList.ppszStrings = (PCSTR*)&pszName;;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_GROUP,
                  LSA_QUERY_TYPE_BY_NAME,
                  1,
                  QueryList,
                  &ppObjects);
    BAIL_ON_MAC_ERROR(dwError);

    if (ppObjects && ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        goto cleanup;
    }

    LOG("(Name: %s) found group (Id: %d, SID: %s) ", pszName, ppObjects[0]->groupInfo.gid, ppObjects[0]->pszObjectSid);

    *pppGroupObject = ppObjects;
    ppObjects = NULL;

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(1, ppObjects);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("(Name: %s). Failed with error: %d", pszName, dwError);

    goto cleanup;
}

LONG
ExpandGroupMembers(
    PCSTR                  pszGroupSid,
    PLSA_SECURITY_OBJECT** pppMembers,
    PDWORD                 pdwMemberCount
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    HANDLE hEnum = (HANDLE)NULL;
    PSTR* ppszMembers = NULL;
    DWORD dwCount = 0;
    LSA_QUERY_LIST QueryList = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaOpenEnumMembers(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  &hEnum,
                  0, /* FindFlags */
                  pszGroupSid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaEnumMembers(
                  hLsaConnection,
                  hEnum,
                  1000, /* Max Count */
                  &dwCount,
                  &ppszMembers);
    if (dwError == ERROR_NO_MORE_ITEMS)
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }
    BAIL_ON_MAC_ERROR(dwError);

    if (dwCount == 0)
    {
        *pppMembers = NULL;
        *pdwMemberCount = 0;
        goto cleanup;
    }

    QueryList.ppszStrings = (PCSTR*)ppszMembers;

    dwError = LsaFindObjects(
                  hLsaConnection,
                  NULL, /* TargetProvider - defaults to all */
                  0, /* FindFlags - LSA_FIND_FLAGS_NSS */
                  LSA_OBJECT_TYPE_UNDEFINED,
                  LSA_QUERY_TYPE_BY_SID,
                  dwCount,
                  QueryList,
                  &ppObjects);
    BAIL_ON_MAC_ERROR(dwError);

    *pppMembers = ppObjects;
    ppObjects = NULL;
    *pdwMemberCount = dwCount;

    LOG("(Group SID: %s) found %d members", pszGroupSid, dwCount);

cleanup:

    if (ppObjects) {
       LsaFreeSecurityObjectList(dwCount, ppObjects);
    }

    if (ppszMembers)
    {
        LsaFreeSidList(dwCount, ppszMembers);
    }

    if (hEnum != (HANDLE)NULL) {
        LsaCloseEnum(hLsaConnection, hEnum);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return LWGetMacError(dwError);

error:

    LOG_ERROR("(Group SID: %s). Failed with error: %d", pszGroupSid, dwError);

    goto cleanup;
}

void
FreeUserInfo(
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    LsaFreeUserInfo(2, pUserInfo2);
}

void
FreeUserInfoList(
    PVOID* ppUserInfo2List,
    DWORD  dwNumberOfUsers
    )
{
    LsaFreeUserInfoList(2, ppUserInfo2List, dwNumberOfUsers);
}

void
FreeGroupInfo(
    PLSA_GROUP_INFO_1 pGroupInfo1
    )
{
    LsaFreeGroupInfo(1, pGroupInfo1);
}

void
FreeGroupInfoList(
    PVOID* ppGroupInfo1List,
    DWORD  dwNumberOfGroups
    )
{
    LsaFreeGroupInfoList(1, ppGroupInfo1List, dwNumberOfGroups);
}

void
FreeObjectList(
    DWORD                 dwCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    LsaFreeSecurityObjectList(dwCount, ppObjects);
}

DWORD
SetupMCXLoginScriptsSupport(
    )
{
    DWORD dwError = 0;
    CHAR  szCommand[2 * PATH_MAX + 1];
    BOOLEAN bFileExists = FALSE;
 
    dwError = LwCheckFileTypeExists("/usr/bin/defaults", LWFILE_REGULAR, &bFileExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (bFileExists)
    {
        sprintf(szCommand, "/usr/bin/defaults write com.apple.loginwindow EnableMCXLoginScripts -boolean true");

        if (system(szCommand) < 0) {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        sprintf(szCommand, "/usr/bin/defaults write com.apple.loginwindow MCXScriptTrust -string FullTrust");

        if (system(szCommand) < 0) {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}


