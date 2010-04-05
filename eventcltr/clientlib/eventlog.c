/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Client API
 *
 */

#include "stdafx.h"

void
LW_CLTRLIB_API
CltrFreeRecordList(
    DWORD cRecords,
    PLWCOLLECTOR_RECORD pRecordList
    )
{
    DWORD iRecord = 0;
    for (iRecord = 0; iRecord < cRecords; iRecord++)
    {
        CltrFreeRecordContents(&pRecordList[iRecord]);
    }
    CLTR_SAFE_FREE_MEMORY(pRecordList);
}

void
LW_CLTRLIB_API
CltrFreeRecordContents(
    PLWCOLLECTOR_RECORD pRecord
    )
{
    CLTR_SAFE_FREE_STRING(pRecord->pwszColComputer);
    CLTR_SAFE_FREE_STRING(pRecord->pwszColComputerAddress);

    CLTR_SAFE_FREE_STRING(pRecord->event.pwszLogname);
    CLTR_SAFE_FREE_STRING(pRecord->event.pwszEventType);
    CLTR_SAFE_FREE_STRING(pRecord->event.pwszEventSource);
    CLTR_SAFE_FREE_STRING(pRecord->event.pwszEventCategory);
    CLTR_SAFE_FREE_STRING(pRecord->event.pwszUser);
    CLTR_SAFE_FREE_STRING(pRecord->event.pwszComputer);
    CLTR_SAFE_FREE_STRING(pRecord->event.pwszDescription);
    CLTR_SAFE_FREE_MEMORY(pRecord->event.pvData);
}

void
LW_CLTRLIB_API
CltrFreeEventLogHandle(
    HANDLE hEventLog
    )
{
    CLTR_LOG_VERBOSE("client::eventlog.c FreeEventLogHandle(pEventLogHandle=%p)\n",
                     hEventLog);
    CltrFreeMemory((void *) hEventLog);
    CLTR_LOG_VERBOSE("client::eventlog.c FreeEventLogHandle() Finished\n");
}

DWORD
LW_CLTRLIB_API
CltrOpenCollector(
    PCWSTR pszServerName,
    PCWSTR pszServerSpn,
    ACCESS_TOKEN pIoAccessToken,
    PHANDLE phEventLog
    )
{
    DWORD dwError = 0;
    PCOLLECTOR_HANDLE* ppEventLogHandle = (PCOLLECTOR_HANDLE*) phEventLog;
    PCOLLECTOR_HANDLE pEventLogHandle = NULL;
#ifdef _WIN32
    BOOL bImpersonated = FALSE;
#else
    rpc_transport_info_handle_t Info = NULL;
#endif

    handle_t eventBindingLocal = 0;
    WCHAR* bindingStringLocal = NULL;
    PWSTR pwszServerNameLocal = NULL;

    CLTR_LOG_VERBOSE("client::eventlog.c OpenEventLog(server=%ws)\n",pszServerName);

    dwError = CltrAllocateMemory(sizeof(COLLECTOR_HANDLE), (PVOID*) ppEventLogHandle);
    BAIL_ON_CLTR_ERROR(dwError);

    pEventLogHandle = *ppEventLogHandle;

    
    if (IsNullOrEmptyString(pszServerName)) {
        dwError = LwRtlWC16StringAllocateFromCString(
                        &pwszServerNameLocal,
                        "127.0.0.1");
        BAIL_ON_CLTR_ERROR(dwError);
    }
    else {
        dwError = LwRtlWC16StringDuplicate(
                        &pwszServerNameLocal,
                        pszServerName);
        BAIL_ON_CLTR_ERROR(dwError);
    }
    

    RpcTryExcept
    {
        dwError = CltrCreateEventLogRpcBinding(
            pwszServerNameLocal,
            pszServerSpn,
                    &bindingStringLocal,
                    &eventBindingLocal);
        BAIL_ON_CLTR_ERROR(dwError);

#ifdef _WIN32
        if (!ImpersonateLoggedOnUser(pIoAccessToken))
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }
        bImpersonated = TRUE;
#else
        rpc_smb_transport_info_from_lwio_creds(
            pIoAccessToken,
            FALSE,
            &Info,
            (unsigned32 *)&dwError);
        BAIL_ON_CLTR_ERROR(dwError);

        rpc_binding_set_transport_info(
            eventBindingLocal,
            Info,
            (unsigned32 *)&dwError);
        BAIL_ON_CLTR_ERROR(dwError);

        Info = NULL;
#endif
        
        dwError = RpcCltrOpen(
                    eventBindingLocal,
                    &pEventLogHandle->hCollector
                    );
        BAIL_ON_CLTR_ERROR(dwError);
    }
    RpcExcept(1)
    {
        dwError = RpcExceptionCode();
        switch (dwError)
        {
            // The end point mapper could not be contacted because the
            // machine is not reachable or it is not running an end
            // point mapper.
            // This is not the same as RPC_S_COMM_FAILURE
            case ERROR_CONNECTION_REFUSED:
                dwError = LW_STATUS_CONNECTION_REFUSED;
                break;

            case EPT_S_NOT_REGISTERED:
            // The service is not running on the machine
                dwError = LW_STATUS_CONNECTION_REFUSED;
                break;
            // Could not get service ticket
            case RPC_S_INVALID_AUTH_IDENTITY:
                dwError = LW_STATUS_RESOURCE_NAME_NOT_FOUND;
                break;
        }
        CLTR_LOG_ERROR("Failed to create event log. Error code [%d]\n", dwError);
        BAIL_ON_CLTR_ERROR(dwError);
    }
    RpcEndExcept

    BAIL_ON_CLTR_ERROR(dwError);

    pEventLogHandle->pszBindingString = bindingStringLocal;

