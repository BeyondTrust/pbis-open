/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipc_client.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

static LWNET_CLIENT_CONNECTION_CONTEXT gContext = {0};
static volatile LONG glLibraryRefCount = 0;
#ifndef BROKEN_ONCE_INIT
#if defined(__LWI_SOLARIS__) || defined (__LWI_AIX__)
#define BROKEN_ONCE_INIT 1
#else
#define BROKEN_ONCE_INIT 0
#endif
#endif

#if BROKEN_ONCE_INIT
static pthread_once_t gOnceControl = {PTHREAD_ONCE_INIT};
#else
static pthread_once_t gOnceControl = PTHREAD_ONCE_INIT;
#endif
static DWORD gdwOnceError = 0;

VOID
LWNetOpenServerOnce(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &gContext.pProtocol));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(gContext.pProtocol, LWNetIPCGetProtocolSpec()));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_new(NULL, gContext.pProtocol, &gContext.pClient));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
                                  gContext.pClient, 
                                  LWMSG_ENDPOINT_DIRECT,
                                  "netlogon"));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
                                  gContext.pClient,
                                  LWMSG_ENDPOINT_LOCAL,
                                  LWNET_CACHE_DIR "/" LWNET_SERVER_FILENAME));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_connect(gContext.pClient, &gContext.pSession));
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:

    gdwOnceError = dwError;

    return;

error:

    if (gContext.pClient)
    {
        lwmsg_peer_delete(gContext.pClient);
        gContext.pClient = NULL;
    }
    
    if (gContext.pProtocol)
    {
        lwmsg_protocol_delete(gContext.pProtocol);
        gContext.pProtocol = NULL;
    }
    
    goto cleanup;
}

DWORD
LWNetOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(phConnection);

    pthread_once(&gOnceControl, LWNetOpenServerOnce);
    
    dwError = gdwOnceError;
    BAIL_ON_LWNET_ERROR(dwError);

    *phConnection = (HANDLE) &gContext;

cleanup:

    return dwError;

error:

    if (phConnection) 
    {
        *phConnection = NULL;
    }
    
    goto cleanup;
}

DWORD
LWNetCloseServer(
    HANDLE hConnection
    )
{
    return 0;
}

static
__attribute__((constructor))
VOID
LWNetOpenServerConstructor(
    VOID
    )
{
    LwInterlockedIncrement(&glLibraryRefCount);
}


static
__attribute__((destructor))
VOID
LWNetCloseServerOnce(
    VOID
    )
{
    if (!LwInterlockedDecrement(&glLibraryRefCount))
    {
        if (gContext.pClient)
        {
            lwmsg_peer_delete(gContext.pClient);
        }

        if (gContext.pProtocol)
        {
            lwmsg_protocol_delete(gContext.pProtocol);
        }

        memset(&gContext, 0, sizeof(gContext));
    }
}

DWORD
LWNetAcquireCall(
    HANDLE hConnection,
    LWMsgCall** ppCall
    )
{
    DWORD dwError = 0;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext = hConnection;

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_acquire_call(pContext->pClient, ppCall));
    BAIL_ON_LWNET_ERROR(dwError);
        
error:

    return dwError;
}

