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
 *        event.h
 *
 * Abstract:
 *
 *        Likewise Security And Authentication Subsystem (LSASS)
 *
 *        Eventlog API
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvLogInformationEvent(
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_LSASS)
    return LW_ERROR_SUCCESS;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = INFORMATION_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise LSASS";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LsaSrvQueueEvent(&event);
#endif
}

DWORD
LsaSrvLogWarningEvent(
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_LSASS)
    return LW_ERROR_SUCCESS;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = WARNING_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise LSASS";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LsaSrvQueueEvent(&event);
#endif
}

DWORD
LsaSrvLogErrorEvent(
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_LSASS)
    return LW_ERROR_SUCCESS;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = ERROR_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise LSASS";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LsaSrvQueueEvent(&event);
#endif
}

DWORD
LsaSrvLogSuccessAuditEvent(
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_LSASS)
    return LW_ERROR_SUCCESS;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = SUCCESS_AUDIT_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise LSASS";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LsaSrvQueueEvent(&event);
#endif
}

DWORD
LsaSrvLogFailureAuditEvent(
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
#if defined(MINIMAL_LSASS)
    return LW_ERROR_SUCCESS;
#else
    EVENT_LOG_RECORD event = {0};

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = "System";
    event.pszEventType = FAILURE_AUDIT_EVENT_TYPE;
    event.dwEventDateTime = 0;
    event.pszEventSource = "Likewise LSASS";
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    return LsaSrvQueueEvent(&event);
#endif
}

