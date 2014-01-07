/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        status.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Status (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvGetStatus(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSASTATUS* ppLsaStatus
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    DWORD dwProviderCount = 0;
    DWORD iCount = 0;
    DWORD dwStatusIndex = 0;
    HANDLE hProvider = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;
    PLSA_AUTH_PROVIDER_STATUS pProviderOwnedStatus = NULL;
    BOOLEAN bFoundProvider = FALSE;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    BAIL_ON_INVALID_POINTER(ppLsaStatus);

    dwError = LwAllocateMemory(
                  sizeof(LSASTATUS),
                  (PVOID*)&pLsaStatus);
    BAIL_ON_LSA_ERROR(dwError);

    pLsaStatus->dwUptime = (DWORD)difftime(time(NULL), gServerStartTime);
    
    dwError = LsaSrvGetLsassVersion(
                    &pLsaStatus->lsassVersion);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaReadVersionFile(
                    &pLsaStatus->productVersion);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (pszTargetProviderName)
    {
        dwProviderCount = 1;
    }
    else
    {
        dwProviderCount = LsaGetNumberOfProviders_inlock();
    }
    
    if (!dwProviderCount)
    {
        goto done;
    }
    
    dwError = LwAllocateMemory(
                    dwProviderCount * sizeof(LSA_AUTH_PROVIDER_STATUS),
                    (PVOID*)&pLsaStatus->pAuthProviderStatusList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pLsaStatus->dwCount = dwProviderCount;
        
    dwError = LW_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList, iCount = 0, dwStatusIndex = 0;
         pProvider;
         pProvider = pProvider->pNext, iCount++)
    {
        PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus = NULL;

        if (pszTargetProviderName)
        {
            if (!strcmp(pszTargetProviderName, pProvider->pszName))
            {
                bFoundProvider = TRUE;
            }
            else
            {
                continue;
            }
        }
        
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

       pAuthProviderStatus = &pLsaStatus->pAuthProviderStatusList[dwStatusIndex++];
        
        dwError = LwAllocateString(
                        pProvider->pszName,
                        &pAuthProviderStatus->pszId);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnGetStatus(
                                            hProvider,
                                            &pProviderOwnedStatus);
        if (dwError == LW_ERROR_NOT_HANDLED)
        {
            dwError = 0;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LsaSrvCopyProviderStatus(
                            pProviderOwnedStatus,
                            pAuthProviderStatus);
            BAIL_ON_LSA_ERROR(dwError);
            
            pProvider->pFnTable->pfnFreeStatus(
                            pProviderOwnedStatus);
            
            pProviderOwnedStatus = NULL;
        }

        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = (HANDLE)NULL;
    }

    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

done:

    *ppLsaStatus = pLsaStatus;

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (pProvider != NULL && pProviderOwnedStatus)
    {
        pProvider->pFnTable->pfnFreeStatus(
                        pProviderOwnedStatus);
    }
        
    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }
    
    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "get lsass status");

    if (ppLsaStatus)
    {
        *ppLsaStatus = NULL;
    }
    
    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

   goto cleanup;
}

