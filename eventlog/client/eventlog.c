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
 *
 * Eventlog Client API
 *
 */
#include "includes.h"


VOID
LWIFreeEventRecord(
    PEVENT_LOG_RECORD pEventRecord
    )
{
    LWIFreeEventRecordList(1, pEventRecord);
}

static
VOID
LWIFreeEventRecordContents(
    PEVENT_LOG_RECORD pEventRecord
)
{
    EVT_SAFE_FREE_STRING(pEventRecord->pszEventTableCategoryId);
    EVT_SAFE_FREE_STRING(pEventRecord->pszEventType);
    EVT_SAFE_FREE_STRING(pEventRecord->pszEventSource);
    EVT_SAFE_FREE_STRING(pEventRecord->pszEventCategory);
    EVT_SAFE_FREE_STRING(pEventRecord->pszUser);
    EVT_SAFE_FREE_STRING(pEventRecord->pszComputer);
    EVT_SAFE_FREE_STRING(pEventRecord->pszDescription);
    EVT_SAFE_FREE_MEMORY(pEventRecord->pszData);
}

VOID
LWIFreeEventRecordList(
    DWORD dwRecords,
    PEVENT_LOG_RECORD pEventRecordList
    )
{
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < dwRecords; dwIndex++)
    {
        LWIFreeEventRecordContents(&pEventRecordList[dwIndex]);
    }

    EVTFreeMemory(pEventRecordList);
}

void
LWIFreeEventLogHandle(
    HANDLE hEventLog
    )
{

    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PEVENT_LOG_RECORD pEventRecord = &(pEventLogHandle->defaultEventLogRecord);

    LWIFreeEventLogRpcBinding(pEventLogHandle->bindingHandle);
    LWIFreeEventRecordContents(pEventRecord);
    EVT_SAFE_FREE_MEMORY(pEventLogHandle);
}

DWORD
LWIOpenEventLog(
    PCSTR pszServerName,
    PHANDLE phEventLog
    )
{
    volatile DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = NULL;
    char serverNameLocal[1024];

    handle_t eventBindingLocal = 0;

#ifdef _WIN32
    if (gBasicLogStreamFD == NULL) {
        dwError = init_basic_log_stream("evtrpcclient_dll.log");
        BAIL_ON_EVT_ERROR(dwError);
    }
#endif //!_WIN32

    EVT_LOG_VERBOSE("client::eventlog.c OpenEventLog(*phEventLog=%.16X, server=%s)\n",
            *phEventLog, pszServerName);

    dwError = EVTAllocateMemory(sizeof(EVENT_LOG_HANDLE), (PVOID*) &pEventLogHandle);
    BAIL_ON_EVT_ERROR(dwError);
    
    pEventLogHandle->bDefaultActive = FALSE;

    if (IsNullOrEmptyString(pszServerName))
    {
        PSTR pszDefaultHostName = NULL;
        
        dwError = EVTGetHostname(&pszDefaultHostName);
        BAIL_ON_EVT_ERROR(dwError);
        
        strncpy((char *)serverNameLocal,
                 pszDefaultHostName,
                 sizeof(serverNameLocal));
        
        EVT_SAFE_FREE_STRING(pszDefaultHostName);
    }
    else
    {
        strncpy((char *)serverNameLocal,
                pszServerName,
                sizeof(serverNameLocal));
    }
    

    TRY
    {
        dwError = LWICreateEventLogRpcBinding(pszServerName,
                                              &eventBindingLocal);
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING);
    }
    ENDTRY

    BAIL_ON_EVT_ERROR(dwError);

    TRY
    {
        dwError = RpcLWIOpenEventLog(eventBindingLocal,
                                     (idl_char*)serverNameLocal,
                                     (idl_char*)serverNameLocal);
    }
    CATCH (rpc_x_auth_method)
    {
        dwError = EVT_ERROR_ACCESS_DENIED;
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_OPEN);
    }
    ENDTRY

    BAIL_ON_EVT_ERROR(dwError);

    pEventLogHandle->bindingHandle = eventBindingLocal;
    *phEventLog = (HANDLE)pEventLogHandle;


cleanup:
    return dwError;

