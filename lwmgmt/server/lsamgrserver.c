/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server API
 *
 */
#include "includes.h"

idl_long_int
RpcLWMgmtEnumPerformanceMetrics(
    handle_t        bindingHandle,
    idl_ushort_int  infoLevel,
    LSAMETRICPACK*  pMetricPack
    )
{
    idl_ulong_int  dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pLsaMetricPack = NULL;

    dwError = LsaOpenServer(
                  &hLsaConnection);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LsaGetMetrics(
                  hLsaConnection,
                  infoLevel,
                  &pLsaMetricPack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    switch (infoLevel)
    {
        case 0:
            {
              PLSA_METRIC_PACK_0 pPack = (PLSA_METRIC_PACK_0)pLsaMetricPack;

              pMetricPack->pack0.failedAuthentications =
                            pPack->failedAuthentications; 
              pMetricPack->pack0.failedUserLookupsByName =
                            pPack->failedUserLookupsByName;
              pMetricPack->pack0.failedUserLookupsById =
                            pPack->failedUserLookupsById;
              pMetricPack->pack0.failedGroupLookupsByName =
                            pPack->failedGroupLookupsByName;
              pMetricPack->pack0.failedGroupLookupsById =
                            pPack->failedGroupLookupsById;
              pMetricPack->pack0.failedOpenSession =
                            pPack->failedOpenSession;
              pMetricPack->pack0.failedCloseSession =
                            pPack->failedCloseSession;
              pMetricPack->pack0.failedChangePassword =
                            pPack->failedChangePassword;
              pMetricPack->pack0.unauthorizedAccesses =
                            pPack->unauthorizedAccesses;
 
            }

            break;

        case 1:

            {
              PLSA_METRIC_PACK_1 pPack = (PLSA_METRIC_PACK_1)pLsaMetricPack;

              pMetricPack->pack1.successfulAuthentications =
                            pPack->successfulAuthentications;
              pMetricPack->pack1.failedAuthentications =
                            pPack->failedAuthentications; 
              pMetricPack->pack1.successfulUserLookupsByName =
                            pPack->successfulUserLookupsByName;
              pMetricPack->pack1.failedUserLookupsByName =
                            pPack->failedUserLookupsByName;
              pMetricPack->pack1.successfulUserLookupsById =
                            pPack->successfulUserLookupsById;
              pMetricPack->pack1.failedUserLookupsById =
                            pPack->failedUserLookupsById;
              pMetricPack->pack1.successfulGroupLookupsByName =
                            pPack->successfulGroupLookupsByName;
              pMetricPack->pack1.failedGroupLookupsByName =
                            pPack->failedGroupLookupsByName;
              pMetricPack->pack1.successfulGroupLookupsById =
                            pPack->successfulGroupLookupsById;
              pMetricPack->pack1.failedGroupLookupsById =
                            pPack->failedGroupLookupsById;
              pMetricPack->pack1.successfulOpenSession =
                            pPack->successfulOpenSession;
              pMetricPack->pack1.failedOpenSession =
                            pPack->failedOpenSession;
              pMetricPack->pack1.successfulCloseSession =
                            pPack->successfulCloseSession;
              pMetricPack->pack1.failedCloseSession =
                            pPack->failedCloseSession;
              pMetricPack->pack1.successfulChangePassword =
                            pPack->successfulChangePassword;
              pMetricPack->pack1.failedChangePassword =
                            pPack->failedChangePassword;
              pMetricPack->pack1.unauthorizedAccesses =
                            pPack->unauthorizedAccesses;
           }

           break;
    }

cleanup:

    if (pLsaMetricPack)
    {
        LsaFreeMemory(pLsaMetricPack);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    goto cleanup;
}

idl_long_int
RpcLWMgmtGetStatus(
    handle_t         bindingHandle,
    LWMGMTLSASTATUS* pLsaStatus
    )
{
    idl_ulong_int  dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSASTATUS pLsaOwnedLsaStatus = NULL;
    DWORD iCount = 0;
    LWMGMTLSAAUTHPROVIDERSTATUS* pAuthProviderStatusArray = NULL;

    dwError = LsaOpenServer(
                  &hLsaConnection);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LsaGetStatus(
                  hLsaConnection,
                  &pLsaOwnedLsaStatus);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    if (pLsaOwnedLsaStatus->dwCount)
    {       
        dwError = RPCAllocateMemory(
                        pLsaOwnedLsaStatus->dwCount * sizeof(LWMGMTLSAAUTHPROVIDERSTATUS),
                        (PVOID*)&pAuthProviderStatusArray);
        BAIL_ON_LWMGMT_ERROR(dwError);
        
        for (iCount = 0; iCount < pLsaOwnedLsaStatus->dwCount; iCount++)
        {
            PLSA_AUTH_PROVIDER_STATUS pLsaOwnedAuthProviderStatus =
                &pLsaOwnedLsaStatus->pAuthProviderStatusList[iCount];
            LWMGMTLSAAUTHPROVIDERSTATUS* pAuthProviderStatus =
                &pAuthProviderStatusArray[iCount];
            
            pAuthProviderStatus->mode = pLsaOwnedAuthProviderStatus->mode;
            pAuthProviderStatus->subMode = pLsaOwnedAuthProviderStatus->subMode;
            pAuthProviderStatus->status = pLsaOwnedAuthProviderStatus->status;
            pAuthProviderStatus->dwNetworkCheckInterval =
                pLsaOwnedAuthProviderStatus->dwNetworkCheckInterval;
            
            if (!IsNullOrEmptyString(pLsaOwnedAuthProviderStatus->pszId))
            {
                dwError = RPCAllocateString(
                                pLsaOwnedAuthProviderStatus->pszId,
                                (PSTR*)&pAuthProviderStatus->pszId);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (!IsNullOrEmptyString(pLsaOwnedAuthProviderStatus->pszDomain))
            {
                dwError = RPCAllocateString(
                                pLsaOwnedAuthProviderStatus->pszDomain,
                                (PSTR*)&pAuthProviderStatus->pszDomain);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (!IsNullOrEmptyString(pLsaOwnedAuthProviderStatus->pszForest))
            {
                dwError = RPCAllocateString(
                                pLsaOwnedAuthProviderStatus->pszForest,
                                (PSTR*)&pAuthProviderStatus->pszForest);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (!IsNullOrEmptyString(pLsaOwnedAuthProviderStatus->pszSite))
            {
                dwError = RPCAllocateString(
                                pLsaOwnedAuthProviderStatus->pszSite,
                                (PSTR*)&pAuthProviderStatus->pszSite);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (!IsNullOrEmptyString(pLsaOwnedAuthProviderStatus->pszCell))
            {
                dwError = RPCAllocateString(
                                pLsaOwnedAuthProviderStatus->pszCell,
                                (PSTR*)&pAuthProviderStatus->pszCell);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            
            if (pLsaOwnedAuthProviderStatus->pTrustedDomainInfoArray)
            {
                dwError = LsaServerBuildDomainInfoArray(
                                pLsaOwnedAuthProviderStatus->dwNumTrustedDomains,
                                pLsaOwnedAuthProviderStatus->pTrustedDomainInfoArray,
                                &pAuthProviderStatus->pTrustedDomainInfoArray);
                BAIL_ON_LWMGMT_ERROR(dwError);
                
                pAuthProviderStatus->dwNumTrustedDomains = pLsaOwnedAuthProviderStatus->dwNumTrustedDomains;
            }
        }
    }
    
    pLsaStatus->agentVersion.major = pLsaOwnedLsaStatus->version.dwMajor;
    pLsaStatus->agentVersion.minor = pLsaOwnedLsaStatus->version.dwMinor;
    pLsaStatus->agentVersion.build = pLsaOwnedLsaStatus->version.dwBuild;
    
    pLsaStatus->uptime = pLsaOwnedLsaStatus->dwUptime;
    pLsaStatus->count = pLsaOwnedLsaStatus->dwCount;
    pLsaStatus->pAuthProviderStatusArray = pAuthProviderStatusArray;

cleanup:

    if (pLsaOwnedLsaStatus)
    {
        LsaFreeStatus(pLsaOwnedLsaStatus);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    if (pAuthProviderStatusArray)
    {
        for (iCount = 0; iCount < pLsaOwnedLsaStatus->dwCount; iCount++)
        {
            LWMGMTLSAAUTHPROVIDERSTATUS* pAuthProviderStatus =
                            &pAuthProviderStatusArray[iCount];
            
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
                LsaServerFreeDomainInfoArray(
                                pAuthProviderStatus->dwNumTrustedDomains,
                                pAuthProviderStatus->pTrustedDomainInfoArray);
            }
            
            RPCFreeMemory(pAuthProviderStatusArray);
        }
    }

    goto cleanup;    
}

DWORD
LsaServerBuildDomainInfoArray(
    DWORD                    dwNumTrustedDomains,
    PLSA_TRUSTED_DOMAIN_INFO pSrcDomainInfoArray,
    LWMGMTLSADOMAININFO**    ppDestDomainInfoArray
    )
{
    DWORD dwError = 0;
    DWORD iDomain = 0;
    LWMGMTLSADOMAININFO* pDestDomainInfoArray = NULL;
    
    dwError = RPCAllocateMemory(
                    dwNumTrustedDomains * sizeof(LWMGMTLSADOMAININFO),
                    (PVOID*)&pDestDomainInfoArray);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    for (; iDomain < dwNumTrustedDomains; iDomain++)
    {
        PLSA_TRUSTED_DOMAIN_INFO pSrcDomainInfo =
            &pSrcDomainInfoArray[iDomain];
        
        LWMGMTLSADOMAININFO* pDestDomainInfo =
            &pDestDomainInfoArray[iDomain];
        
        if (!IsNullOrEmptyString(pSrcDomainInfo->pszDnsDomain))
        {
            dwError = RPCAllocateString(
                            pSrcDomainInfo->pszDnsDomain,
                            (PSTR*)&pDestDomainInfo->pszDnsDomain);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        if (!IsNullOrEmptyString(pSrcDomainInfo->pszNetbiosDomain))
        {
            dwError = RPCAllocateString(
                            pSrcDomainInfo->pszNetbiosDomain,
                            (PSTR*)&pDestDomainInfo->pszNetbiosDomain);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        if (!IsNullOrEmptyString(pSrcDomainInfo->pszTrusteeDnsDomain))
        {
            dwError = RPCAllocateString(
                            pSrcDomainInfo->pszTrusteeDnsDomain,
                            (PSTR*)&pDestDomainInfo->pszTrusteeDnsDomain);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        if (!IsNullOrEmptyString(pSrcDomainInfo->pszDomainSID))
        {
            dwError = RPCAllocateString(
                            pSrcDomainInfo->pszDomainSID,
                            (PSTR*)&pDestDomainInfo->pszDomainSID);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        if (!IsNullOrEmptyString(pSrcDomainInfo->pszDomainGUID))
        {
            dwError = RPCAllocateString(
                            pSrcDomainInfo->pszDomainGUID,
                            (PSTR*)&pDestDomainInfo->pszDomainGUID);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        if (!IsNullOrEmptyString(pSrcDomainInfo->pszForestName))
        {
            dwError = RPCAllocateString(
                            pSrcDomainInfo->pszForestName,
                            (PSTR*)&pDestDomainInfo->pszForestName);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        if (!IsNullOrEmptyString(pSrcDomainInfo->pszClientSiteName))
        {
            dwError = RPCAllocateString(
                            pSrcDomainInfo->pszClientSiteName,
                            (PSTR*)&pDestDomainInfo->pszClientSiteName);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        pDestDomainInfo->dwTrustFlags = pSrcDomainInfo->dwTrustFlags;
        pDestDomainInfo->dwTrustType = pSrcDomainInfo->dwTrustType;
        pDestDomainInfo->dwTrustAttributes = pSrcDomainInfo->dwTrustAttributes;
        pDestDomainInfo->dwDomainFlags = pSrcDomainInfo->dwDomainFlags;
        
        if (pSrcDomainInfo->pDCInfo)
        {
            dwError = LsaServerBuildDCInfo(
                            pSrcDomainInfo->pDCInfo,
                            &pDestDomainInfo->pDCInfo);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        
        if (pSrcDomainInfo->pGCInfo)
        {
            dwError = LsaServerBuildDCInfo(
                            pSrcDomainInfo->pGCInfo,
                            &pDestDomainInfo->pGCInfo);
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
    }
    
    *ppDestDomainInfoArray = pDestDomainInfoArray;
    
cleanup:

    return dwError;
    
error:

    *ppDestDomainInfoArray = NULL;
    
    if (pDestDomainInfoArray)
    {
        LsaServerFreeDomainInfoArray(dwNumTrustedDomains, pDestDomainInfoArray);
    }

    goto cleanup;
}

DWORD
LsaServerBuildDCInfo(
    PLSA_DC_INFO      pSrcDCInfo,
    LWMGMTLSADCINFO** ppDCInfo
    )
{
    DWORD dwError = 0;
    LWMGMTLSADCINFO* pDCInfo = NULL;
    
    dwError = RPCAllocateMemory(
                    sizeof(LWMGMTLSADCINFO),
                    (PVOID*)&pDCInfo);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pSrcDCInfo->pszName))
    {
        dwError = RPCAllocateString(
                        pSrcDCInfo->pszName,
                        (PSTR*)&pDCInfo->pszName);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    
    if (!IsNullOrEmptyString(pSrcDCInfo->pszAddress))
    {
        dwError = RPCAllocateString(
                        pSrcDCInfo->pszAddress,
                        (PSTR*)&pDCInfo->pszAddress);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    
    if (!IsNullOrEmptyString(pSrcDCInfo->pszSiteName))
    {
        dwError = RPCAllocateString(
                        pSrcDCInfo->pszSiteName,
                        (PSTR*)&pDCInfo->pszSiteName);
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
        LsaServerFreeDCInfo(pDCInfo);
    }

    goto cleanup;
}

VOID
LsaServerFreeDomainInfoArray(
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
            LsaServerFreeDCInfo(pDomainInfo->pDCInfo);
        }
        
        if (pDomainInfo->pGCInfo)
        {
            LsaServerFreeDCInfo(pDomainInfo->pGCInfo);
        }
    }
    
    RPCFreeMemory(pDomainInfoArray);
}

VOID
LsaServerFreeDCInfo(
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
LsaServerRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    )
{
    unsigned32 dwError = 0;
    rpc_binding_vector_p_t pServerBinding = NULL;
    BOOLEAN bRegistered = FALSE;
    BOOLEAN bBound = FALSE;
    BOOLEAN bEPRegistered = FALSE;

    TRY
    {
        rpc_server_register_if (lsamgr_v1_0_s_ifspec,
                                NULL,
                                NULL,
                                &dwError);
        BAIL_ON_DCE_ERROR(dwError);

        bRegistered = TRUE;
        LWMGMT_LOG_INFO("RPC Service registered successfully.");

        dwError = LsaServerBind(&pServerBinding,
                              lsamgr_v1_0_s_ifspec,
                              (unsigned_char_p_t)"ncacn_ip_tcp",
                              NULL /* endpoint */);
        BAIL_ON_DCE_ERROR(dwError);

        bBound = TRUE;

        rpc_ep_register(lsamgr_v1_0_s_ifspec,
                        pServerBinding,
                        NULL,
                        (idl_char*)pszServiceName,
                        &dwError);
        BAIL_ON_DCE_ERROR(dwError);

        bEPRegistered = TRUE;
        LWMGMT_LOG_INFO("RPC Endpoint registered successfully.");
    }
    CATCH_ALL
    {
        dwError = dcethread_exc_getstatus (THIS_CATCH);
    	if(!dwError)
    	{
                dwError = LWMGMT_ERROR_RPC_EXCEPTION_UPON_REGISTER;
    	}
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

    *ppServerBinding = pServerBinding;

cleanup:

    return dwError;

error:

    LWMGMT_LOG_ERROR("Failed to register RPC endpoint.  Error Code: [%u]\n", dwError);

    if (bBound) {
        unsigned32 tmpStatus = 0;
        rpc_ep_unregister(lsamgr_v1_0_s_ifspec,
                         pServerBinding,
                         NULL,
                         &tmpStatus);
    }

    if (bEPRegistered) {
        unsigned32 tmpStatus = 0;
        rpc_server_unregister_if (lsamgr_v1_0_s_ifspec,
                                NULL,
                                &tmpStatus);
    }

    *ppServerBinding = NULL;

    goto cleanup;
}

DWORD
LsaServerBind(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    unsigned_char_p_t  protocol,
    unsigned_char_p_t  endpoint
    )
{
    unsigned32 status = 0;

    /*
     * Prepare the server binding handle
     * use all avail protocols (UDP and TCP). This basically allocates
     * new sockets for us and associates the interface UUID and
     * object UUID of with those communications endpoints.
     */

    if (!endpoint)
    {
        rpc_server_use_protseq(
                        protocol, 
                        rpc_c_protseq_max_calls_default,
                        &status);
        BAIL_ON_DCE_ERROR(status);
    }
    else
    {
        rpc_server_use_protseq_ep(
                        protocol,
                        rpc_c_protseq_max_calls_default,
                        endpoint,
                        &status);
        BAIL_ON_DCE_ERROR(status);
    }

    rpc_server_inq_bindings(server_binding, &status);
    BAIL_ON_DCE_ERROR(status);

error:

    return status;
}


DWORD
LsaServerUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    )
{
    unsigned32 dwError = 0;

    TRY
    {      
        LWMGMT_LOG_INFO("Unregistering server from the endpoint mapper...");
        rpc_ep_unregister(lsamgr_v1_0_s_ifspec,
                            pServerBinding,
                            NULL,
                            &dwError);
        BAIL_ON_DCE_ERROR(dwError);

        LWMGMT_LOG_INFO("Cleaning up the communications endpoints...");
        rpc_server_unregister_if (lsamgr_v1_0_s_ifspec,
                                 NULL,
                                 &dwError);
        BAIL_ON_DCE_ERROR(dwError);
    }

    CATCH_ALL
    {
        dwError = dcethread_exc_getstatus (THIS_CATCH);
    	if(!dwError)
    	{
                dwError = LWMGMT_ERROR_RPC_EXCEPTION_UPON_UNREGISTER;
    	}
    }
    ENDTRY

    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:
    return dwError;

error:
    LWMGMT_LOG_ERROR("Failed to unregister RPC endpoing.  Error code [%d]\n", dwError);
    goto cleanup;
}