DWORD
LsaSrvCopyProviderStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderOwnedStatus,
    PLSA_AUTH_PROVIDER_STATUS pTargetStatus
    )
{
    DWORD dwError = 0;
    
    pTargetStatus->mode = pProviderOwnedStatus->mode;
    
    LW_SAFE_FREE_STRING(pTargetStatus->pszCell);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pProviderOwnedStatus->pszCell))
    {
        dwError = LwAllocateString(
                        pProviderOwnedStatus->pszCell,
                        &pTargetStatus->pszCell);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LW_SAFE_FREE_STRING(pTargetStatus->pszDomain);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pProviderOwnedStatus->pszDomain))
    {
        dwError = LwAllocateString(
                        pProviderOwnedStatus->pszDomain,
                        &pTargetStatus->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LW_SAFE_FREE_STRING(pTargetStatus->pszDomainSid);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pProviderOwnedStatus->pszDomainSid))
    {
        dwError = LwAllocateString(
                        pProviderOwnedStatus->pszDomainSid,
                        &pTargetStatus->pszDomainSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LW_SAFE_FREE_STRING(pTargetStatus->pszForest);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pProviderOwnedStatus->pszForest))
    {
        dwError = LwAllocateString(
                        pProviderOwnedStatus->pszForest,
                        &pTargetStatus->pszForest);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LW_SAFE_FREE_STRING(pTargetStatus->pszId);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pProviderOwnedStatus->pszId))
    {
        dwError = LwAllocateString(
                    pProviderOwnedStatus->pszId,
                    &pTargetStatus->pszId);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LW_SAFE_FREE_STRING(pTargetStatus->pszSite);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pProviderOwnedStatus->pszSite))
    {
        dwError = LwAllocateString(
                    pProviderOwnedStatus->pszSite,
                    &pTargetStatus->pszSite);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pTargetStatus->status = pProviderOwnedStatus->status;
    pTargetStatus->subMode = pProviderOwnedStatus->subMode;
    pTargetStatus->dwNetworkCheckInterval = pProviderOwnedStatus->dwNetworkCheckInterval;
    
    if (pProviderOwnedStatus->pTrustedDomainInfoArray)
    {
        dwError = LsaSrvCopyTrustedDomainInfoArray(
                        pProviderOwnedStatus->dwNumTrustedDomains,
                        pProviderOwnedStatus->pTrustedDomainInfoArray,
                        &pTargetStatus->pTrustedDomainInfoArray);
        BAIL_ON_LSA_ERROR(dwError);
        
        pTargetStatus->dwNumTrustedDomains = pProviderOwnedStatus->dwNumTrustedDomains;
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvCopyTrustedDomainInfoArray(
    DWORD                     dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO  pSrcDomainInfoArray,
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray
    )
{
    DWORD dwError = 0;
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray = NULL;
    DWORD iDomain = 0;
    
    dwError = LwAllocateMemory(
                    dwNumDomains * sizeof(LSA_TRUSTED_DOMAIN_INFO),
                    (PVOID*)&pDomainInfoArray);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (; iDomain < dwNumDomains; iDomain++)
    {
        PLSA_TRUSTED_DOMAIN_INFO pSrcDomainInfo =
            &pSrcDomainInfoArray[iDomain];
        PLSA_TRUSTED_DOMAIN_INFO pDestDomainInfo =
            &pDomainInfoArray[iDomain];
        
        dwError = LwStrDupOrNull(
                        pSrcDomainInfo->pszDnsDomain,
                        &pDestDomainInfo->pszDnsDomain);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwStrDupOrNull(
                        pSrcDomainInfo->pszNetbiosDomain,
                        &pDestDomainInfo->pszNetbiosDomain);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwStrDupOrNull(
                        pSrcDomainInfo->pszDomainSID,
                        &pDestDomainInfo->pszDomainSID);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwStrDupOrNull(
                        pSrcDomainInfo->pszDomainGUID,
                        &pDestDomainInfo->pszDomainGUID);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwStrDupOrNull(
                        pSrcDomainInfo->pszTrusteeDnsDomain,
                        &pDestDomainInfo->pszTrusteeDnsDomain);
        BAIL_ON_LSA_ERROR(dwError);
        
        pDestDomainInfo->dwTrustFlags = pSrcDomainInfo->dwTrustFlags;
        pDestDomainInfo->dwTrustType = pSrcDomainInfo->dwTrustType;
        pDestDomainInfo->dwTrustAttributes = pSrcDomainInfo->dwTrustAttributes;
        pDestDomainInfo->dwTrustDirection = pSrcDomainInfo->dwTrustDirection;
        pDestDomainInfo->dwTrustMode = pSrcDomainInfo->dwTrustMode;
        
        dwError = LwStrDupOrNull(
                        pSrcDomainInfo->pszForestName,
                        &pDestDomainInfo->pszForestName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwStrDupOrNull(
                        pSrcDomainInfo->pszClientSiteName,
                        &pDestDomainInfo->pszClientSiteName);
        BAIL_ON_LSA_ERROR(dwError);
        
        pDestDomainInfo->dwDomainFlags = pSrcDomainInfo->dwDomainFlags;
        
        if (pSrcDomainInfo->pDCInfo)
        {
            dwError = LsaSrvCopyDCInfo(
                            pSrcDomainInfo->pDCInfo,
                            &pDestDomainInfo->pDCInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        if (pSrcDomainInfo->pGCInfo)
        {
            dwError = LsaSrvCopyDCInfo(
                            pSrcDomainInfo->pGCInfo,
                            &pDestDomainInfo->pGCInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    *ppDomainInfoArray = pDomainInfoArray;
    
cleanup:

    return dwError;
    
error:

    *ppDomainInfoArray = NULL;
    
    if (pDomainInfoArray)
    {
        LsaFreeDomainInfoArray(dwNumDomains, pDomainInfoArray);
    }

    goto cleanup;
}

DWORD
LsaSrvCopyDCInfo(
    PLSA_DC_INFO  pSrcInfo,
    PLSA_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    PLSA_DC_INFO pDCInfo = NULL;
    
    dwError = LwAllocateMemory(
                    sizeof(LSA_DC_INFO),
                    (PVOID*)&pDCInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwStrDupOrNull(
                    pSrcInfo->pszName,
                    &pDCInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwStrDupOrNull(
                    pSrcInfo->pszAddress,
                    &pDCInfo->pszAddress);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwStrDupOrNull(
                    pSrcInfo->pszSiteName,
                    &pDCInfo->pszSiteName);
    BAIL_ON_LSA_ERROR(dwError);
    
    pDCInfo->dwFlags = pSrcInfo->dwFlags;
    
    *ppDCInfo = pDCInfo;
    
cleanup:

    return dwError;
    
error:

    *ppDCInfo = NULL;
    
    if (pDCInfo)
    {
        LsaFreeDCInfo(pDCInfo);
    }
    goto cleanup;
}

DWORD
LsaSrvGetLsassVersion(
    PLSA_VERSION pVersion
    )
{  
    DWORD dwError = 0;
    PSTR pszVersion = NULL;
    DWORD iVerComp = 0;
    PSTR  pszToken = NULL;
    PSTR  pszTokenState = NULL;
    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwBuild = 0;
    DWORD dwRevision = 0;
    
    if (LW_IS_NULL_OR_EMPTY_STR(COMPONENT_VERSION))
    {
        dwError = LW_ERROR_INVALID_AGENT_VERSION;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwAllocateString(
                    COMPONENT_VERSION,
                    &pszVersion);
    BAIL_ON_LSA_ERROR(dwError);
    
    pszToken = strtok_r(pszVersion, ".",  &pszTokenState);
    
    while (!LW_IS_NULL_OR_EMPTY_STR(pszVersion) && (iVerComp < 4))
    {
        int i = 0;
        
        for (; i < strlen(pszVersion); i++)
        {
            if (!isdigit((int)pszVersion[i]))
            {
                dwError = LW_ERROR_INVALID_AGENT_VERSION;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        
        switch (iVerComp++)
        {
            case 0:
                
                dwMajor = atoi(pszToken);
                break;
                
            case 1:
                
                dwMinor = atoi(pszToken);
                break;
                
            case 2:
                
                dwBuild = atoi(pszToken);
                break;

            case 3:
                
                errno = 0;
                dwRevision = strtoul(pszToken, NULL, 10);
                dwError = LwMapErrnoToLwError(errno);
                if (dwError != 0)
                {
                    LSA_LOG_DEBUG("Unable to parse revision due to error %u", dwError);
                    dwRevision = 0;
                    dwError = 0;
                }
                break;
                
            default:
                
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }
        
        pszToken = strtok_r(NULL, ".", &pszTokenState);
    }
    
    if (iVerComp < 4)
    {
        dwError = LW_ERROR_INVALID_AGENT_VERSION;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pVersion->dwMajor = dwMajor;
    pVersion->dwMinor = dwMinor;
    pVersion->dwBuild = dwBuild;
    pVersion->dwRevision = dwRevision;
    
cleanup:

    LW_SAFE_FREE_MEMORY(pszVersion);

    return dwError;
    
error:

    memset(pVersion, 0, sizeof(*pVersion));
    
    goto cleanup;
}