error:
    switch(dwError)
    {
        case EVT_ERROR_ACCESS_DENIED:
            EVT_LOG_ERROR("Failed to open event log. Access is denied.\n");
            break;
        default:
            EVT_LOG_ERROR("Failed to open event log. Error code [%d]\n", dwError);
            break;
    }

    if (pEventLogHandle)
    {
        LWIFreeEventLogHandle((HANDLE)pEventLogHandle);
    }

    if (eventBindingLocal)
    {
        LWIFreeEventLogRpcBinding(eventBindingLocal);
    }

    *phEventLog = (HANDLE) NULL;

    goto cleanup;

}

DWORD
LWIOpenEventLogEx(
    PCSTR pszServerName,
    PCSTR pszEventTableCategoryId,
    PCSTR pszSource,
    DWORD dwEventSourceId,
    PCSTR pszUser,
    PCSTR pszComputer,
    PHANDLE phEventLog
    )
{
    DWORD dwError = 0;

    HANDLE hEventLogLocal = 0;

    EVT_LOG_VERBOSE("client::eventlog.c OpenEventLog(server=%s, source=%s, user=%s, computer=%s)\n",
            pszServerName, pszSource, pszUser, pszComputer);

    dwError = LWIOpenEventLog( pszServerName, 
                               &hEventLogLocal);
    BAIL_ON_EVT_ERROR(dwError);


    dwError = LWISetEventLogTableCategoryId(hEventLogLocal,
                                            pszEventTableCategoryId);
    BAIL_ON_EVT_ERROR(dwError);


    dwError = LWISetEventLogSource(hEventLogLocal,
                                   pszSource,
                                   dwEventSourceId);
    BAIL_ON_EVT_ERROR(dwError);


    dwError = LWISetEventLogUser(hEventLogLocal,
                 pszUser);
    BAIL_ON_EVT_ERROR(dwError);


    dwError = LWISetEventLogComputer(hEventLogLocal,
                     pszComputer);
    BAIL_ON_EVT_ERROR(dwError);


    *phEventLog = hEventLogLocal;

cleanup:

    EVT_LOG_VERBOSE("client::eventlog.c OpenEventLog(*phEventLog=%.16X)\n",
            *phEventLog);

    return dwError;

error:

    if (hEventLogLocal)
    {
        LWIFreeEventLogHandle(hEventLogLocal);
    }

    *phEventLog = (HANDLE)NULL;

    goto cleanup;
}


DWORD
LWICloseEventLog(
    HANDLE hEventLog
    )
{
    volatile DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;

    if (pEventLogHandle == NULL) {
        EVT_LOG_ERROR("LWICloseEventLog() called with pEventLogHandle=NULL");
        return -1;
    }

    TRY
    {
        dwError = RpcLWICloseEventLog(
            (handle_t) pEventLogHandle->bindingHandle
            );
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_CLOSE);
    }
    ENDTRY;

    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    if (pEventLogHandle)
    {
        LWIFreeEventLogHandle((HANDLE)pEventLogHandle);
    }

    return dwError;

error:

    EVT_LOG_ERROR("Failed to close event log. Error code [%d]\n", dwError);

    goto cleanup;
}


DWORD
LWIReadEventLog(
    HANDLE hEventLog,
    DWORD dwLastRecordId,
    DWORD nRecordsPerPage,
    PCWSTR sqlFilter,
    DWORD* pdwNumReturned,
    EVENT_LOG_RECORD** eventRecords
    )
{
    volatile DWORD dwError = 0;
    char* sqlFilterChar = NULL;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;

    if (sqlFilter == NULL) {
        dwError = EVT_ERROR_INTERNAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = EVTLpwStrToLpStr(sqlFilter, (PSTR*)(&sqlFilterChar));
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_VERBOSE("client::eventlog.c ReadEventLog() sqlFilterChar=\"%s\"\n", sqlFilterChar);

    EVTAllocateMemory(nRecordsPerPage * sizeof(EVENT_LOG_RECORD), (PVOID*)(eventRecords));

    TRY
    {
        dwError = RpcLWIReadEventLog(
                    (handle_t) pEventLogHandle->bindingHandle,
                    dwLastRecordId, 
                    nRecordsPerPage,
                    (idl_char*)sqlFilterChar, 
                    (unsigned32*)pdwNumReturned, 
                    *eventRecords);
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_READ);
    }
    ENDTRY;

    BAIL_ON_EVT_ERROR(dwError);


cleanup:
    EVT_SAFE_FREE_STRING(sqlFilterChar);
    return dwError;

error:
    EVT_LOG_ERROR("Failed to read event log. Error code [%d]\n", dwError);
    goto cleanup;
}


