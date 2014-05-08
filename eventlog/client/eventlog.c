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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen <kstemen@likewise.com>
 *
 * Eventlog Client API
 *
 */
#include "includes.h"

VOID
LwEvtFreeRecord(
    IN PLW_EVENTLOG_RECORD pRecord
    )
{
    LwEvtFreeRecordArray(1, pRecord);
}

VOID
LwEvtFreeRecordArray(
    IN DWORD Count,
    IN PLW_EVENTLOG_RECORD pRecords
    )
{
    DWORD index = 0;

    if (pRecords)
    {
        for (index = 0; index < Count; index++)
        {
            LW_SAFE_FREE_MEMORY(pRecords[index].pLogname);
            LW_SAFE_FREE_MEMORY(pRecords[index].pEventType);
            LW_SAFE_FREE_MEMORY(pRecords[index].pEventSource);
            LW_SAFE_FREE_MEMORY(pRecords[index].pEventCategory);
            LW_SAFE_FREE_MEMORY(pRecords[index].pUser);
            LW_SAFE_FREE_MEMORY(pRecords[index].pComputer);
            LW_SAFE_FREE_MEMORY(pRecords[index].pDescription);
            LW_SAFE_FREE_MEMORY(pRecords[index].pData);
        }

        LW_SAFE_FREE_MEMORY(pRecords);
    }
}

static
BOOLEAN
LwEvtIsLocalHost(
    const char * hostname
    )
{
    DWORD            dwError = 0;
    BOOLEAN          bResult = FALSE;
    char             localHost[256] = {0};
    struct addrinfo* localInfo = NULL;
    struct addrinfo* remoteInfo = NULL;
    PCSTR            pcszLocalHost = NULL;
    PCSTR            pcszRemoteHost = NULL;
    CHAR             canonNameLocal[NI_MAXHOST] = "";
    CHAR             canonNameRemote[NI_MAXHOST] = "";

    memset(localHost, 0, sizeof(localHost));

    if ( !strcasecmp(hostname, "localhost") ||
         !strcmp(hostname, "127.0.0.1") )
    {
        bResult = TRUE;
        goto cleanup;
    }

    dwError = gethostname(localHost, sizeof(localHost) - 1);
    if (!LW_IS_EMPTY_STR(localHost))
    {
        dwError = getaddrinfo(localHost, NULL, NULL, &localInfo);
        if ( dwError )
        {
            pcszLocalHost = localHost;
        }
        else
        {
            dwError = getnameinfo(localInfo->ai_addr, localInfo->ai_addrlen, canonNameLocal, NI_MAXHOST, NULL, 0, 0);

            if(dwError || !canonNameLocal[0]) {
                pcszLocalHost = localHost;
            }
            else {
                pcszLocalHost = canonNameLocal;
            }
        }

        dwError = getaddrinfo(hostname, NULL, NULL, &remoteInfo);
        if ( dwError )
        {
            pcszRemoteHost = hostname;
        }
        else
        {
            dwError = getnameinfo(remoteInfo->ai_addr, remoteInfo->ai_addrlen, canonNameRemote, NI_MAXHOST, NULL, 0, 0);

            if(dwError || !canonNameRemote[0]) {
                pcszRemoteHost = hostname;
            }
            else {
                pcszRemoteHost = canonNameRemote;
            }
        }

        if ( !strcasecmp(pcszLocalHost, pcszRemoteHost) )
        {
            bResult = TRUE;
        }
    }

cleanup:

    if (localInfo)
    {
        freeaddrinfo(localInfo);
    }
    if (remoteInfo)
    {
        freeaddrinfo(remoteInfo);
    }

    return bResult;
}

