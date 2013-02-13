/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 * Eventlog Legacy Client API
 *
 */
#include "includes.h"

DWORD
LWIOpenEventLog(
    PCSTR pszServerName,
    PHANDLE phEventLog
    )
{
    return LwEvtOpenEventlog(
                pszServerName,
                (PLW_EVENTLOG_CONNECTION*)phEventLog);
}

static
PCWSTR
EvtEmptyWc16sForNull(
    PCWSTR pInput
    )
{
    static const WCHAR pEmpty[] = { 0 };

    if (pInput == NULL)
    {
        return pEmpty;
    }
    return pInput;
}

DWORD
LWIReadEventLog(
    HANDLE hEventLog,
    DWORD dwLastRecordId,
    DWORD nRecordsPerPage,
    PCWSTR sqlFilter,
    PDWORD pdwNumReturned,
    EVENT_LOG_RECORD** eventRecords
    )
{
    DWORD dwError = 0;
    DWORD index = 0;
    DWORD count = 0;
    PLW_EVENTLOG_RECORD pRecords = NULL;
    EVENT_LOG_RECORD* pLegacyRecords = NULL;

    if (dwLastRecordId != 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LwEvtReadRecords(
        (PLW_EVENTLOG_CONNECTION)hEventLog,
        nRecordsPerPage,
        sqlFilter,
        &count,
        &pRecords);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(pLegacyRecords[0]) * count,
                    (PVOID)&pLegacyRecords);
    BAIL_ON_EVT_ERROR(dwError);

    for (index = 0; index < count; index++)
    {
        pLegacyRecords[index].dwEventRecordId = pRecords[index].EventRecordId;

        dwError = LwWc16sToMbs(
                        EvtEmptyWc16sForNull(pRecords[index].pLogname),
                        &pLegacyRecords[index].pszEventTableCategoryId);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwWc16sToMbs(
                        EvtEmptyWc16sForNull(pRecords[index].pEventType),
                        &pLegacyRecords[index].pszEventType);
        BAIL_ON_EVT_ERROR(dwError);

        pLegacyRecords[index].dwEventDateTime = pRecords[index].EventDateTime;

        dwError = LwWc16sToMbs(
                        EvtEmptyWc16sForNull(pRecords[index].pEventSource),
                        &pLegacyRecords[index].pszEventSource);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwWc16sToMbs(
                        EvtEmptyWc16sForNull(pRecords[index].pEventCategory),
                        &pLegacyRecords[index].pszEventCategory);
        BAIL_ON_EVT_ERROR(dwError);

        pLegacyRecords[index].dwEventSourceId = pRecords[index].EventSourceId;

        dwError = LwWc16sToMbs(
                        EvtEmptyWc16sForNull(pRecords[index].pUser),
                        &pLegacyRecords[index].pszUser);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwWc16sToMbs(
                        EvtEmptyWc16sForNull(pRecords[index].pComputer),
                        &pLegacyRecords[index].pszComputer);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwWc16sToMbs(
                        EvtEmptyWc16sForNull(pRecords[index].pDescription),
                        &pLegacyRecords[index].pszDescription);
        BAIL_ON_EVT_ERROR(dwError);

        // One extra byte to make sure the result is NULL terminated
        dwError = LwAllocateMemory(
                        pRecords[index].DataLen + 1,
                        (PVOID*)&pLegacyRecords[index].pszData);
        BAIL_ON_EVT_ERROR(dwError);

        memcpy(
                pLegacyRecords[index].pszData,
                pRecords[index].pData,
                pRecords[index].DataLen);
    }

    *pdwNumReturned = count;
    *eventRecords = pLegacyRecords;

cleanup:

    if (pRecords)
    {
        LwEvtFreeRecordArray(count, pRecords);
    }
    
    return dwError;

error:
    *pdwNumReturned = 0;
    *eventRecords = NULL;
    if (pLegacyRecords)
    {
        LWIFreeEventRecordList(
            count,
            pLegacyRecords);
    }
    goto cleanup;
}

DWORD
LWICountEventLog(
    HANDLE hEventLog,
    PCWSTR sqlFilter,
    DWORD* pdwNumMatched
    )
{
    return LwEvtGetRecordCount(
                (PLW_EVENTLOG_CONNECTION)hEventLog,
                sqlFilter,
                pdwNumMatched);
}

DWORD
LWIWriteEventLogBase(
    HANDLE hEventLog,
    EVENT_LOG_RECORD eventRecord
    )
{
    return LWIWriteEventLogRecords(
                hEventLog,
                1,
                &eventRecord);
}