DWORD
LWICountEventLog(
    HANDLE hEventLog,
    PCWSTR sqlFilter,
    DWORD* pdwNumMatched
    )
{
    volatile DWORD dwError = 0;
    char* sqlFilterChar = NULL;

    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;

    if (sqlFilter == NULL) {
        EVT_LOG_VERBOSE("CountEventLog(): sqlFilter == NULL\n");
        dwError = EVT_ERROR_INTERNAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = EVTLpwStrToLpStr((PWSTR)sqlFilter, (PSTR*)(&sqlFilterChar));

    BAIL_ON_EVT_ERROR(dwError);

    TRY
    {
        dwError = RpcLWIEventLogCount(
                    (handle_t) pEventLogHandle->bindingHandle,
                    (idl_char*)sqlFilterChar,
                    (unsigned32*)pdwNumMatched);
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_COUNT);
    }
    ENDTRY;

    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    EVT_SAFE_FREE_STRING(sqlFilterChar);
    return dwError;

error:
    EVT_LOG_ERROR("Failed to count event log. Error code [%d]\n", dwError);
    goto cleanup;
}


DWORD
LWISetEventLogTableCategoryId(
    HANDLE hEventLog,
    PCSTR  pszEventTableCategoryId
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PEVENT_LOG_RECORD pEventRecord = &(pEventLogHandle->defaultEventLogRecord);

    if (!IsNullOrEmptyString(pszEventTableCategoryId)) {
        dwError = EVTAllocateString(pszEventTableCategoryId, (&pEventRecord->pszEventTableCategoryId));
        BAIL_ON_EVT_ERROR(dwError);
        pEventLogHandle->bDefaultActive = TRUE;
    }

error:
    return dwError;
}


DWORD
LWISetEventLogType(
    HANDLE hEventLog,
    PCSTR pszEventType
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PEVENT_LOG_RECORD pEventRecord = &(pEventLogHandle->defaultEventLogRecord);

    if (!IsNullOrEmptyString(pszEventType)) {
        dwError = EVTAllocateString(pszEventType, (&pEventRecord->pszEventType));
        BAIL_ON_EVT_ERROR(dwError);
        pEventLogHandle->bDefaultActive = TRUE;
    }

error:
    return dwError;
}


DWORD
LWISetEventLogSource(
    HANDLE hEventLog,
    PCSTR pszEventSource,
    DWORD dwEventSourceId
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PEVENT_LOG_RECORD pEventRecord = &(pEventLogHandle->defaultEventLogRecord);

    if (!IsNullOrEmptyString(pszEventSource)) {
        dwError = EVTAllocateString(pszEventSource, (&pEventRecord->pszEventSource));
        BAIL_ON_EVT_ERROR(dwError);
        pEventLogHandle->bDefaultActive = TRUE;
    }

    if (pEventRecord->dwEventSourceId != dwEventSourceId) {
        pEventRecord->dwEventSourceId = dwEventSourceId;
        pEventLogHandle->bDefaultActive = TRUE;
    }

error:
    return dwError;
}


DWORD
LWISetEventLogTableCategory(
    HANDLE hEventLog,
    PCSTR pszEventCategory
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PEVENT_LOG_RECORD pEventRecord = &(pEventLogHandle->defaultEventLogRecord);

    if (!IsNullOrEmptyString(pszEventCategory)) {
        dwError = EVTAllocateString(pszEventCategory, (&pEventRecord->pszEventCategory));
        BAIL_ON_EVT_ERROR(dwError);
        pEventLogHandle->bDefaultActive = TRUE;
    }
    
error:
    return dwError;
}


DWORD
LWISetEventLogUser(
    HANDLE hEventLog,
    PCSTR pszUser
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PEVENT_LOG_RECORD pEventRecord = &(pEventLogHandle->defaultEventLogRecord);

    if (IsNullOrEmptyString(pszUser)) {
#ifndef _WIN32
    uid_t processUID = getuid();
    struct passwd* processPWD = getpwuid(processUID);
    if (!IsNullOrEmptyString(processPWD->pw_name)) {
        dwError = EVTAllocateString(processPWD->pw_name, (&pEventRecord->pszUser));
        BAIL_ON_EVT_ERROR(dwError);
        pEventLogHandle->bDefaultActive = TRUE;
    }
#endif
    }
    else {
        dwError = EVTAllocateString(pszUser, (&pEventRecord->pszUser));
        BAIL_ON_EVT_ERROR(dwError);
        pEventLogHandle->bDefaultActive = TRUE;
    }
    
 error:
    return dwError;
}


