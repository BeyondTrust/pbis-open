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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 * Eventlog Server API
 *
 */
#include "includes.h"

DWORD
SrvRpcEvtOpen(
    handle_t bindingHandle,
    RPC_LW_EVENTLOG_HANDLE *phEventlog
    )
{
    DWORD dwError = 0;
    DWORD rpcError = 0;
    struct _RPC_LW_EVENTLOG_CONNECTION *pConn = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pConn),
                    (PVOID *)&pConn);

    pConn->pMagic = &SrvRpcEvtOpen;

    TRY
    {
        rpc_binding_inq_access_token_caller(
            bindingHandle,
            &pConn->pUserToken,
            (unsigned32*)&rpcError);
    }
    CATCH_ALL
    {
        rpcError = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY;

    BAIL_ON_DCE_ERROR(dwError, rpcError);

    dwError = EVTCheckAllowed(
                pConn->pUserToken, 
                EVENTLOG_READ_RECORD,
                &pConn->ReadAllowed);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTCheckAllowed(
                pConn->pUserToken, 
                EVENTLOG_WRITE_RECORD,
                &pConn->WriteAllowed);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTCheckAllowed(
                pConn->pUserToken, 
                EVENTLOG_DELETE_RECORD,
                &pConn->DeleteAllowed);
    BAIL_ON_EVT_ERROR(dwError);

    if (!pConn->ReadAllowed &&
        !pConn->WriteAllowed &&
        !pConn->DeleteAllowed)
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_EVT_ERROR(dwError);
    }

    *phEventlog = pConn;

cleanup:
    return dwError;

error:
    *phEventlog = NULL;

    if (pConn)
    {
        if (pConn->pUserToken)
        {
            RtlReleaseAccessToken(&pConn->pUserToken);
        }
        LW_SAFE_FREE_MEMORY(pConn);
    }
    goto cleanup;
}

DWORD
SrvRpcEvtClose(
    RPC_LW_EVENTLOG_HANDLE *phEventlog
    )
{
    DWORD dwError = 0;
    struct _RPC_LW_EVENTLOG_CONNECTION *pConn = NULL;

    if (phEventlog == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }
    pConn = *phEventlog;
    if (pConn == NULL || pConn->pMagic != &SrvRpcEvtOpen)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (pConn->pUserToken)
    {
        RtlReleaseAccessToken(&pConn->pUserToken);
    }
    LW_SAFE_FREE_MEMORY(pConn);

    *phEventlog = NULL;

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
RPC_LW_EVENTLOG_HANDLE_rundown(
    rpc_ss_context_t context_handle
    )
{
    SrvRpcEvtClose((RPC_LW_EVENTLOG_HANDLE*)&context_handle);
}

DWORD
SrvRpcEvtGetRecordCount(
    RPC_LW_EVENTLOG_HANDLE pConn,
    WCHAR * pSqlFilter,
    DWORD * pNumMatched
    )
{
    DWORD  dwError = 0;
    sqlite3 *pDb = NULL;

    if (pConn == NULL || pConn->pMagic != &SrvRpcEvtOpen)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (!pConn->ReadAllowed)
    {
        dwError = ERROR_INVALID_ACCESS;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LwEvtDbOpen(&pDb);
    BAIL_ON_EVT_ERROR(dwError);

    dwError =  LwEvtDbGetRecordCount(
                    pDb,
                    pSqlFilter,
                    pNumMatched);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }

    return dwError;

error:
    *pNumMatched = 0;
    goto cleanup;
}

static
DWORD
RpcAllocate(
    DWORD Len,
    PVOID* ppData
    )
{
    PVOID pData = NULL;
    idl_ulong_int status = 0;

    pData = rpc_sm_allocate(Len, &status);

    *ppData = pData;
    return LwNtStatusToWin32Error(LwRpcStatusToNtStatus(status));
}

static
VOID
RpcFree(
    PVOID pData
    )
{
    idl_ulong_int status = 0;

    rpc_sm_free(pData, &status);

    LW_ASSERT(status == 0);
}

DWORD
SrvRpcEvtReadRecords(
    RPC_LW_EVENTLOG_HANDLE pConn,
    DWORD MaxResults,
    WCHAR * pSqlFilter,
    LW_EVENTLOG_RECORD_LIST *pRecords
    )
{
    DWORD  dwError = 0;
    sqlite3 *pDb = NULL;

    if (pConn == NULL || pConn->pMagic != &SrvRpcEvtOpen)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }
    if (!pConn->ReadAllowed)
    {
        dwError = ERROR_INVALID_ACCESS;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LwEvtDbOpen(&pDb);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbReadRecords(
                    RpcAllocate,
                    RpcFree,
                    pDb,
                    MaxResults,
                    pSqlFilter,
                    &pRecords->Count,
                    &pRecords->pRecords);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }
    return dwError;

error:
    pRecords->Count = 0;
    pRecords->pRecords = NULL;
    goto cleanup;
}