DWORD
LwEvtOpenEventlog(
    IN OPTIONAL PCSTR pServerName,
    OUT PLW_EVENTLOG_CONNECTION* ppConn
    )
{
    volatile DWORD dwError = 0;
    PLW_EVENTLOG_CONNECTION pConn = NULL;
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    handle_t eventBindingLocal = 0;
#endif

    EVT_LOG_VERBOSE("client::eventlog.c OpenEventlog server=%s)\n",
            LW_SAFE_LOG_STRING(pServerName));

    dwError = LwAllocateMemory(sizeof(*pConn), (PVOID*) &pConn);
    BAIL_ON_EVT_ERROR(dwError);

    if (pServerName == NULL || LwEvtIsLocalHost(pServerName))
    {
        // If no host is specified, connect via lwmsg
        dwError = LwmEvtOpenServer(&pConn->Local);
        BAIL_ON_EVT_ERROR(dwError);
    }
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    else
    {
        TRY
        {
            dwError = LwEvtCreateEventlogRpcBinding(pServerName,
                                                  &eventBindingLocal);
        }
        CATCH_ALL
        {
            dwError = EVTGetRpcError(THIS_CATCH);
        }
        ENDTRY

        BAIL_ON_EVT_ERROR(dwError);

        TRY
        {
            dwError = RpcEvtOpen(eventBindingLocal,
                                    &pConn->Remote);
        }
        CATCH (rpc_x_auth_method)
        {
            dwError = ERROR_ACCESS_DENIED;
        }
        CATCH_ALL
        {
            dwError = EVTGetRpcError(THIS_CATCH);
        }
        ENDTRY

        BAIL_ON_EVT_ERROR(dwError);
    }
#endif

    *ppConn = pConn;

cleanup:
    return dwError;

error:
    if (pConn)
    {
        LwEvtCloseEventlog(pConn);
    }

#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    if (eventBindingLocal)
    {
        LwEvtFreeEventlogRpcBinding(eventBindingLocal);
    }
#endif

    *ppConn = NULL;
    goto cleanup;
}