DWORD
LWISetEventLogComputer(
    HANDLE hEventLog,
    PCSTR pszComputer
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PEVENT_LOG_RECORD pEventRecord = &(pEventLogHandle->defaultEventLogRecord);

    if (IsNullOrEmptyString(pszComputer)) {
#ifndef _WIN32
        char currentHost[129];
        dwError = gethostname(currentHost, 128);
        if (!IsNullOrEmptyString(currentHost)) {
            dwError = EVTAllocateString(currentHost, (&pEventRecord->pszComputer));
            BAIL_ON_EVT_ERROR(dwError);
            pEventLogHandle->bDefaultActive = TRUE;
        }
#endif
    }
    else {
        dwError = EVTAllocateString(pszComputer, (&pEventRecord->pszComputer));
        BAIL_ON_EVT_ERROR(dwError);
        pEventLogHandle->bDefaultActive = TRUE;
    }
    
error:
    return dwError;
}


DWORD
LWIWriteEventLogRecords(
    HANDLE hEventLog,
    DWORD cRecords,
    PEVENT_LOG_RECORD pEventRecords 
    )
{
    volatile DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;

    TRY
    {
        dwError = RpcLWIWriteEventLogRecords(
                        (handle_t) pEventLogHandle->bindingHandle,
                        cRecords,
                        pEventRecords);
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_WRITE);        
    }
    ENDTRY;

    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    return dwError;
    
error:
    EVT_LOG_ERROR("Failed to write event log. Error code [%d]\n", dwError);
    goto cleanup;
}

DWORD
LWIWriteEventLogBase(
    HANDLE hEventLog,
    EVENT_LOG_RECORD eventRecord
    )
{
    volatile DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    EVENT_LOG_RECORD  eventRecordLocal = eventRecord;

    EVT_LOG_VERBOSE("client::eventlog.c WriteEventLog(pEventLogHandle=%.16X, computer=%s)\n",
        pEventLogHandle, (IsNullOrEmptyString(eventRecord.pszComputer) ? "" : eventRecord.pszComputer));

    //Copy any empty fields from defaults
    if (pEventLogHandle->bDefaultActive)
    {
        char* pszDefault = NULL;
        DWORD dwDefault = 0;

        EVT_LOG_VERBOSE("client::eventlog.c WriteEventLog() checking defaults\n");

        pszDefault = pEventLogHandle->defaultEventLogRecord.pszEventTableCategoryId;
        if (IsNullOrEmptyString(eventRecordLocal.pszEventTableCategoryId) && !IsNullOrEmptyString(pszDefault))
        {
            eventRecordLocal.pszEventTableCategoryId = pszDefault;
        }

        if (eventRecordLocal.dwEventDateTime == 0)
        {
            if (pEventLogHandle->defaultEventLogRecord.dwEventDateTime != 0)
            {
                eventRecordLocal.dwEventDateTime = pEventLogHandle->defaultEventLogRecord.dwEventDateTime;
            }
            else {
                eventRecordLocal.dwEventDateTime = (DWORD) time(NULL);
            }
        }

        pszDefault = pEventLogHandle->defaultEventLogRecord.pszEventSource;
        if (IsNullOrEmptyString(eventRecordLocal.pszEventSource) && !IsNullOrEmptyString(pszDefault))
        {
            eventRecordLocal.pszEventSource = pszDefault;
        }

        pszDefault = pEventLogHandle->defaultEventLogRecord.pszEventCategory;
        if (IsNullOrEmptyString(eventRecordLocal.pszEventCategory) && !IsNullOrEmptyString(pszDefault))
        {
            eventRecordLocal.pszEventCategory = pszDefault;
        }

        dwDefault = pEventLogHandle->defaultEventLogRecord.dwEventSourceId;
        if (eventRecordLocal.dwEventSourceId == 0 && dwDefault != 0)
        {
            eventRecordLocal.dwEventSourceId = dwDefault;
        }

        pszDefault = pEventLogHandle->defaultEventLogRecord.pszUser;
        if (IsNullOrEmptyString(eventRecordLocal.pszUser) && !IsNullOrEmptyString(pszDefault))
        {
            eventRecordLocal.pszUser = pszDefault;
        }

        pszDefault = pEventLogHandle->defaultEventLogRecord.pszComputer;
        if (IsNullOrEmptyString(eventRecordLocal.pszComputer) && !IsNullOrEmptyString(pszDefault))
        {
            eventRecordLocal.pszComputer = pszDefault;
        }

        pszDefault = pEventLogHandle->defaultEventLogRecord.pszDescription;
        if (IsNullOrEmptyString(eventRecordLocal.pszDescription) && !IsNullOrEmptyString(pszDefault))
        {
            eventRecordLocal.pszDescription = pszDefault;
        }
        
        pszDefault = pEventLogHandle->defaultEventLogRecord.pszData;
        if (IsNullOrEmptyString(eventRecordLocal.pszData) && !IsNullOrEmptyString(pszDefault))
        {
            eventRecordLocal.pszData = pszDefault;
        }

    } //end if (bDefaultActive)

    TRY
    {
        dwError = RpcLWIWriteEventLogRecords(
                        (handle_t) pEventLogHandle->bindingHandle,
                        1,
                        &eventRecordLocal);
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_WRITE);        
    }
    ENDTRY;

    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    return dwError;
    