DWORD
LWIWriteEventLogRecords(
    HANDLE hEventLog,
    DWORD cRecords,
    PEVENT_LOG_RECORD pEventRecords 
    )
{
    DWORD dwError = 0;
    DWORD index = 0;
    PLW_EVENTLOG_RECORD pNewRecords = NULL;

    dwError = LwAllocateMemory(
                    sizeof(pNewRecords[0]) * cRecords,
                    (PVOID)&pNewRecords);
    BAIL_ON_EVT_ERROR(dwError);

    for (index = 0; index < cRecords; index++)
    {
        pNewRecords[index].EventRecordId = pEventRecords[index].dwEventRecordId;

        dwError = LwMbsToWc16s(
                        pEventRecords[index].pszEventTableCategoryId,
                        &pNewRecords[index].pLogname);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pEventRecords[index].pszEventType,
                        &pNewRecords[index].pEventType);
        BAIL_ON_EVT_ERROR(dwError);

        pNewRecords[index].EventDateTime = pEventRecords[index].dwEventDateTime;
        if (pNewRecords[index].EventDateTime == 0)
        {
            pNewRecords[index].EventDateTime = (DWORD) time(NULL);
        }

        dwError = LwMbsToWc16s(
                        pEventRecords[index].pszEventSource,
                        &pNewRecords[index].pEventSource);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pEventRecords[index].pszEventCategory,
                        &pNewRecords[index].pEventCategory);
        BAIL_ON_EVT_ERROR(dwError);

        pNewRecords[index].EventSourceId = pEventRecords[index].dwEventSourceId;

        dwError = LwMbsToWc16s(
                        pEventRecords[index].pszUser,
                        &pNewRecords[index].pUser);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pEventRecords[index].pszComputer,
                        &pNewRecords[index].pComputer);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pEventRecords[index].pszDescription,
                        &pNewRecords[index].pDescription);
        BAIL_ON_EVT_ERROR(dwError);

        if (pEventRecords[index].pszData)
        {
            pNewRecords[index].DataLen =
                strlen(pEventRecords[index].pszData) + 1;
            dwError = LwAllocateString(
                            pEventRecords[index].pszData,
                            (PSTR*)&pNewRecords[index].pData);
            BAIL_ON_EVT_ERROR(dwError);
        }
    }

    dwError = LwEvtWriteRecords(
                    (PLW_EVENTLOG_CONNECTION)hEventLog,
                    cRecords,
                    pNewRecords);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    if (pNewRecords)
    {
        LwEvtFreeRecordArray(
            cRecords,
            pNewRecords);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
LWIDeleteFromEventLog(
    HANDLE hEventLog,
    PCWSTR sqlFilter
    )
{
    return LwEvtDeleteRecords(
                (PLW_EVENTLOG_CONNECTION)hEventLog,
                sqlFilter);
}

DWORD
LWICloseEventLog(
    HANDLE hEventLog
    )
{
    return LwEvtCloseEventlog((PLW_EVENTLOG_CONNECTION)hEventLog);
}

VOID
LWIFreeEventRecord(
    PEVENT_LOG_RECORD pEventRecord
    )
{
    LWIFreeEventRecordList(
        1,
        pEventRecord);
}

VOID
LWIFreeEventRecordContents(
    PEVENT_LOG_RECORD pEventRecord
    )
{
    if (pEventRecord)
    {
        LW_SAFE_FREE_STRING(pEventRecord->pszEventTableCategoryId);
        LW_SAFE_FREE_STRING(pEventRecord->pszEventType);
        LW_SAFE_FREE_STRING(pEventRecord->pszEventSource);
        LW_SAFE_FREE_STRING(pEventRecord->pszEventCategory);
        LW_SAFE_FREE_STRING(pEventRecord->pszUser);
        LW_SAFE_FREE_STRING(pEventRecord->pszComputer);
        LW_SAFE_FREE_STRING(pEventRecord->pszDescription);
        LW_SAFE_FREE_STRING(pEventRecord->pszData);
    }
}

VOID
LWIFreeEventRecordList(
    DWORD dwRecords,
    PEVENT_LOG_RECORD pEventRecordList
    )
{
    DWORD index = 0;

    if (pEventRecordList)
    {
        for (index = 0; index < dwRecords; index++)
        {
            LWIFreeEventRecordContents(&pEventRecordList[index]);
        }
        LW_SAFE_FREE_MEMORY(pEventRecordList);
    }
}