VOID
LsaSrvLogServiceSuccessEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    )
{
    DWORD dwError = 0;
    
    dwError = LsaSrvLogInformationEvent(            
                  dwEventID,
                  NULL, // defaults to SYSTEM
                  pszEventCategory,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return;

error:

    LSA_LOG_VERBOSE("Failed to post service success event.");
    LSA_LOG_VERBOSE("Error code: [%u]", dwError);

    goto cleanup;
}

VOID
LsaSrvLogServiceWarningEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory, 
    PCSTR pszDescription,
    PCSTR pszData
    )
{
    DWORD dwError = 0;
    
    dwError = LsaSrvLogWarningEvent(            
                  dwEventID,
                  NULL, // defaults to SYSTEM
                  pszEventCategory,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    
    return;

error:

    LSA_LOG_VERBOSE("Failed to post service warning event.");
    LSA_LOG_VERBOSE("Error code: [%u]", dwError);

    goto cleanup;
}

VOID
LsaSrvLogServiceFailureEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    )
{
    DWORD dwError = 0;
    
    dwError = LsaSrvLogErrorEvent(            
                  dwEventID,
                  NULL, // defaults to SYSTEM
                  pszEventCategory,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    return;

error:

    LSA_LOG_VERBOSE("Failed to post service failure event.");
    LSA_LOG_VERBOSE("Error code: [%u]", dwError);

    goto cleanup;
}

VOID
LsaSrvLogUserIDConflictEvent(
    uid_t uid,
    PCSTR pszProviderName,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszUserIDConflictDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszUserIDConflictDescription,
                 "Likewise account provisioning conflict.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Reason:                  Found duplicate entries for UIDs:\r\n" \
                 "     UID:                     %u",
                 pszProviderName,
                 uid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceWarningEvent(
            LSASS_EVENT_WARNING_CONFIGURATION_ID_CONFLICT,
            SERVICE_EVENT_CATEGORY,
            pszUserIDConflictDescription,
            pszData ? pszData : " ");

    LSA_LOG_WARNING(pszUserIDConflictDescription);

cleanup:

    LW_SAFE_FREE_STRING(pszUserIDConflictDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;

}

VOID
LsaSrvLogUserGIDConflictEvent(
    gid_t gid,
    PCSTR pszProviderName,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszUserGIDConflictDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszUserGIDConflictDescription,
                 "Likewise account provisioning conflict.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Reason:                  Found duplicate entries for GIDs:\r\n" \
                 "     GID:                     %u",
                 pszProviderName,
                 gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceWarningEvent(
            LSASS_EVENT_WARNING_CONFIGURATION_ID_CONFLICT,
            SERVICE_EVENT_CATEGORY,
            pszUserGIDConflictDescription,
            pszData ? pszData : " ");

    LSA_LOG_WARNING(pszUserGIDConflictDescription);

cleanup:

    LW_SAFE_FREE_STRING(pszUserGIDConflictDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;

}

VOID
LsaSrvLogUserAliasConflictEvent(
    PCSTR pszAlias,
    PCSTR pszProviderName,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszUserAliasConflictDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszUserAliasConflictDescription,
                 "Likewise account provisioning conflict.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Reason:                  Found duplicate entries for alias:\r\n" \
                 "     Alias:                   %s",
                 pszProviderName,
                 pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceWarningEvent(
            LSASS_EVENT_WARNING_CONFIGURATION_ALIAS_CONFLICT,
            SERVICE_EVENT_CATEGORY,
            pszUserAliasConflictDescription,
            pszData ? pszData : " ");

cleanup:

    LW_SAFE_FREE_STRING(pszUserAliasConflictDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;

}

VOID
LsaSrvLogDuplicateObjectFoundEvent(
    PCSTR pszName1,
    PCSTR pszName2,
    PCSTR pszProviderName,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszObjectDuplicateDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszObjectDuplicateDescription,
                 "Likewise account provisioning conflict\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Reason:                  Found duplicate entries for names:\r\n" \
                 "     Name 1:                  %s\r\n" \
                 "     Name 2:                  %s",
                 pszProviderName,
                 pszName1,
                 pszName2);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceWarningEvent(
            1020, // Lsass assigned object conflict event id
            SERVICE_EVENT_CATEGORY,
            pszObjectDuplicateDescription,
            pszData ? pszData : " ");

    LSA_LOG_WARNING(pszObjectDuplicateDescription);

cleanup:

    LW_SAFE_FREE_STRING(pszObjectDuplicateDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;

}

DWORD
LsaSrvStartEventLoggingThread(
    VOID
    )
{
    DWORD dwError = 0;
    char currentHost[129] = {0};

    gEventLogState.bShouldExit = FALSE;
    dwError = LwMapErrnoToLwError(pthread_create(
                    &gEventLogState.writerThread,
                    NULL,
                    LsaSrvEventWriterRoutine,
                    NULL));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(gethostname(
                    currentHost,
                    sizeof(currentHost) - 1));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    currentHost,
                    &gEventLogState.pszComputerName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaSrvStopEventLoggingThread(
    VOID
    )
{
    DWORD dwError = 0;
    PVOID pvReturned = NULL;

    if (pthread_mutex_lock(&gEventLogState.queueMutex))
    {
        // Only fails if the mutex was not properly initalized
        abort();
    }
    gEventLogState.bShouldExit = TRUE;
    if (pthread_cond_signal(&gEventLogState.wakeUp))
    {
        // POSIX says it never fails
        abort();
    }
    if (pthread_mutex_unlock(&gEventLogState.queueMutex))
    {
        // Only fails if the mutex was not properly initalized
        abort();
    }

    LW_SAFE_FREE_STRING(gEventLogState.pszComputerName);

    if (gEventLogState.writerThread != (pthread_t)-1)
    {
        dwError = LwMapErrnoToLwError(pthread_join(
                        gEventLogState.writerThread,
                        &pvReturned));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = (DWORD)(size_t)pvReturned;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
LsaSrvEmptyQueue(
    PEVENT_LOG_RECORD_QUEUE pWriteQueue
    )
{
    DWORD dwIndex = 0;

    // Prepare the queue for writing again, but leave its capacity as
    // is. This avoids extra reallocations the next time around.
    for (dwIndex = 0; dwIndex < pWriteQueue->sSize; dwIndex++)
    {
        LWIFreeEventRecordContents(&pWriteQueue->pRecords[dwIndex]);
    }
    pWriteQueue->sSize = 0;
}

DWORD
LsaSrvSendQueue(
    PHANDLE phEventLog,
    PEVENT_LOG_RECORD_QUEUE pWriteQueue
    )
{
    DWORD dwError = 0;

    if (*phEventLog == NULL)
    {
        dwError = LWIOpenEventLog(
                      NULL,             // Server name (defaults to local computer eventlogd)
                      phEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LWIWriteEventLogRecords(
                    *phEventLog,
                    pWriteQueue->sSize,
                    pWriteQueue->pRecords);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvEmptyQueue(pWriteQueue);

cleanup:
    return dwError;

error:
    if (*phEventLog != NULL)
    {
        LWICloseEventLog(*phEventLog);
        *phEventLog = NULL;
    }
    goto cleanup;
}

VOID *
LsaSrvEventWriterRoutine(
    PVOID pvUnused
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = NULL;
    PEVENT_LOG_RECORD_QUEUE pWriteQueue = NULL;
    BOOLEAN bInLock = 0;
    DWORD dwTry = 0;
    DWORD dwSendError = 0;
    struct timespec wakeTime = { 0 };

    if (pthread_mutex_lock(&gEventLogState.queueMutex))
    {
        // Only fails if the mutex was not properly initalized
        abort();
    }
    bInLock = TRUE;

    while (TRUE)
    {
        while (gEventLogState.pQueue->sSize)
        {
            // There are two queues in existence. This thread will take
            // ownership of the current queue and send its contents to the
            // event log in the background. The current thread pointer will be
            // swapped, and the other threads will write to the other queue.
            pWriteQueue = gEventLogState.pQueue;

            // If pQueue was at index 0, set it to index 1. If it was at index
            // 1, move it to index 0
            *(size_t *)&gEventLogState.pQueue ^= (size_t)&gEventLogQueues[0] ^
                            (size_t)&gEventLogQueues[1];
            // Let other threads continue queuing events
            if (pthread_mutex_unlock(&gEventLogState.queueMutex))
            {
                // Only fails if the mutex was not properly initalized
                abort();
            }
            bInLock = FALSE;

            dwTry = 0;
            while (pWriteQueue->sSize)
            {
                dwError = LsaSrvSendQueue(
                                &hEventLog,
                                pWriteQueue
                                );
                if (dwError)
                {
                    dwSendError = dwError;
                    dwError = 0;
                    dwTry++;

                    if (pthread_mutex_lock(&gEventLogState.queueMutex))
                    {
                        // Only fails if the mutex was not properly initalized
                        abort();
                    }
                    bInLock = TRUE;

                    wakeTime.tv_sec = time(NULL) + (dwTry < 10? dwTry : 10) + 1;

                    while (!gEventLogState.bShouldExit &&
                            time(NULL) < wakeTime.tv_sec)
                    {
                        dwError = pthread_cond_timedwait(&gEventLogState.wakeUp,
                                                  &gEventLogState.queueMutex,
                                                  &wakeTime);
                        if (dwError == EINTR || dwError == ETIMEDOUT)
                        {
                            dwError = 0;
                        }
                        dwError = LwMapErrnoToLwError(dwError);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                    if ((dwTry > 3 && gEventLogState.pQueue->sSize >
                                LSA_MAX_EVENT_ERROR_BACKLOG) ||
                            gEventLogState.bShouldExit)
                    {
                        LSA_LOG_ERROR("Dropping %d events after %d send attempts due to error %d while trying to talk to the eventlog.",
                                (int)gEventLogState.pQueue->sSize,
                                dwTry,
                                dwSendError);
                        LsaSrvEmptyQueue(pWriteQueue);
                    }

                    if (pthread_mutex_unlock(&gEventLogState.queueMutex))
                    {
                        // Only fails if the mutex was not properly initalized
                        abort();
                    }
                    bInLock = FALSE;
                }
            }

            if (pthread_mutex_lock(&gEventLogState.queueMutex))
            {
                // Only fails if the mutex was not properly initalized
                abort();
            }
            bInLock = TRUE;
        }

        if (gEventLogState.bShouldExit && !gEventLogState.pQueue->sSize)
        {
            break;
        }

        dwError = LwMapErrnoToLwError(
                        pthread_cond_wait(&gEventLogState.wakeUp,
                                          &gEventLogState.queueMutex));
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (bInLock)
    {
        if (!gEventLogQueues[0].sSize)
        {
            LW_SAFE_FREE_MEMORY(gEventLogQueues[0].pRecords);
            gEventLogQueues[0].sCapacity = 0;
        }
        if (!gEventLogQueues[1].sSize)
        {
            LW_SAFE_FREE_MEMORY(gEventLogQueues[1].pRecords);
            gEventLogQueues[1].sCapacity = 0;
        }
        if (pthread_mutex_unlock(&gEventLogState.queueMutex))
        {
            // Only fails if the mutex was not properly initalized
            abort();
        }
    }

    if (hEventLog)
    {
        LWICloseEventLog(hEventLog);
    }
    return (PVOID)(size_t)dwError;

error:
    LSA_LOG_ERROR("Aborting on fatal error in event log thread - %d", dwError);
    abort();

    goto cleanup;
}

DWORD
LsaSrvQueueEvent(
    PEVENT_LOG_RECORD pEvent
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = 0;
    PEVENT_LOG_RECORD pNewEvent = NULL;

    if (pthread_mutex_lock(&gEventLogState.queueMutex))
    {
        // Only fails if the mutex was not properly initalized
        abort();
    }
    bInLock = TRUE;

    if (gEventLogState.pQueue->sSize + 1 > gEventLogState.pQueue->sCapacity)
    {
        PEVENT_LOG_RECORD pNewRecords = NULL;
        size_t sNewCapacity = (gEventLogState.pQueue->sCapacity + 10) * 3 / 2;

        dwError = LwReallocMemory(
                        gEventLogState.pQueue->pRecords,
                        (PVOID*)&pNewRecords,
                        sNewCapacity *
                            sizeof(*gEventLogState.pQueue->pRecords));
        BAIL_ON_LSA_ERROR(dwError);
        gEventLogState.pQueue->pRecords = pNewRecords;
        gEventLogState.pQueue->sCapacity = sNewCapacity;
    }

    pNewEvent = gEventLogState.pQueue->pRecords + gEventLogState.pQueue->sSize;

    pNewEvent->dwEventRecordId = pEvent->dwEventRecordId;
    dwError = LwStrDupOrNull(
                    pEvent->pszEventTableCategoryId,
                    &pNewEvent->pszEventTableCategoryId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pEvent->pszEventType,
                    &pNewEvent->pszEventType);
    BAIL_ON_LSA_ERROR(dwError);

    pNewEvent->dwEventDateTime = pEvent->dwEventDateTime;
    
    if (pNewEvent->dwEventDateTime == 0)
    {
        time((time_t *)&pNewEvent->dwEventDateTime);
    }

    dwError = LwStrDupOrNull(
                    pEvent->pszEventSource,
                    &pNewEvent->pszEventSource);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pEvent->pszEventCategory,
                    &pNewEvent->pszEventCategory);
    BAIL_ON_LSA_ERROR(dwError);

    pNewEvent->dwEventSourceId = pEvent->dwEventSourceId;

    dwError = LwStrDupOrNull(
                    pEvent->pszUser,
                    &pNewEvent->pszUser);
    BAIL_ON_LSA_ERROR(dwError);

    if (pNewEvent->pszUser == NULL)
    {
        dwError = LwAllocateString(
                        "SYSTEM",
                        &pNewEvent->pszUser);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwStrDupOrNull(
                    pEvent->pszComputer,
                    &pNewEvent->pszComputer);
    BAIL_ON_LSA_ERROR(dwError);

    if (pNewEvent->pszComputer == NULL)
    {
        dwError = LwStrDupOrNull(
                        gEventLogState.pszComputerName,
                        &pNewEvent->pszComputer);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwStrDupOrNull(
                    pEvent->pszDescription,
                    &pNewEvent->pszDescription);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pEvent->pszData,
                    &pNewEvent->pszData);
    BAIL_ON_LSA_ERROR(dwError);

    gEventLogState.pQueue->sSize++;

    if (pthread_cond_signal(&gEventLogState.wakeUp))
    {
        // POSIX says it never fails
        abort();
    }

cleanup:
    if (bInLock && pthread_mutex_unlock(&gEventLogState.queueMutex))
    {
        // Can't fail if the lock succeeded
        abort();
    }
    return dwError;

error:
    goto cleanup;
}