error:
    EVT_LOG_ERROR("Failed to write event log. Error code [%d]\n", dwError);
    goto cleanup;
}

DWORD
LWIWriteEventLog(
    HANDLE hEventLog,
    PCSTR eventType,
    PCSTR eventCategory,
    PCSTR eventDescription,
    PCSTR eventData
    )
{

    DWORD dwError = 0;

    EVENT_LOG_RECORD eventRecord;

    eventRecord.dwEventRecordId = 0;
    eventRecord.pszEventTableCategoryId = NULL;
    eventRecord.pszEventType = (PSTR)eventType;
    eventRecord.dwEventDateTime = (DWORD) time(NULL);
    eventRecord.pszEventSource = NULL;
    eventRecord.pszEventCategory = (PSTR)eventCategory;
    eventRecord.dwEventSourceId = 0;
    eventRecord.pszUser = NULL;
    eventRecord.pszComputer = NULL;
    eventRecord.pszDescription = (PSTR)eventDescription;   
    eventRecord.pszData = (PSTR)eventData;

    dwError = LWIWriteEventLogBase( hEventLog,
    							    eventRecord );

    return dwError;

}


DWORD
LWIDeleteFromEventLog(
    HANDLE hEventLog,
    PCWSTR sqlFilter
    )
{
    volatile DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;
    PSTR sqlFilterChar = NULL;

    dwError = EVTLpwStrToLpStr(sqlFilter, &sqlFilterChar);
    BAIL_ON_EVT_ERROR(dwError);

    TRY
    {
        dwError = RpcLWIDeleteFromEventLog( (handle_t) pEventLogHandle->bindingHandle,
                                            (idl_char*)sqlFilterChar);
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_DELETE);
    }
    ENDTRY;

    BAIL_ON_EVT_ERROR(dwError);


cleanup:
    return dwError;

error:
    EVT_LOG_ERROR("Failed to delete entry from event log. Error code [%d]\n", dwError);
    goto cleanup;
}

DWORD
LWIClearEventLog(
    HANDLE hEventLog
    )
{
    volatile DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = (PEVENT_LOG_HANDLE) hEventLog;

    TRY
    {
        dwError = RpcLWIClearEventLog( (handle_t) pEventLogHandle->bindingHandle);
    }
    CATCH_ALL
    {
        dwError = EVTGetRpcError(THIS_CATCH, EVT_ERROR_RPC_EXCEPTION_UPON_CLEAR);
    }
    ENDTRY;

    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    return dwError;

error:
    EVT_LOG_ERROR("Failed to clear event log. Error code [%d]\n", dwError);
    goto cleanup;

}

DWORD
EVTGetRpcError(
    dcethread_exc* exCatch,
    DWORD dwEVTError
    )
{
    DWORD dwError = 0;

#ifdef _WIN32
    dwError = dwEVTError;
#else
    dwError = dcethread_exc_getstatus (exCatch);
    if(!dwError)
    {
        dwError = dwEVTError;
    }
#endif //!_WIN32
    
    return dwError;
}