cleanup:
#ifdef _WIN32
    if (bImpersonated)
    {
        RevertToSelf();
    }
#else
    if (Info)
    {
        rpc_smb_transport_info_free(Info);
    }
#endif
    LwRtlWC16StringFree(&pwszServerNameLocal);
    CltrFreeEventLogRpcBinding(eventBindingLocal, NULL);
    return dwError;

error:
    CltrFreeEventLogRpcBinding(NULL, bindingStringLocal);
    goto cleanup;
}

DWORD
LW_CLTRLIB_API
CltrCloseCollector(
    HANDLE hEventLog
    )
{
    DWORD dwError = 0;
    PCOLLECTOR_HANDLE pEventLogHandle = (PCOLLECTOR_HANDLE) hEventLog;

    CLTR_LOG_VERBOSE("client::eventlog.c CloseEventLog(pEventLogHandle=%p)\n",
        pEventLogHandle->hCollector);

    if (pEventLogHandle == NULL) {
        CLTR_LOG_ERROR("LWICloseEventLog() called with pEventLogHandle=NULL");
        return -1;
    }

    RpcTryExcept
    {
        dwError = RpcCltrClose(&pEventLogHandle->hCollector);
        BAIL_ON_CLTR_ERROR(dwError);
    }
    RpcExcept(1)
    {
        dwError = RpcExceptionCode();
        CLTR_LOG_ERROR("Failed to close event log. Error code [%d]\n", dwError);
        BAIL_ON_CLTR_ERROR(dwError);
    }
    RpcEndExcept

    BAIL_ON_CLTR_ERROR(dwError);

error:

    if (pEventLogHandle)
    {
        if (pEventLogHandle->hCollector != NULL)
        {
            RpcSsDestroyClientContext(&pEventLogHandle->hCollector);
        }
        CltrFreeEventLogRpcBinding(NULL, pEventLogHandle->pszBindingString);
        CLTR_SAFE_FREE_MEMORY(pEventLogHandle);
    }

    CLTR_LOG_VERBOSE("client::eventlog.c CloseEventLog() finished\n");

    return dwError;
}


DWORD
LW_CLTRLIB_API
CltrReadRecords(
    HANDLE hEventLog,
    UINT64 qwLastRecordId,
    DWORD nRecordsPerPage,
    PCWSTR sqlFilter,
    DWORD* pdwNumReturned,
    LWCOLLECTOR_RECORD** ppeventRecords
    )
{
    DWORD dwError = 0;
    WCHAR* sqlFilterWCHAR = NULL;
    PCOLLECTOR_HANDLE pEventLogHandle = (PCOLLECTOR_HANDLE) hEventLog;
    LWCOLLECTOR_RECORD_LIST list = {0};

    if (sqlFilter == NULL) {
    CLTR_LOG_ERROR("ReadEventLogEx(): sqlFilter == NULL\n");
    return -1;
    }

    CLTR_LOG_VERBOSE("client::eventlog.c ReadEventLogEx(pEventLogHandle=%p, lastRecord=%lld, pageSize=%d,\n)\n",
        pEventLogHandle->hCollector, qwLastRecordId, nRecordsPerPage);

    dwError = CltrLWCHARToLpStr(sqlFilter, &sqlFilterWCHAR);

    BAIL_ON_CLTR_ERROR(dwError);

    CLTR_LOG_VERBOSE("client::eventlog.c ReadEventLogEx() sqlFilterWCHAR=\"%s\"\n", sqlFilterWCHAR);

    dwError = RpcCltrReadRecords( pEventLogHandle->hCollector,
                                  qwLastRecordId, nRecordsPerPage,
                                  sqlFilterWCHAR, &list); 
    *pdwNumReturned = list.dwCount;
    *ppeventRecords = list.pRecords;

    BAIL_ON_CLTR_ERROR(dwError);


 error:

    CLTR_LOG_VERBOSE("client::eventlog.c ReadEventLogEx(dwError=%d, returned=%d) finish\n",
    dwError, *pdwNumReturned);

    return dwError;
}

DWORD
LW_CLTRLIB_API
CltrGetPushRate(
    HANDLE hEventLog,
    DWORD dwEventsWaiting,
    double dPeriodConsumed,
    PDWORD pdwRecordsPerPeriod,
    PDWORD pdwRecordsPerBatch,
    PDWORD pdwPeriodSecs
    )
{
    PCOLLECTOR_HANDLE pEventLogHandle = (PCOLLECTOR_HANDLE) hEventLog;
    return RpcCltrGetPushRate(
        pEventLogHandle->hCollector,
        dwEventsWaiting,
        dPeriodConsumed,
        pdwRecordsPerPeriod,
        pdwRecordsPerBatch,
        pdwPeriodSecs);
}

