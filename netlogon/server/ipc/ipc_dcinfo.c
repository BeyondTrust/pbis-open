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
 *        ipc_dcinfo.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Inter-process communication (Server) API for querying DC Info
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

static DWORD
LWNetSrvIpcCreateError(
    DWORD dwErrorCode,
    PCSTR pszErrorMessage,
    PLWNET_IPC_ERROR* ppError
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetAllocateMemory(sizeof(*pError), (void**) (void*) &pError);
    BAIL_ON_LWNET_ERROR(dwError);

    if (pszErrorMessage)
    {
        dwError = LWNetAllocateString(pszErrorMessage, (PSTR*) &pError->pszErrorMessage);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    pError->dwError = dwErrorCode;

    *ppError = pError;

error:

    return dwError;
}

LWMsgStatus
LWNetSrvIpcSetLogLevel(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_LOG_INFO pReq = pIn->data;
    PLWNET_IPC_ERROR pError = NULL;
    LWMsgSecurityToken* pToken = NULL;
    uid_t uid = 0;
    gid_t gid = 0;

    pToken = lwmsg_session_get_peer_security_token(lwmsg_call_get_session(pCall));
    if (!pToken || strcmp(lwmsg_security_token_get_type(pToken), "local"))
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(pToken, &uid, &gid));
    BAIL_ON_LWNET_ERROR(dwError);

    if (uid != 0 )
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = lwnet_set_log_level(pReq->LogLevel);
    if (!dwError)
    {
        pOut->tag = LWNET_R_SET_LOG_LEVEL;
        pOut->data = NULL;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_LWNET_ERROR(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LWNetSrvIpcGetLogInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_LOG_LEVEL LogLevel = 0;
    LWNET_LOG_TARGET LogTarget = 0;
    PSTR pszLogPath = NULL;
    LWMsgSecurityToken* pToken = NULL;
    uid_t uid = 0;
    gid_t gid = 0;

    pToken = lwmsg_session_get_peer_security_token(lwmsg_call_get_session(pCall));
    if (!pToken || strcmp(lwmsg_security_token_get_type(pToken), "local"))
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(pToken, &uid, &gid));
    BAIL_ON_LWNET_ERROR(dwError);

    if (uid != 0 )
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = lwnet_get_log_info(&LogLevel, &LogTarget, &pszLogPath);
    if (!dwError)
    {
        PLWNET_IPC_LOG_INFO pRes = NULL;

        dwError = LWNetAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        BAIL_ON_LWNET_ERROR(dwError);

        pRes->LogLevel = LogLevel;
        pRes->LogTarget = LogTarget;
        pRes->pszLogPath = pszLogPath;
        pszLogPath = NULL;

        pOut->tag = LWNET_R_GET_LOG_INFO;
        pOut->data = pRes;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    LWNET_SAFE_FREE_STRING(pszLogPath);

    return MAP_LWNET_ERROR(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LWNetSrvIpcGetDCName(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDCInfo = NULL;
    PLWNET_IPC_GET_DC pReq = pIn->data;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetSrvGetDCName(
        pReq->pszServerFQDN,
        pReq->pszDomainFQDN,
        pReq->pszSiteName,
        pReq->pszPrimaryDomain,
        pReq->dwFlags,
        pReq->dwBlackListCount,
        pReq->ppszAddressBlackList,
        &pDCInfo);

    if (!dwError)
    {
        pOut->tag = LWNET_R_GET_DC_NAME;
        pOut->data = pDCInfo;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    return MAP_LWNET_ERROR(dwError);

error:

    if(pDCInfo != NULL)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    goto cleanup;
}

LWMsgStatus
LWNetSrvIpcGetDCList(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_DC_ADDRESS pDcList = NULL;
    DWORD dwDcCount = 0;
    PLWNET_IPC_GET_DC pReq = pIn->data;

    dwError = LWNetSrvGetDCList(
                    pReq->pszDomainFQDN,
                    pReq->pszSiteName,
                    pReq->dwFlags,
                    &pDcList,
                    &dwDcCount);
    if (!dwError)
    {
        PLWNET_IPC_DC_LIST pRes = NULL;

        dwError = LWNetAllocateMemory(sizeof(*pRes), (PVOID*)&pRes);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_GET_DC_LIST;
        pOut->data = pRes;
        pRes->pDcList = pDcList;
        pDcList = NULL;
        pRes->dwDcCount = dwDcCount;
    }
    else
    {
        PLWNET_IPC_ERROR pError = NULL;

        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_ERROR;;
        pOut->data = pError;
    }

cleanup:

    return MAP_LWNET_ERROR(dwError);

error:

    if (pDcList)
    {
        LWNetFreeDCList(pDcList, dwDcCount);
    }

    goto cleanup;
}

DWORD
LWNetSrvIpcGetDCTime(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_CONST_STRING pReq = pIn->data;
    PLWNET_IPC_TIME pRes = NULL;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvGetDCTime(
        pReq->pszString,
        &pRes->Time);

    if (!dwError)
    {
        pOut->tag = LWNET_R_GET_DC_TIME;
        pOut->data = pRes;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    /* If we are returning an error, free the response structure we allocated */
    if (pError && pRes)
    {
        LWNetFreeMemory(pRes);
    }

    return MAP_LWNET_ERROR(dwError);

error:

    goto cleanup;
}

DWORD
LWNetSrvIpcGetDomainController(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_CONST_STRING pReq = pIn->data;
    PLWNET_IPC_STRING pRes = NULL;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvGetDomainController(
        pReq->pszString,
        &pRes->pszString
        );

    if (!dwError)
    {
        pOut->tag = LWNET_R_GET_DOMAIN_CONTROLLER;
        pOut->data = pRes;

    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    /* If we are responding with an error, free the response structure we allocated */
    if (pError && pRes)
    {
        LWNetFreeMemory(pRes);
    }

    return MAP_LWNET_ERROR(dwError);

error:

    LWNET_SAFE_FREE_MEMORY(pRes);

    goto cleanup;
}

DWORD
LWNetSrvIpcGetCurrentDomain(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_STRING pRes = NULL;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvGetCurrentDomain(
        &pRes->pszString
        );

    if (!dwError)
    {
        pOut->tag = LWNET_R_GET_CURRENT_DOMAIN;
        pOut->data = pRes;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);

        pOut->tag = LWNET_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    if (pRes && pError)
    {
        LWNetFreeMemory(pRes);
    }

    return MAP_LWNET_ERROR(dwError);

error:

    LWNET_SAFE_FREE_MEMORY(pRes);

    goto cleanup;
}

static DWORD
LWNetSrvIpcResolveNetBiosName(
    PSTR pszHostName,
    LWMsgParams* pOut)
{
    struct in_addr *nbAddrs = {0};
    DWORD nbAddrsLen = 0;
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD nbAddrsAlloc = 0;
    PLWNET_RESOLVE_NAME_ADDRESS_RESPONSE pRes = NULL;
    PLWNET_RESOLVE_ADDR *ppResAddr = NULL;
    PLWNET_RESOLVE_ADDR pResAddr = NULL;
    PWSTR pwszCanonName = NULL;

    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetNbResolveName(
                  pszHostName,
                  LWNB_NETBIOS_FLAGS_RESOLVE_FILE_SERVICE |
                      LWNB_NETBIOS_FLAGS_MODE_HYBRID,
                  &nbAddrs,
                  &nbAddrsLen);
    if (dwError)
    {
        dwError = LWNetNbResolveName(
                      pszHostName,
                      LWNB_NETBIOS_FLAGS_RESOLVE_DC |
                          LWNB_NETBIOS_FLAGS_MODE_HYBRID,
                      &nbAddrs,
                      &nbAddrsLen);
    }
    if (dwError)
    {
        dwError = ERROR_BAD_NET_NAME;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateMemory(sizeof(*ppResAddr) * nbAddrsLen,
                                  (void*) &ppResAddr);
    BAIL_ON_LWNET_ERROR(dwError);
    for (i=0; i<nbAddrsLen; i++)
    {
        dwError = LWNetAllocateMemory(sizeof(*pResAddr),
                                      (void*) &pResAddr);
        BAIL_ON_LWNET_ERROR(dwError);

        pResAddr->AddressType = LWNET_IP_ADDR_V4;
        memcpy(pResAddr->Address.Ip4Addr,
               &nbAddrs[i],
               4);
        ppResAddr[i] = pResAddr;
        nbAddrsAlloc = i;
    }

    dwError = LwRtlWC16StringAllocateFromCString(
                  &pwszCanonName,
                  pszHostName);
    BAIL_ON_LWNET_ERROR(dwError);

    pRes->pwszCanonName = pwszCanonName; // Return value requested
    pRes->ppAddressList = ppResAddr;
    pRes->dwAddressListLen = nbAddrsLen;
    pOut->tag = LWNET_R_RESOLVE_NAME;
    pOut->data = pRes;

cleanup:
    LWNetNbAddressListFree(nbAddrs);
    return dwError;

error:
    for (i=0; i<nbAddrsAlloc; i++)
    {
        LWNET_SAFE_FREE_MEMORY(ppResAddr[i]);
    }
    LWNET_SAFE_FREE_MEMORY(ppResAddr);
    LWNET_SAFE_FREE_MEMORY(pRes);
    goto cleanup;
}

static DWORD
LWNetSrvIpcResolveDnsName(
    PSTR pszHostName,
    LWMsgParams* pOut)
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    PWSTR pwszCanonName = NULL;
    PLWNET_RESOLVE_ADDR *ppResAddr = NULL;
    PLWNET_RESOLVE_ADDR pResAddr = NULL;
    DWORD dwResAddrLen = 0;
    PLWNET_RESOLVE_NAME_ADDRESS_RESPONSE pRes = NULL;
    DWORD i = 0;
    int addrinfoResult = 0;
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *rp = NULL;

    /*
     * Resolve pszHostName using DNS then NetBIOS
     */
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags  = AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    /* Resolve using DNS */
    addrinfoResult = getaddrinfo(pszHostName, NULL, &hints, &result);
    if (addrinfoResult == 0)
    {
        dwError = LwRtlWC16StringAllocateFromCString(
                      &pwszCanonName, result->ai_canonname);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else
    {
        dwError = ERROR_BAD_NET_NAME;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);

    for (dwResAddrLen = 0, rp = result; rp; rp = rp->ai_next)
    {
        dwResAddrLen++;
    }

    dwError = LWNetAllocateMemory(sizeof(*ppResAddr) * dwResAddrLen,
                                  (void*) &ppResAddr);
    BAIL_ON_LWNET_ERROR(dwError);
    for (i = 0, rp = result; rp; rp = rp->ai_next, i++)
    {
        dwError = LWNetAllocateMemory(sizeof(*pResAddr),
                                      (void*) &pResAddr);
        BAIL_ON_LWNET_ERROR(dwError);

        if (rp->ai_family == PF_INET)
        {
            pResAddr->AddressType = LWNET_IP_ADDR_V4;
            memcpy(pResAddr->Address.Ip4Addr,
                   &((struct sockaddr_in *)rp->ai_addr)->sin_addr,
                   4);
        }
        else if (rp->ai_family == PF_INET6)
        {
            pResAddr->AddressType = LWNET_IP_ADDR_V6;
            memcpy(pResAddr->Address.Ip6Addr,
                   &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr,
                   16);
        }
        ppResAddr[i] = pResAddr;
    }

    pRes->pwszCanonName = pwszCanonName;
    pRes->ppAddressList = ppResAddr;
    pRes->dwAddressListLen = dwResAddrLen;
    pOut->tag = LWNET_R_RESOLVE_NAME;
    pOut->data = pRes;

cleanup:
    freeaddrinfo(result);

    if (pError)
    {
        pOut->data = pError;
        pOut->tag = LWNET_R_ERROR;
    }

    return MAP_LWNET_ERROR(dwError);

error:
    LWNET_SAFE_FREE_MEMORY(pwszCanonName);
    if (ppResAddr)
    {
        for (i=0; ppResAddr[i]; i++)
        {
            LWNET_SAFE_FREE_MEMORY(ppResAddr[i]);
        }
        LWNET_SAFE_FREE_MEMORY(ppResAddr);
    }
    LWNET_SAFE_FREE_MEMORY(pRes);

    goto cleanup;
}


DWORD
LWNetSrvIpcResolveName(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    LWNET_RESOLVE_NAME_ADDRESS *pReq = pIn->data;
    PLWNET_IPC_ERROR pError = NULL;
    PWSTR pwszHostName = NULL;
    PSTR pszHostName = NULL;
    PWSTR pwszCanonName = NULL;

    /* Convert hostname to resolve from WC to C string */
    pwszHostName = pReq->pwszHostName;
    dwError = LwRtlCStringAllocateFromWC16String(&pszHostName, pwszHostName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvIpcResolveDnsName(
                  pszHostName,
                  pOut);
    if (dwError)
    {
        dwError = LWNetSrvIpcResolveNetBiosName(
                  pszHostName,
                  pOut);
    }
    if (dwError)
    {
        dwError = LWNetSrvIpcCreateError(dwError, "DNS Lookup Failed", &pError);
    }

cleanup:
    LWNET_SAFE_FREE_STRING(pszHostName);

    if (pError)
    {
        pOut->data = pError;
        pOut->tag = LWNET_R_ERROR;
    }

    return MAP_LWNET_ERROR(dwError);

error:
    LWNET_SAFE_FREE_MEMORY(pwszCanonName);
    goto cleanup;
}