DWORD
LwEvtCloseEventlog(
    IN PLW_EVENTLOG_CONNECTION pConn
    )
{
    volatile DWORD dwError = 0;

    if (pConn == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (pConn->Local)
    {
        dwError = LwmEvtCloseServer(pConn->Local);
        BAIL_ON_EVT_ERROR(dwError);
    }
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    if (pConn->Remote)
    {
        TRY
        {
            dwError = RpcEvtClose(&pConn->Remote);
        }
        CATCH_ALL
        {
            dwError = EVTGetRpcError(THIS_CATCH);
        }
        ENDTRY;

        BAIL_ON_EVT_ERROR(dwError);
    }
#endif

cleanup:
    if (pConn)
    {
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
        if (pConn->Remote != NULL)
        {
            RpcSsDestroyClientContext(&pConn->Remote);
        }
#endif
        LW_SAFE_FREE_MEMORY(pConn);
    }

    return dwError;

error:

    EVT_LOG_ERROR("Failed to close event log. Error code [%d]\n", dwError);
    goto cleanup;
}

#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
static
idl_void_p_t
LwEvtRpcAllocateMemory(
    idl_size_t Size
    )
{
    idl_void_p_t pResult = NULL;
    if ((idl_size_t)(DWORD)Size != Size)
    {
        // Overflow
        return NULL;
    }

    if (LwAllocateMemory((DWORD)Size, &pResult))
    {
        // Error
        return NULL;
    }

    return pResult;
}

static
VOID
LwEvtRpcFreeMemory(
    idl_void_p_t pMem
    )
{
    LwFreeMemory(pMem);
}
#endif

DWORD
LwEvtReadRecords(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN DWORD MaxResults,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pCount,
    OUT PLW_EVENTLOG_RECORD* ppRecords
    )
{
    volatile DWORD dwError = 0;
    LW_EVENTLOG_RECORD_LIST records = { 0 };
    // These are function pointers initialized to NULL

    if (pConn->Local)
    {
        dwError = LwmEvtReadRecords(
                        pConn->Local,
                        MaxResults,
                        pSqlFilter,
                        &records.Count,
                        &records.pRecords);
        BAIL_ON_EVT_ERROR(dwError);
    }
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    else
    {
        idl_void_p_t (*pOldMalloc)(idl_size_t) = NULL;
        void (*pOldFree)(idl_void_p_t) = NULL;
        TRY
        {
            rpc_ss_swap_client_alloc_free(
                        LwEvtRpcAllocateMemory,
                        LwEvtRpcFreeMemory,
                        &pOldMalloc,
                        &pOldFree);

            dwError = RpcEvtReadRecords(
                        pConn->Remote,
                        MaxResults, 
                        (PWSTR)pSqlFilter, 
                        &records);
        }
        CATCH_ALL
        {
            dwError = EVTGetRpcError(THIS_CATCH);
        }
        FINALLY
        {
            rpc_ss_set_client_alloc_free(
                        pOldMalloc,
                        pOldFree);
        }
        ENDTRY;

        BAIL_ON_EVT_ERROR(dwError);
    }
#endif

    *pCount = records.Count;
    *ppRecords = records.pRecords;

cleanup:
    return dwError;

error:
    EVT_LOG_ERROR("Failed to read event log. Error code [%d]\n", dwError);

    LwEvtFreeRecordArray(records.Count, records.pRecords);
    *pCount = 0;
    *ppRecords = NULL;
    goto cleanup;
}


DWORD
LwEvtGetRecordCount(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pNumMatched
    )
{
    volatile DWORD dwError = 0;
    DWORD numMatched = 0;

    if (pConn->Local)
    {
        dwError = LwmEvtGetRecordCount(
                        pConn->Local,
                        pSqlFilter,
                        &numMatched);
        BAIL_ON_EVT_ERROR(dwError);
    }
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    else
    {
        TRY
        {
            dwError = RpcEvtGetRecordCount(
                        pConn->Remote,
                        (PWSTR)pSqlFilter, 
                        &numMatched);
        }
        CATCH_ALL
        {
            dwError = EVTGetRpcError(THIS_CATCH);
        }
        ENDTRY;

        BAIL_ON_EVT_ERROR(dwError);
    }
#endif

    *pNumMatched = numMatched;

cleanup:
    return dwError;

error:
    EVT_LOG_ERROR("Failed to get record count. Error code [%d]\n", dwError);

    *pNumMatched = 0;
    goto cleanup;
}


DWORD
LwEvtWriteRecords(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN DWORD Count,
    IN PLW_EVENTLOG_RECORD pRecords 
    )
{
    volatile DWORD dwError = 0;
    char pszHostname[1024];
    PWSTR pwszHostname = NULL;
    DWORD index = 0;

    for (index = 0; index < Count; index++)
    {
        if (pRecords[index].pComputer == NULL)
        {
            if (!pwszHostname)
            {
                dwError = LwMapErrnoToLwError(
                            gethostname(pszHostname, sizeof(pszHostname)));
                BAIL_ON_EVT_ERROR(dwError);

                dwError = LwMbsToWc16s(pszHostname, &pwszHostname);
                BAIL_ON_EVT_ERROR(dwError);
            }
            pRecords[index].pComputer = pwszHostname;
        }
    }

    if (pConn->Local)
    {
        dwError = LwmEvtWriteRecords(
                        pConn->Local,
                        Count,
                        pRecords);
        BAIL_ON_EVT_ERROR(dwError);
    }
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    else
    {
        TRY
        {
            dwError = RpcEvtWriteRecords(
                        pConn->Remote,
                        Count,
                        pRecords);
        }
        CATCH_ALL
        {
            dwError = EVTGetRpcError(THIS_CATCH);
        }
        ENDTRY;

        BAIL_ON_EVT_ERROR(dwError);
    }
#endif

cleanup:
    for (index = 0; index < Count; index++)
    {
        if (pRecords[index].pComputer == pwszHostname)
        {
            pRecords[index].pComputer = NULL;
        }
    }
    LW_SAFE_FREE_MEMORY(pwszHostname);
    return dwError;

error:
    EVT_LOG_ERROR("Failed to write records. Error code [%d]\n", dwError);

    goto cleanup;
}

DWORD
LwEvtDeleteRecords(
    IN PLW_EVENTLOG_CONNECTION pConn,
    IN OPTIONAL PCWSTR pSqlFilter
    )
{
    volatile DWORD dwError = 0;

    if (pConn->Local)
    {
        dwError = LwmEvtDeleteRecords(
                        pConn->Local,
                        pSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);
    }
#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
    else
    {
        TRY
        {
            dwError = RpcEvtDeleteRecords(
                            pConn->Remote,
                            (PWSTR)pSqlFilter);
        }
        CATCH_ALL
        {
            dwError = EVTGetRpcError(THIS_CATCH);
        }
        ENDTRY;

        BAIL_ON_EVT_ERROR(dwError);
    }
#endif

cleanup:
    return dwError;

error:
    EVT_LOG_ERROR("Failed to delete records. Error code [%d]\n", dwError);

    goto cleanup;
}

#ifndef _EVENTLOG_NO_DCERPC_SUPPORT_
DWORD
EVTGetRpcError(
    dcethread_exc* exCatch
    )
{
    DWORD dwError = 0;
    dwError = dcethread_exc_getstatus (exCatch);
    return LwNtStatusToWin32Error(LwRpcStatusToNtStatus(dwError));
}
#endif
