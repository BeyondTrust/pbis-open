/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Management Services Client API
 *
 */
#include "includes.h"

DWORD
LWMGMTQueryLsaMetrics_0(
    PCSTR pszHostname,
    PLSA_METRIC_PACK_0* ppPack
    )
{
    DWORD dwError = 0;
    handle_t hLsa = NULL;
    char* pszBinding = NULL;
    PLSA_METRIC_PACK_0 pPack = NULL;
    LSAMETRICPACK metricPack;

    dwError = LWMGMTOpenLsaServer(
                  pszHostname,
                  &pszBinding,
                  &hLsa);
    BAIL_ON_LWMGMT_ERROR(dwError); 

    dwError = LWMGMTGetLsaMetrics(
                  hLsa,
                  0,
                  &metricPack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTAllocateMemory(
                  sizeof(LSA_METRIC_PACK_0),
                  (PVOID*)&pPack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pPack->failedAuthentications =
           metricPack.pack0.failedAuthentications;
    pPack->failedUserLookupsByName =
           metricPack.pack0.failedUserLookupsByName;
    pPack->failedUserLookupsById =
           metricPack.pack0.failedUserLookupsById;
    pPack->failedGroupLookupsByName =
           metricPack.pack0.failedGroupLookupsByName;
    pPack->failedGroupLookupsById =
           metricPack.pack0.failedGroupLookupsById;
    pPack->failedOpenSession =
           metricPack.pack0.failedOpenSession;
    pPack->failedCloseSession =
           metricPack.pack0.failedCloseSession;
    pPack->failedChangePassword =
           metricPack.pack0.failedChangePassword;
    pPack->unauthorizedAccesses =
           metricPack.pack0.unauthorizedAccesses;

    *ppPack = pPack;

cleanup:

    if (hLsa)
    {
       LWMGMTCloseLsaServer(
                            hLsa,
                            pszBinding);
    }

    return dwError;

error:

    if (pPack)
    {
        LWMGMTFreeLsaMetrics_0(pPack);
    }

    *ppPack = NULL;

    goto cleanup;
}

VOID
LWMGMTFreeLsaMetrics_0(
    PLSA_METRIC_PACK_0 pPack
    )
{
    LWMGMT_SAFE_FREE_MEMORY(pPack);
}

DWORD
LWMGMTQueryLsaMetrics_1(
    PCSTR pszHostname,
    PLSA_METRIC_PACK_1* ppPack
    ) 
{
    DWORD dwError = 0;
    handle_t hLsa = NULL;
    char* pszBinding = NULL;
    PLSA_METRIC_PACK_1 pPack = NULL;
    LSAMETRICPACK metricPack;

    dwError = LWMGMTOpenLsaServer(
                  pszHostname,
                  &pszBinding,
                  &hLsa);
    BAIL_ON_LWMGMT_ERROR(dwError); 

    dwError = LWMGMTGetLsaMetrics(
                  hLsa,
                  1,
                  &metricPack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTAllocateMemory(
                  sizeof(LSA_METRIC_PACK_1),
                  (PVOID*)&pPack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pPack->successfulAuthentications =
           metricPack.pack1.successfulAuthentications;
    pPack->failedAuthentications =
           metricPack.pack1.failedAuthentications;
    pPack->successfulUserLookupsByName =
           metricPack.pack1.successfulUserLookupsByName;
    pPack->failedUserLookupsByName =
           metricPack.pack1.failedUserLookupsByName;
    pPack->successfulUserLookupsById =
           metricPack.pack1.successfulUserLookupsById;
    pPack->failedUserLookupsById =
           metricPack.pack1.failedUserLookupsById;
    pPack->successfulGroupLookupsByName =
           metricPack.pack1.successfulGroupLookupsByName;
    pPack->failedGroupLookupsByName =
           metricPack.pack1.failedGroupLookupsByName;
    pPack->successfulGroupLookupsById =
           metricPack.pack1.successfulGroupLookupsById;
    pPack->failedGroupLookupsById =
           metricPack.pack1.failedGroupLookupsById;
    pPack->successfulOpenSession =
           metricPack.pack1.successfulOpenSession;
    pPack->failedOpenSession =
           metricPack.pack1.failedOpenSession;
    pPack->successfulCloseSession =
           metricPack.pack1.successfulCloseSession;    
    pPack->failedCloseSession =
           metricPack.pack1.failedCloseSession;
    pPack->successfulChangePassword =
           metricPack.pack1.successfulChangePassword;    
    pPack->failedChangePassword =
           metricPack.pack1.failedChangePassword;
    pPack->unauthorizedAccesses =
           metricPack.pack1.unauthorizedAccesses;

    *ppPack = pPack;

cleanup:

    if (hLsa)
    {
       LWMGMTCloseLsaServer(
                            hLsa,
                            pszBinding);
    }

    return dwError;

error:

    if (pPack)
    {
        LWMGMTFreeLsaMetrics_1(pPack);
    }

    *ppPack = NULL;

    goto cleanup;
}

VOID
LWMGMTFreeLsaMetrics_1(
    PLSA_METRIC_PACK_1 pPack
    )
{
    LWMGMT_SAFE_FREE_MEMORY(pPack);
}

DWORD
LWMGMTQueryLsaStatus(
    PCSTR        pszHostname,
    PLSA_STATUS* ppLsaStatus
    )
{
    DWORD dwError = 0;
    handle_t hLsa = NULL;
    char* pszBinding = NULL;
    PLSA_STATUS pLsaStatus = NULL;
    LWMGMTLSASTATUS lsaStatus;
    DWORD iCount = 0;
    
    memset(&lsaStatus, 0, sizeof(lsaStatus));

    dwError = LWMGMTOpenLsaServer(
                  pszHostname,
                  &pszBinding,
                  &hLsa);
    BAIL_ON_LWMGMT_ERROR(dwError); 

    dwError = LWMGMTGetLsaStatus(
                  hLsa,
                  &lsaStatus);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTAllocateMemory(
                  sizeof(LSA_STATUS),
                  (PVOID*)&pLsaStatus);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    pLsaStatus->version.dwMajor = lsaStatus.agentVersion.major;
    pLsaStatus->version.dwMinor = lsaStatus.agentVersion.minor;
    pLsaStatus->version.dwBuild = lsaStatus.agentVersion.build;
    
    pLsaStatus->dwUptime = lsaStatus.uptime;
    pLsaStatus->dwCount = lsaStatus.count;
    
    if (lsaStatus.count)
    {
        dwError = LWMGMTAllocateMemory(
                        lsaStatus.count * sizeof(LSA_AUTH_PROVIDER_STATUS),
                        (PVOID*)&pLsaStatus->pAuthProviderStatusArray);
        BAIL_ON_LWMGMT_ERROR(dwError);
        
        for (iCount = 0; iCount < lsaStatus.count; iCount++)
        {
            PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus =
                &pLsaStatus->pAuthProviderStatusArray[iCount];
            LWMGMTLSAAUTHPROVIDERSTATUS* pRpcAuthProviderStatus =
                &lsaStatus.pAuthProviderStatusArray[iCount];
            
            pAuthProviderStatus->mode = pRpcAuthProviderStatus->mode;
            pAuthProviderStatus->status = pRpcAuthProviderStatus->status;
            pAuthProviderStatus->subMode = pRpcAuthProviderStatus->subMode;
            pAuthProviderStatus->dwNetworkCheckInterval = pRpcAuthProviderStatus->dwNetworkCheckInterval;
            
            if (pRpcAuthProviderStatus->pszId)
            {
                dwError = LWMGMTAllocateString(
                                (PCSTR)pRpcAuthProviderStatus->pszId,
                                &pAuthProviderStatus->pszId);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (pRpcAuthProviderStatus->pszDomain)
            {
                dwError = LWMGMTAllocateString(
                                (PCSTR)pRpcAuthProviderStatus->pszDomain,
                                &pAuthProviderStatus->pszDomain);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (pRpcAuthProviderStatus->pszForest)
            {
                dwError = LWMGMTAllocateString(
                                (PCSTR)pRpcAuthProviderStatus->pszForest,
                                &pAuthProviderStatus->pszForest);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (pRpcAuthProviderStatus->pszSite)
            {
                dwError = LWMGMTAllocateString(
                                (PCSTR)pRpcAuthProviderStatus->pszSite,
                                &pAuthProviderStatus->pszSite);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (pRpcAuthProviderStatus->pszCell)
            {
                dwError = LWMGMTAllocateString(
                                (PCSTR)pRpcAuthProviderStatus->pszCell,
                                &pAuthProviderStatus->pszCell);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (pRpcAuthProviderStatus->pTrustedDomainInfoArray)
            {
                dwError = LWMGMTBuildDomainInfoArray(
                                pRpcAuthProviderStatus->dwNumTrustedDomains,
                                pRpcAuthProviderStatus->pTrustedDomainInfoArray,
                                &pAuthProviderStatus->pTrustedDomainInfoArray);
                BAIL_ON_LWMGMT_ERROR(dwError);
                
                pAuthProviderStatus->dwNumTrustedDomains = pRpcAuthProviderStatus->dwNumTrustedDomains;
            }
        }
    }

    *ppLsaStatus = pLsaStatus;

cleanup:

    LWMGMTFreeRpcLsaStatus(&lsaStatus);

    if (hLsa)
    {
       LWMGMTCloseLsaServer(
                            hLsa,
                            pszBinding);
    }

    return dwError;

error:

    if (pLsaStatus)
    {
        LWMGMTFreeLsaStatus(pLsaStatus);
    }

    *ppLsaStatus = NULL;

    goto cleanup;    
}

DWORD
LWMGMTBuildDomainInfoArray(
    DWORD                     dwNumTrustedDomains,
    LWMGMTLSADOMAININFO*      pSrcDomainInfoArray,
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray
    )
{
    DWORD dwError = 0;
    PLSA_TRUSTED_DOMAIN_INFO pDestDomainInfoArray = NULL;
    DWORD iDomain = 0;
    
    dwError = LWMGMTAllocateMemory(
                    sizeof(LSA_TRUSTED_DOMAIN_INFO) * dwNumTrustedDomains,
                    (PVOID*)&pDestDomainInfoArray);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    for (; iDomain < dwNumTrustedDomains; iDomain++)
    {
        LWMGMTLSADOMAININFO* pSrcDomainInfo =
            &pSrcDomainInfoArray[iDomain];
        PLSA_TRUSTED_DOMAIN_INFO pDestDomainInfo =
            &pDestDomainInfoArray[iDomain];
        
        if (pSrcDomainInfo->pszDnsDomain)
        {
            dwError = LWMGMTAllocateString(
                            (PCSTR)pSrcDomainInfo->pszDnsDomain,
                            &pDestDomainInfo->pszDnsDomain);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        if (pSrcDomainInfo->pszNetbiosDomain)
        {
            dwError = LWMGMTAllocateString(
                            (PCSTR)pSrcDomainInfo->pszNetbiosDomain,
                            &pDestDomainInfo->pszNetbiosDomain);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        if (pSrcDomainInfo->pszTrusteeDnsDomain)
        {
            dwError = LWMGMTAllocateString(
                            (PCSTR)pSrcDomainInfo->pszTrusteeDnsDomain,
                            &pDestDomainInfo->pszTrusteeDnsDomain);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        if (pSrcDomainInfo->pszDomainSID)
        {
            dwError = LWMGMTAllocateString(
                            (PCSTR)pSrcDomainInfo->pszDomainSID,
                            &pDestDomainInfo->pszDomainSID);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        if (pSrcDomainInfo->pszDomainGUID)
        {
            dwError = LWMGMTAllocateString(
                            (PCSTR)pSrcDomainInfo->pszDomainGUID,
                            &pDestDomainInfo->pszDomainGUID);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        if (pSrcDomainInfo->pszForestName)
        {
            dwError = LWMGMTAllocateString(
                            (PCSTR)pSrcDomainInfo->pszForestName,
                            &pDestDomainInfo->pszForestName);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        if (pSrcDomainInfo->pszClientSiteName)
        {
            dwError = LWMGMTAllocateString(
                            (PCSTR)pSrcDomainInfo->pszClientSiteName,
                            &pDestDomainInfo->pszClientSiteName);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        pDestDomainInfo->dwTrustFlags = pSrcDomainInfo->dwTrustFlags;
        pDestDomainInfo->dwTrustType = pSrcDomainInfo->dwTrustType;
        pDestDomainInfo->dwTrustAttributes = pSrcDomainInfo->dwTrustAttributes;
        pDestDomainInfo->dwDomainFlags = pSrcDomainInfo->dwDomainFlags;
        
        if (pSrcDomainInfo->pDCInfo)
        {
            dwError = LWMGMTBuildDCInfo(
                            pSrcDomainInfo->pDCInfo,
                            &pDestDomainInfo->pDCInfo);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        if (pSrcDomainInfo->pGCInfo)
        {
            dwError = LWMGMTBuildDCInfo(
                            pSrcDomainInfo->pGCInfo,
                            &pDestDomainInfo->pGCInfo);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
    }
    
    *ppDomainInfoArray = pDestDomainInfoArray;
    
cleanup:

    return dwError;
    
error:

    *ppDomainInfoArray = NULL;
    
    if (pDestDomainInfoArray)
    {
        LWMGMTFreeDomainInfoArray(
                        dwNumTrustedDomains,
                        pDestDomainInfoArray);
    }
    
    goto cleanup;    
}

DWORD
LWMGMTBuildDCInfo(
    LWMGMTLSADCINFO* pSrcDCInfo,
    PLSA_DC_INFO*    ppDCInfo
    )
{
    DWORD dwError = 0;
    PLSA_DC_INFO pDCInfo = NULL;
    
    dwError = LWMGMTAllocateMemory(
                    sizeof(LSA_DC_INFO),
                    (PVOID*)&pDCInfo);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    if (pSrcDCInfo->pszName)
    {
        dwError = LWMGMTAllocateString(
                        (PCSTR)pSrcDCInfo->pszName,
                        &pDCInfo->pszName);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    
    if (pSrcDCInfo->pszAddress)
    {
        dwError = LWMGMTAllocateString(
                        (PCSTR)pSrcDCInfo->pszAddress,
                        &pDCInfo->pszAddress);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    
    if (pSrcDCInfo->pszSiteName)
    {
        dwError = LWMGMTAllocateString(
                        (PCSTR)pSrcDCInfo->pszSiteName,
                        &pDCInfo->pszSiteName);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    
    pDCInfo->dwFlags = pSrcDCInfo->dwFlags;
    
    *ppDCInfo = pDCInfo;
    
cleanup:

    return dwError;
    
error:

    *ppDCInfo = NULL;
    
    if (pDCInfo)
    {
        LWMGMTFreeDCInfo(pDCInfo);
    }
    
    goto cleanup;
}

VOID
LWMGMTFreeLsaStatus(
    PLSA_STATUS pLsaStatus
    )
{
    DWORD iCount = 0;
    
    for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
    {
        PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus =
            &pLsaStatus->pAuthProviderStatusArray[iCount];
        
        LWMGMT_SAFE_FREE_STRING(pAuthProviderStatus->pszId);
        LWMGMT_SAFE_FREE_STRING(pAuthProviderStatus->pszDomain);
        LWMGMT_SAFE_FREE_STRING(pAuthProviderStatus->pszForest);
        LWMGMT_SAFE_FREE_STRING(pAuthProviderStatus->pszSite);
        LWMGMT_SAFE_FREE_STRING(pAuthProviderStatus->pszCell);
    }
    
    LWMGMT_SAFE_FREE_MEMORY(pLsaStatus->pAuthProviderStatusArray);
    
    LWMGMTFreeMemory(pLsaStatus);
}

VOID
LWMGMTFreeDomainInfoArray(
    DWORD dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray
    )
{
    DWORD iDomain = 0;
    
    for (; iDomain < dwNumDomains; iDomain++)
    {
        PLSA_TRUSTED_DOMAIN_INFO pDomainInfo =
            &pDomainInfoArray[iDomain];
        
        LWMGMT_SAFE_FREE_STRING(pDomainInfo->pszDnsDomain);
        LWMGMT_SAFE_FREE_STRING(pDomainInfo->pszNetbiosDomain);
        LWMGMT_SAFE_FREE_STRING(pDomainInfo->pszTrusteeDnsDomain);
        LWMGMT_SAFE_FREE_STRING(pDomainInfo->pszDomainSID);
        LWMGMT_SAFE_FREE_STRING(pDomainInfo->pszDomainGUID);
        LWMGMT_SAFE_FREE_STRING(pDomainInfo->pszForestName);
        LWMGMT_SAFE_FREE_STRING(pDomainInfo->pszClientSiteName);
        
        if (pDomainInfo->pDCInfo)
        {
            LWMGMTFreeDCInfo(pDomainInfo->pDCInfo);
        }
        
        if (pDomainInfo->pGCInfo)
        {
            LWMGMTFreeDCInfo(pDomainInfo->pGCInfo);
        }
    }
    
    LWMGMTFreeMemory(pDomainInfoArray);
}

VOID
LWMGMTFreeDCInfo(
    PLSA_DC_INFO pDCInfo
    )
{
    LWMGMT_SAFE_FREE_STRING(pDCInfo->pszName);
    LWMGMT_SAFE_FREE_STRING(pDCInfo->pszAddress);
    LWMGMT_SAFE_FREE_STRING(pDCInfo->pszSiteName);
    
    LWMGMTFreeMemory(pDCInfo);
}

VOID
LWMGMTFreeRpcLsaStatus(
    LWMGMTLSASTATUS* pLsaStatus
    )
{
    if (pLsaStatus->count && pLsaStatus->pAuthProviderStatusArray)
    {
        unsigned32 iCount = 0;
    
        for (iCount = 0; iCount < pLsaStatus->count; iCount++)
        {
            LWMGMTLSAAUTHPROVIDERSTATUS* pAuthProviderStatus =
                            &pLsaStatus->pAuthProviderStatusArray[iCount];
            
            if (pAuthProviderStatus->pszId)
            {
                RPCFreeMemory(pAuthProviderStatus->pszId);
            }
            if (pAuthProviderStatus->pszDomain)
            {
                RPCFreeMemory(pAuthProviderStatus->pszDomain);
            }
            if (pAuthProviderStatus->pszForest)
            {
                RPCFreeMemory(pAuthProviderStatus->pszForest);
            }
            if (pAuthProviderStatus->pszSite)
            {
                RPCFreeMemory(pAuthProviderStatus->pszSite);
            }
            if (pAuthProviderStatus->pszCell)
            {
                RPCFreeMemory(pAuthProviderStatus->pszCell);
            }
            if (pAuthProviderStatus->pTrustedDomainInfoArray)
            {
                LWMGMTRpcFreeDomainInfoArray(
                                pAuthProviderStatus->dwNumTrustedDomains,
                                pAuthProviderStatus->pTrustedDomainInfoArray);
            }
        }
        
        RPCFreeMemory(pLsaStatus->pAuthProviderStatusArray);
    }
}

VOID
LWMGMTRpcFreeDomainInfoArray(
    DWORD                dwNumDomains,
    LWMGMTLSADOMAININFO* pDomainInfoArray
    )
{
    DWORD iDomain = 0;
    
    for (; iDomain < dwNumDomains; iDomain++)
    {
        LWMGMTLSADOMAININFO* pDomainInfo =
            &pDomainInfoArray[iDomain];
        
        if (pDomainInfo->pszDnsDomain)
        {
            RPCFreeMemory(pDomainInfo->pszDnsDomain);
        }
        
        if (pDomainInfo->pszNetbiosDomain)
        {
            RPCFreeMemory(pDomainInfo->pszNetbiosDomain);
        }
        
        if (pDomainInfo->pszTrusteeDnsDomain)
        {
            RPCFreeMemory(pDomainInfo->pszTrusteeDnsDomain);
        }
        
        if (pDomainInfo->pszDomainSID)
        {
            RPCFreeMemory(pDomainInfo->pszDomainSID);
        }
        
        if (pDomainInfo->pszDomainGUID)
        {
            RPCFreeMemory(pDomainInfo->pszDomainGUID);
        }
        
        if (pDomainInfo->pszForestName)
        {
            RPCFreeMemory(pDomainInfo->pszForestName);
        }
        
        if (pDomainInfo->pszClientSiteName)
        {
            RPCFreeMemory(pDomainInfo->pszClientSiteName);
        }
        
        if (pDomainInfo->pDCInfo)
        {
            LWMGMTRpcFreeDCInfo(pDomainInfo->pDCInfo);
        }
        
        if (pDomainInfo->pGCInfo)
        {
            LWMGMTRpcFreeDCInfo(pDomainInfo->pGCInfo);
        }
    }
    
    RPCFreeMemory(pDomainInfoArray);
}

VOID
LWMGMTRpcFreeDCInfo(
    LWMGMTLSADCINFO* pDCInfo
    )
{
    if (pDCInfo->pszName)
    {
        RPCFreeMemory(pDCInfo->pszName);
    }
    
    if (pDCInfo->pszAddress)
    {
        RPCFreeMemory(pDCInfo->pszAddress);
    }
    
    if (pDCInfo->pszSiteName)
    {
        RPCFreeMemory(pDCInfo->pszSiteName);
    }
    
    RPCFreeMemory(pDCInfo);
}

DWORD
LWMGMTReadKeyTab(
    PCSTR                        pszHostname,
    PCSTR                        pszKeyTabPath,
    DWORD                        dwLastRecordId,
    DWORD                        dwRecordsPerPage,
    PLWMGMT_LSA_KEYTAB_ENTRIES * ppKeyTabEntries
    )
{
    DWORD                      dwError = 0;
    DWORD                      dwCount = 0;
    handle_t                   hKeyTab = NULL;
    char*                      pszBinding = NULL;
    PLWMGMT_LSA_KEYTAB_ENTRIES pKeyTabEntries = NULL;
    LSA_KEYTAB_ENTRIES         pRPCKeyTabEntries;

    memset(&pRPCKeyTabEntries, 0, sizeof(pRPCKeyTabEntries));

    dwError = LWIOpenKeyTabServer(
                  pszHostname,
                  &pszBinding,
                  &hKeyTab);
    BAIL_ON_LWMGMT_ERROR(dwError); 

    dwError = LWIReadKeyTab(
                  hKeyTab,
                  pszKeyTabPath,
                  dwLastRecordId,
                  dwRecordsPerPage,
                  &pRPCKeyTabEntries);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTAllocateMemory(
                  sizeof(LWMGMT_LSA_KEYTAB_ENTRIES),
                  (PVOID*)&pKeyTabEntries);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pKeyTabEntries->dwCount = pRPCKeyTabEntries.count;

    if ( pRPCKeyTabEntries.count )
    {
        dwError = LWMGMTAllocateMemory(
                      pRPCKeyTabEntries.count * sizeof(LWMGMT_LSA_KEYTAB_ENTRY),
                      (PVOID*)&pKeyTabEntries->pLsaKeyTabEntryArray);
        BAIL_ON_LWMGMT_ERROR(dwError);

        for ( dwCount = 0 ; dwCount < pRPCKeyTabEntries.count ; dwCount++ )
        {
            PLWMGMT_LSA_KEYTAB_ENTRY pKeyTabEntry = 
                &pKeyTabEntries->pLsaKeyTabEntryArray[dwCount];
            LSA_KEYTAB_ENTRY * pRpcKeyTabEntry =
                &pRPCKeyTabEntries.pLsaKeyTabEntryArray[dwCount];

            pKeyTabEntry->timestamp = pRpcKeyTabEntry->timestamp;
            pKeyTabEntry->kvno = pRpcKeyTabEntry->kvno;
            pKeyTabEntry->enctype = pRpcKeyTabEntry->enctype;

            if ( pRpcKeyTabEntry )
            {
                dwError = LWMGMTAllocateString(
                              (PCSTR)pRpcKeyTabEntry->pszPrincipal,
                              &pKeyTabEntry->pszPrincipal);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
        }
    }

    *ppKeyTabEntries = pKeyTabEntries;

cleanup:

    LWMGMTFreeRpcKeyTabEntries(&pRPCKeyTabEntries);

    if (hKeyTab)
    {
       LWICloseKeyTabServer(
           hKeyTab,
           pszBinding);
    }

    return dwError;

error:

    if (pKeyTabEntries)
    {
        LWMGMTFreeKeyTabEntries(pKeyTabEntries);
    }

    *ppKeyTabEntries = NULL;

    goto cleanup;
}

VOID
LWMGMTFreeKeyTabEntries(
    PLWMGMT_LSA_KEYTAB_ENTRIES pKeyTabEntries
    )
{
    DWORD iCount = 0;
    
    for (iCount = 0; iCount < pKeyTabEntries->dwCount; iCount++)
    {
        PLWMGMT_LSA_KEYTAB_ENTRY pKeyTabEntry = 
            &pKeyTabEntries->pLsaKeyTabEntryArray[iCount];
        
        LWMGMT_SAFE_FREE_STRING(pKeyTabEntry->pszPrincipal);
    }
    
    LWMGMT_SAFE_FREE_MEMORY(pKeyTabEntries->pLsaKeyTabEntryArray);
    
    LWMGMTFreeMemory(pKeyTabEntries);
}

VOID
LWMGMTFreeRpcKeyTabEntries(
    LSA_KEYTAB_ENTRIES * pKeyTabEntries
    )
{
    if (pKeyTabEntries->count && pKeyTabEntries->pLsaKeyTabEntryArray)
    {
        unsigned32 iCount = 0;
    
        for (iCount = 0; iCount < pKeyTabEntries->count; iCount++)
        {
            LSA_KEYTAB_ENTRY * pKeyTabEntry =
                &pKeyTabEntries->pLsaKeyTabEntryArray[iCount];
            
            if (pKeyTabEntry->pszPrincipal)
            {
                RPCFreeMemory(pKeyTabEntry->pszPrincipal);
            }
        }
        
        RPCFreeMemory(pKeyTabEntries->pLsaKeyTabEntryArray);
    }
}

DWORD
LWMGMTCountKeyTabEntries(
    PCSTR  pszHostname,
    PCSTR  pszKeyTabPath,
    PDWORD pdwCount
    )
{
    DWORD                      dwError = 0;
    DWORD                      dwCount = 0;
    handle_t                   hKeyTab = NULL;
    char*                      pszBinding = NULL;

    dwError = LWIOpenKeyTabServer(
                  pszHostname,
                  &pszBinding,
                  &hKeyTab);
    BAIL_ON_LWMGMT_ERROR(dwError); 

    dwError = LWICountKeyTabEntries(
                  hKeyTab,
                  pszKeyTabPath,
                  &dwCount);
    BAIL_ON_LWMGMT_ERROR(dwError);

    *pdwCount = dwCount;

cleanup:


    if (hKeyTab)
    {
       LWICloseKeyTabServer(
           hKeyTab,
           pszBinding);
    }

    return dwError;

error:

    goto cleanup;
}