DWORD
SrvRpcEvtWriteRecords(
    RPC_LW_EVENTLOG_HANDLE pConn,
    DWORD Count,
    LW_EVENTLOG_RECORD* pRecords 
    )
{
    DWORD  dwError = 0;
    sqlite3 *pDb = NULL;

    if (pConn == NULL || pConn->pMagic != &SrvRpcEvtOpen)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }
    if (!pConn->WriteAllowed)
    {
        dwError = ERROR_INVALID_ACCESS;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LwEvtDbOpen(&pDb);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbWriteRecords(
                    pDb,
                    Count,
                    pRecords);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
SrvRpcEvtDeleteRecords(
    RPC_LW_EVENTLOG_HANDLE pConn,
    WCHAR *pSqlFilter
    )
{
    DWORD dwError = 0;
    sqlite3 *pDb = NULL;

    if (pConn == NULL || pConn->pMagic != &SrvRpcEvtOpen)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }
    if (!pConn->DeleteAllowed)
    {
        dwError = ERROR_INVALID_ACCESS;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LwEvtDbOpen(&pDb);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbDeleteRecords(
                    pDb,
                    pSqlFilter);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }
    return dwError;

error:
    goto cleanup;
}

static
DWORD
prepare_domain_socket(PCSTR pszPath)
{
    DWORD dwError = 0;
    PSTR pszPathCopy = NULL;
    PSTR pszDirname = NULL;
    PSTR pszBasename = NULL;

    dwError = LwAllocateString(
                    pszPath,
                    &pszPathCopy);
    BAIL_ON_EVT_ERROR(dwError);
    
    pszBasename = strrchr(pszPathCopy, '/');
    
    if (!pszBasename)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    *(pszBasename++) = '\0';

    pszDirname = pszPathCopy;
    
    dwError = LwCreateDirectory(pszDirname, 0655);
    BAIL_ON_EVT_ERROR(dwError);
    
error:
    
    LW_SAFE_FREE_STRING(pszPathCopy);

    return dwError;
}

static
DWORD
bind_server(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    PENDPOINT pEndPoint
    )
{
    DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;

    if (!pEndPoint->endpoint)
    {
        rpc_server_use_protseq((unsigned char *)pEndPoint->protocol, 
                               rpc_c_protseq_max_calls_default,
                               (unsigned32*)&dwRpcStatus);
        BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    }
    else
    {
        if (!strcmp(pEndPoint->protocol, "ncalrpc") &&
            pEndPoint->endpoint[0] == '/')
        {
            dwError = prepare_domain_socket(pEndPoint->endpoint);
            BAIL_ON_EVT_ERROR(dwError);
        }
        
        rpc_server_use_protseq_ep((unsigned char *)pEndPoint->protocol,
                                  rpc_c_protseq_max_calls_default,
                                  (unsigned char *)pEndPoint->endpoint,
                                  (unsigned32*)&dwRpcStatus);
        BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    }

    rpc_server_inq_bindings(server_binding, (unsigned32*)&dwRpcStatus);
    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);

error:

    return dwError;
}