DWORD
LWNetTransactGetDCName(
    HANDLE hConnection,
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    PCSTR pszPrimaryDomain,
    DWORD dwFlags,
    DWORD dwBlackListCount,
    PSTR* ppszAddressBlackList,
    PLWNET_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    LWNET_IPC_GET_DC dcReq;
    PLWNET_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcReq.pszServerFQDN = pszServerFQDN;
    dcReq.pszDomainFQDN = pszDomainFQDN;
    dcReq.pszSiteName = pszSiteName;
    dcReq.pszPrimaryDomain = pszPrimaryDomain;
    dcReq.dwFlags = dwFlags;
    dcReq.dwBlackListCount = dwBlackListCount;
    dcReq.ppszAddressBlackList = ppszAddressBlackList;

    in.tag = LWNET_Q_GET_DC_NAME;
    in.data = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (out.tag)
    {
    case LWNET_R_GET_DC_NAME:
        *ppDCInfo = (PLWNET_DC_INFO) out.data;
        out.data = NULL;
        break;
    case LWNET_R_ERROR:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (ppDCInfo)
    {
        *ppDCInfo = NULL;
    }
                    
    goto cleanup;
}

DWORD
LWNetTransactGetDCList(
    IN HANDLE hConnection,
    IN PCSTR pszDomainFQDN,
    IN PCSTR pszSiteName,
    IN DWORD dwFlags,
    OUT PLWNET_DC_ADDRESS* ppDcList,
    OUT LW_PDWORD pdwDcCount
    )
{
    DWORD dwError = 0;
    LWNET_IPC_GET_DC dcReq = { 0 };
    PLWNET_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcReq.pszDomainFQDN = pszDomainFQDN;
    dcReq.pszSiteName = pszSiteName;
    dcReq.dwFlags = dwFlags;

    in.tag = LWNET_Q_GET_DC_LIST;
    in.data = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (out.tag)
    {
    case LWNET_R_GET_DC_LIST:
    {
        PLWNET_IPC_DC_LIST pResult = (PLWNET_IPC_DC_LIST) out.data;
        *ppDcList = pResult->pDcList;
        pResult->pDcList = NULL;
        *pdwDcCount = pResult->dwDcCount;
        break;
    }
    case LWNET_R_ERROR:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *ppDcList = NULL;
    *pdwDcCount = 0;
                    
    goto cleanup;

}

DWORD
LWNetTransactGetDCTime(
    HANDLE hConnection,
    PCSTR pszDomainFQDN,
    PLWNET_UNIX_TIME_T pDCTime
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_IPC_CONST_STRING dcTimeReq;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcTimeReq.pszString = pszDomainFQDN;
    in.tag = LWNET_Q_GET_DC_TIME;
    in.data = &dcTimeReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (out.tag)
    {
    case LWNET_R_GET_DC_TIME:
        *pDCTime = ((PLWNET_IPC_TIME) out.data)->Time;
        break;
    case LWNET_R_ERROR:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetTransactGetDomainController(
    HANDLE hConnection,
    PCSTR pszDomainFQDN,
    PSTR* ppszDomainControllerFQDN
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_IPC_CONST_STRING dcReq;
    PLWNET_IPC_STRING dcRes = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcReq.pszString = pszDomainFQDN;
    in.tag = LWNET_Q_GET_DOMAIN_CONTROLLER;
    in.data = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (out.tag)
    {
    case LWNET_R_GET_DOMAIN_CONTROLLER:
        dcRes = out.data;
        *ppszDomainControllerFQDN = dcRes->pszString;
        /* NULL out the field so it does not get freed */
        dcRes->pszString = NULL;
        break;
    case LWNET_R_ERROR:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetTransactResolveName(
    LW_IN HANDLE hConnection,
    LW_IN LW_PCWSTR pcwszHostName,
    LW_OUT LW_PWSTR *ppwszCanonName,
    LW_OUT PLWNET_RESOLVE_ADDR **pppAddressList,
    LW_OUT PDWORD pdwAddressListLen)
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_RESOLVE_NAME_ADDRESS inHostName = {0};
    PLWNET_RESOLVE_ADDR *ppAddressList = NULL;
    DWORD dwAddressListLen = 0;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    
    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    inHostName.pwszHostName = (PWSTR) pcwszHostName;

    in.tag = LWNET_Q_RESOLVE_NAME;
    in.data = &inHostName;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);

    switch (out.tag)
    {
    case LWNET_R_RESOLVE_NAME:
    {
        PLWNET_RESOLVE_NAME_ADDRESS_RESPONSE pResponse = 
            (PLWNET_RESOLVE_NAME_ADDRESS_RESPONSE) out.data;
        ppAddressList = pResponse->ppAddressList;
        dwAddressListLen = pResponse->dwAddressListLen;

        *ppwszCanonName = pResponse->pwszCanonName;
        // Returned to caller, NULL to prevent freeing by lwmsg
        pResponse->ppAddressList = NULL;
        pResponse->pwszCanonName = NULL;
        break;
    }
    case LWNET_R_ERROR:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    *pppAddressList = ppAddressList;
    *pdwAddressListLen = dwAddressListLen;

    return dwError;

error:
    goto cleanup;
}