DWORD
LW_CLTRLIB_API
CltrGetRecordCount(
    HANDLE hEventLog,
    PCWSTR sqlFilter,
    DWORD* pdwNumMatched
    )
{
    DWORD dwError = 0;
    WCHAR* sqlFilterWCHAR = NULL;

    PCOLLECTOR_HANDLE pEventLogHandle = (PCOLLECTOR_HANDLE) hEventLog;

    if (sqlFilter == NULL) {
        CLTR_LOG_VERBOSE("CountEventLog(): sqlFilter == NULL\n");
        return -1;
    }

    CLTR_LOG_VERBOSE("client::eventlog.c CountEventLog(pEventLogHandle=%p)\n",
                    pEventLogHandle->hCollector);


    dwError = CltrLWCHARToLpStr((PCWSTR)sqlFilter, (PWSTR*)(&sqlFilterWCHAR));

    BAIL_ON_CLTR_ERROR(dwError);

    CLTR_LOG_VERBOSE("client::eventlog.c CountEventLog() sqlFilterWCHAR=\"%s\"\n", sqlFilterWCHAR);

    dwError = RpcCltrGetRecordCount( pEventLogHandle->hCollector,
                                   sqlFilterWCHAR, 
                                   pdwNumMatched);

    BAIL_ON_CLTR_ERROR(dwError);

error:

    CLTR_LOG_VERBOSE("client::eventlog.c CountEventLog(returned=%d) finished\n", *pdwNumMatched);

    return dwError;
}

DWORD
LW_CLTRLIB_API
CltrWriteRecords(
    HANDLE hEventLog,
    DWORD cRecords,
    LWCOLLECTOR_RECORD *peventRecords
    )
{
    DWORD dwError = 0;
    PCOLLECTOR_HANDLE pEventLogHandle = (PCOLLECTOR_HANDLE) hEventLog;

    CLTR_LOG_VERBOSE("client::eventlog.c WriteEventLog(pEventLogHandle=%p)\n",
        pEventLogHandle->hCollector);

    RpcTryExcept
    {
        dwError = RpcCltrWriteRecords( pEventLogHandle->hCollector, cRecords, peventRecords);
    }
    RpcExcept(1)
    {
        dwError = RpcExceptionCode();
        if (!dwError)
        {
            dwError = CLTR_ERROR_RPC_EXCEPTION_UPON_OPEN;
        }
    }
    RpcEndExcept

    BAIL_ON_CLTR_ERROR(dwError);

error:

    CLTR_LOG_VERBOSE("client::eventlog.c WriteEventLog() finish\n");

    return dwError;
}

DWORD
LW_CLTRLIB_API
CltrDeleteRecords(
    HANDLE hEventLog,
    PCWSTR sqlFilter
    )
{
    DWORD dwError = 0;

    PCOLLECTOR_HANDLE pEventLogHandle = (PCOLLECTOR_HANDLE) hEventLog;
    PWSTR sqlFilterWCHAR = NULL;

    CLTR_LOG_VERBOSE("client::eventlog.c DeleteFromEventLog(pEventLogHandle=%p)\n",
                    pEventLogHandle->hCollector);

    dwError = CltrLWCHARToLpStr(sqlFilter, &sqlFilterWCHAR);
    BAIL_ON_CLTR_ERROR(dwError);

    CLTR_LOG_VERBOSE("client::eventlog.c DeleteFromEventLog() sqlFilterWCHAR=\"%s\"\n", sqlFilterWCHAR);

    dwError = RpcCltrDeleteRecords(pEventLogHandle->hCollector, sqlFilterWCHAR);

    BAIL_ON_CLTR_ERROR(dwError);

error:

    CLTR_LOG_VERBOSE("client::eventlog.c DeleteFromEventLog() finish\n");

    return dwError;
}

DWORD
LW_CLTRLIB_API
CltrClearRecords(
    HANDLE hEventLog
    )
{
    DWORD dwError = 0;
    PCOLLECTOR_HANDLE pEventLogHandle = (PCOLLECTOR_HANDLE) hEventLog;
    WCHAR pwszTrue[] = {'C', 'o', 'l', 'E', 'v', 'e', 'n', 't', 'R', 'e', 'c',
        'o', 'r', 'd', 'I', 'd', ' ', '>', '=', ' ', '0', 0};

    CLTR_LOG_VERBOSE("client::eventlog.c ClearEventLog(pEventLogHandle=%p)\n",
                    pEventLogHandle->hCollector);

    dwError = RpcCltrDeleteRecords(pEventLogHandle->hCollector, pwszTrue);

    BAIL_ON_CLTR_ERROR(dwError);

error:

    CLTR_LOG_VERBOSE("client::eventlog.c ClearEventLog() finish\n");

    return dwError;
}