DWORD
EVTRegisterInterface(
    VOID
    )
{
    volatile DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;

    TRY
    {
        rpc_server_register_if (LwEventlog_v1_0_s_ifspec,
                                NULL,
                                NULL,
                                (unsigned32*)&dwRpcStatus);
    }
    CATCH_ALL
    {
        dwRpcStatus = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY;
    
    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_EVT_ERROR(dwError);

error:

    return dwError;
}

DWORD
EVTRegisterEndpoint(
    PSTR pszServiceName,
    PENDPOINT pEndpoint
    )
{
    volatile DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;
    unsigned32 tmpStatus = 0;
    rpc_binding_vector_p_t pServerBinding = NULL;

    TRY
    {
        dwError = bind_server(&pServerBinding,
                              LwEventlog_v1_0_s_ifspec,
                              pEndpoint);
    }
    CATCH_ALL
    {
        dwRpcStatus = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY;

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_EVT_ERROR(dwError);


    TRY
    {
        rpc_ep_register(LwEventlog_v1_0_s_ifspec,
                        pServerBinding,
                        NULL,
                        (idl_char*)pszServiceName,
                        (unsigned32*)&dwRpcStatus);
    }
    CATCH_ALL
    {
        dwRpcStatus = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY;

    if (dwRpcStatus == rpc_s_no_bindings &&
            !pEndpoint->bRegistrationRequired)
    {
        EVT_LOG_WARNING("Unable to register end point %s", pEndpoint->endpoint);
        dwRpcStatus = 0;
    }
    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_INFO("RPC Endpoint registered successfully.");

cleanup:

    if (pServerBinding)
    {
        rpc_binding_vector_free(&pServerBinding, &tmpStatus);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
EVTListen(
    VOID
    )
{
    volatile DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;

    TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, (unsigned32*)&dwRpcStatus);
    }
    CATCH_ALL
    {
        dwRpcStatus = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    return dwError;

error:
    EVT_LOG_ERROR("Failed to begin RPC listening.  Error code [%d]\n", dwError);
    goto cleanup;
}

DWORD
EVTStopListen(
    VOID
    )
{
    volatile DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;

    TRY
    {
        rpc_mgmt_stop_server_listening(NULL, (unsigned32*)&dwRpcStatus);
    }
    CATCH_ALL
    {
        dwRpcStatus = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    return dwError;

error:

    EVT_LOG_ERROR("Failed to stop RPC listening.  Error code [%d]\n", dwError);
    goto cleanup;
}

BOOLEAN
EVTIsListening(
    VOID
    )
{
    volatile DWORD dwError = 0;
    boolean32 bIsListening = FALSE;
    volatile DWORD dwRpcStatus = 0;

    TRY
    {
        bIsListening = rpc_mgmt_is_server_listening(NULL, (unsigned32*)&dwRpcStatus);
    }
    CATCH_ALL
    {
        dwRpcStatus = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    return (BOOLEAN) bIsListening;

error:

    bIsListening = FALSE;

    goto cleanup;
}

DWORD
EVTUnregisterAllEndpoints(
    VOID
    )
{
    volatile DWORD dwError = 0;
    volatile DWORD dwRpcStatus = 0;
    rpc_binding_vector_p_t serverBindings = NULL;

    TRY
    {
        EVT_LOG_INFO("Unregistering server from the endpoint mapper...");
        rpc_server_inq_bindings(&serverBindings, (unsigned32*)&dwRpcStatus);

        if (dwRpcStatus == rpc_s_ok)
        {
            rpc_ep_unregister(LwEventlog_v1_0_s_ifspec,
                              serverBindings,
                              NULL,
                              (unsigned32*)&dwRpcStatus);
        }
    }
    CATCH_ALL
    {
        dwRpcStatus = dcethread_exc_getstatus (THIS_CATCH);
    }
    ENDTRY

    BAIL_ON_DCE_ERROR(dwError, dwRpcStatus);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    if (serverBindings)
    {
        TRY
        {
            rpc_binding_vector_free(&serverBindings, (unsigned32*)&dwRpcStatus);
        }
        CATCH_ALL
        {
        }
        ENDTRY;
    }

    return dwError;

error:

    EVT_LOG_ERROR("Failed to unregister RPC endpoints.  Error code [%d]\n", dwError);

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
