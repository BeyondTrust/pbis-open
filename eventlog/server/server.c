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



idl_long_int
RpcLWIOpenEventLog(
    handle_t bindingHandle,
    idl_char * pszServerName,
    idl_char * pszEventLog
    )
{
    DWORD dwError = 0;

    dwError = LWICheckSecurity(
                  bindingHandle,
                  EVENTLOG_READ_RECORD);

    EVTUnlockServerInfo();

    if ( dwError )
    {
        dwError = LWICheckSecurity(
                      bindingHandle,
                      EVENTLOG_WRITE_RECORD);

        EVTUnlockServerInfo();
    }

    if ( dwError )
    {
        dwError = LWICheckSecurity(
                      bindingHandle,
                      EVENTLOG_DELETE_RECORD);

        EVTUnlockServerInfo();
    }

    return dwError;
}

idl_long_int
RpcLWICloseEventLog(handle_t bindingHandle)
{
    DWORD     dwError = 0;

    /* We support the notion of a connect, but we currently don't do much here */
    /* Someday RPC connection cleanup  may be required here */

    return dwError;
}

idl_long_int
RpcLWIEventLogCount(
    handle_t bindingHandle,
    idl_char* sqlFilter,
    unsigned32* pdwNumMatched
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    dwError = LWICheckSecurity(
                  bindingHandle,
                  EVENTLOG_READ_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_EVT_ERROR(dwError);

    dwError =  SrvEventLogCount(
                    hDB,
                    (PSTR) sqlFilter,
                    (PDWORD)pdwNumMatched);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    return dwError;

error:

    goto cleanup;
}

idl_long_int
RpcLWIReadEventLog(
    handle_t    bindingHandle,
    unsigned32  startingRowId,
    unsigned32  nRecordsPerPage,
    idl_char*   sqlFilter,
    unsigned32* pdwNumReturned,
    EVENT_LOG_RECORD* eventRecords
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    dwError = LWICheckSecurity(
                  bindingHandle,
                  EVENTLOG_READ_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = SrvReadEventLog(
                    hDB,
                    startingRowId,
                    nRecordsPerPage,
                    (PSTR) sqlFilter,
                    (PDWORD)pdwNumReturned,
                    &eventRecords);

    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    return dwError;

error:

    *pdwNumReturned = 0;

    goto cleanup;
}


idl_long_int
RpcLWIWriteEventLogRecords(
    handle_t bindingHandle,
    unsigned32 cRecords,
    EVENT_LOG_RECORD* pEventRecords 
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    dwError = LWICheckSecurity(
                  bindingHandle,
                  EVENTLOG_WRITE_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = SrvWriteToDB(
                    hDB,
                    cRecords,
                    pEventRecords);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    return dwError;

error:

    goto cleanup;
}

idl_long_int
RpcLWIClearEventLog(handle_t bindingHandle)
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    dwError = LWICheckSecurity(
                  bindingHandle,
                  EVENTLOG_DELETE_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = SrvClearEventLog(hDB);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    return dwError;

error:

    goto cleanup;
}


idl_long_int
RpcLWIDeleteFromEventLog(
    handle_t bindingHandle,
    idl_char* sqlFilter
    )
{
    DWORD  dwError = 0;
    HANDLE hDB = (HANDLE)NULL;

    dwError = LWICheckSecurity(
                  bindingHandle,
                  EVENTLOG_DELETE_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError =  SrvOpenEventDatabase(&hDB);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = SrvDeleteFromEventLog(
                    hDB,
                    (PSTR) sqlFilter);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
        SrvCloseEventDatabase(hDB);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
mkdir_recursive(PSTR pszPath, mode_t mode)
{
    DWORD dwError = 0;
    struct stat statbuf;
    PSTR pszSlash = NULL;

    for(pszSlash = strchr(pszPath, '/'); pszSlash; pszSlash = strchr(pszSlash + 1, '/'))
    {
        if (pszSlash == pszPath)
        {
            continue;
        }

        *pszSlash = '\0';

        if (stat(pszPath, &statbuf) == 0)
        {
            /* Make sure its a directory */
            if (!S_ISDIR(statbuf.st_mode))
            {
                dwError = ENOENT;
                BAIL_ON_EVT_ERROR(dwError);
            }
        }
        else
        {
            /* Create it */
            if (mkdir(pszPath, mode))
            {
                dwError = errno;
                BAIL_ON_EVT_ERROR(dwError);
            }
        }

        *pszSlash = '/';
    }

    if (stat(pszPath, &statbuf) == 0)
    {
        /* Make sure its a directory */
        if (!S_ISDIR(statbuf.st_mode))
        {
            dwError = ENOENT;
            BAIL_ON_EVT_ERROR(dwError);
            }
    }
    else
    {
        /* Create it */
        if (mkdir(pszPath, mode))
        {
                dwError = errno;
                BAIL_ON_EVT_ERROR(dwError);
        }
    }  
    
error:
    if (pszSlash)
        *pszSlash = '/';

    return dwError;
}

static
DWORD
prepare_domain_socket(PCSTR pszPath)
{
    DWORD dwError = 0;
    PSTR pszPathCopy = NULL;
    PSTR pszDirname = NULL;
    PSTR pszBasename = NULL;

    pszPathCopy = strdup(pszPath);
    if (!pszPathCopy)
    {
        dwError = ENOMEM;
        BAIL_ON_EVT_ERROR(dwError);
    }
    
    pszBasename = strrchr(pszPathCopy, '/');
    
    if (!pszBasename)
    {
        dwError = EINVAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

    *(pszBasename++) = '\0';

    pszDirname = pszPathCopy;
    
    dwError = mkdir_recursive(pszDirname, 0655);
    BAIL_ON_EVT_ERROR(dwError);
    
error:
    
    if (pszPathCopy)
        free(pszPathCopy);

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
        rpc_server_register_if (eventlog_v1_0_s_ifspec,
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
                              eventlog_v1_0_s_ifspec,
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
        rpc_ep_register(eventlog_v1_0_s_ifspec,
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
            rpc_ep_unregister(eventlog_v1_0_s_ifspec,
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
