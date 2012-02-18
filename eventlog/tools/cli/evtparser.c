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
 *
 * Eventlog parser to parse log files from Microsoft Event Viewer.
 *
 */
#include "includes.h"

static
DWORD
ExportEventRecord (
    PLW_EVENTLOG_RECORD pRecord,
    FILE* fp
    )
{
    DWORD dwError = 0;
    WCHAR pNullName[] = { '<', 'n', 'u', 'l', 'l', '>', 0 };
    time_t eventTimeStruct = (time_t) -1;

    char eventDate[256];
    char eventTime[256];

    if (pRecord == NULL || fp == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    /*
     * CSV fields:
     *    LogfileName,Date,Time,Source,Type,Category,SourceID,User,Computer,Description,Data
     */
    eventTimeStruct = (time_t) pRecord->EventDateTime;

    strftime(eventDate, 255, "%Y-%m-%d", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    fw16printfw(
                    fp, 
                    L"%ws,%hhs,%hhs,%ws,%ws,%ws,%d,%ws,%ws,\"%ws\",\"%hhs\"\n",
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pLogname) ?
                        pNullName : pRecord->pLogname,
                    eventDate, //PSTR
                    eventTime, //PSTR
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventSource) ?
                        pNullName : pRecord->pEventSource,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventType) ?
                        pNullName : pRecord->pEventType,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventCategory) ?
                        pNullName : pRecord->pEventCategory,
                    pRecord->EventSourceId, //DWORD
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pUser) ?
                        pNullName : pRecord->pUser,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pComputer) ?
                        pNullName : pRecord->pComputer,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pDescription) ?
                        pNullName : pRecord->pDescription,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pData) ?
                        "<null>" : (PSTR)pRecord->pData);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    return dwError;
    
error:
    goto cleanup;
}

static
DWORD
PrintEventRecordTableRow (
    PLW_EVENTLOG_RECORD pRecord,
    FILE* fp
    )
{

    DWORD dwError = 0;
    WCHAR pNullName[] = { '<', 'n', 'u', 'l', 'l', '>', 0 };
    char eventDate[256];
    char eventTime[256];
    time_t eventTimeStruct = (time_t)-1;

    if (pRecord == NULL || fp == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    eventTimeStruct = (time_t) pRecord->EventDateTime;

    strftime(eventDate, 255, "%Y-%m-%d", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    fw16printfw(
                    fp,
                    L"%8llu | %13ws | %10s | %11s | %17ws | %22ws | %7d | %ws\n",
                    (unsigned long long)pRecord->EventRecordId,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventType) ?
                        pNullName : pRecord->pEventType,
                    eventDate,
                    eventTime,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventSource) ?
                        pNullName : pRecord->pEventSource,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventCategory) ?
                        pNullName : pRecord->pEventCategory,
                    pRecord->EventSourceId,
                    LW_IS_NULL_OR_EMPTY_STR(pRecord->pUser) ?
                        pNullName : pRecord->pUser);

cleanup:
    return dwError;
    
error:
    goto cleanup;
}

DWORD
PrintEventRecords(
    FILE* output,
    LW_EVENTLOG_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    char eventDate[256];
    char eventTime[256];
    WCHAR pNullName[] = { '<', 'n', 'u', 'l', 'l', '>', 0 };

    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;
    int iRecord = 0;

    for (iRecord = 0; iRecord < nRecords; iRecord++)
    {
        PLW_EVENTLOG_RECORD pRecord = &(eventRecords[iRecord]);
        time_t eventTimeStruct = (time_t) pRecord->EventDateTime;

        strftime(eventDate, 255, "%Y-%m-%d", localtime(&eventTimeStruct));
        strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

        printf("Event Record: (%d/%d) (%d total)\n",
                iRecord+1, nRecords, ++totalRecordsLocal);
        printf("========================================\n");
        printf("Event Record ID......... %llu\n",
                (unsigned long long)pRecord->EventRecordId);
        fw16printfw(
                stdout,
                L"Event Table Category.... %ws\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pLogname) ?
                    pNullName : pRecord->pLogname);
        fw16printfw(
                stdout, 
                L"Event Type.............. %ws\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventType) ?
                    pNullName : pRecord->pEventType);
        printf("Event Date.............. %s\n", eventDate);
        printf("Event Time.............. %s\n", eventTime);
        fw16printfw(
                stdout, 
                L"Event Source............ %ws\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventSource) ?
                    pNullName : pRecord->pEventSource);
        fw16printfw(
                stdout, 
                L"Event Category.......... %ws\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pEventCategory) ?
                    pNullName : pRecord->pEventCategory);
        printf("Event Source ID......... %d\n", pRecord->EventSourceId);
        fw16printfw(
                stdout, 
                L"Event User.............. %ws\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pUser) ?
                    pNullName : pRecord->pUser);
        fw16printfw(
                stdout, 
                L"Event Computer.......... %ws\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pComputer) ?
                    pNullName : pRecord->pComputer);
        fw16printfw(
                stdout, 
                L"Event Description....... %ws\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pDescription) ?
                    pNullName : pRecord->pDescription);
        printf("Event Data.............. %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pRecord->pData) ?
                    "<null>" : (char*) (pRecord->pData));
        printf("========================================\n");

    }

    *totalRecords = totalRecordsLocal;

    return dwError;
}

DWORD
PrintEventRecordsTable(
    FILE* output,
    LW_EVENTLOG_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;

    fprintf(
            output,
            "%8s | %13s | %10s | %11s | %17s | %22s | %7s | %s\n",
            "Id",
            "Type",
            "Date",
            "Time",
            "Source",
            "Category",
            "Event",
            "User");

    for (i = 0; i < nRecords; i++)
    {
        dwError = PrintEventRecordTableRow(&(eventRecords[i]), output);
        BAIL_ON_EVT_ERROR(dwError);
        totalRecordsLocal++;
    }


 error:

    *totalRecords = totalRecordsLocal;

    return dwError;
}

DWORD
ReadAndExportEvents(
    PLW_EVENTLOG_CONNECTION pEventLogHandle,
    PCWSTR pwszSqlFilter,
    FILE* fpExport
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    const DWORD pageSize = 2000;
    DWORD entriesRead = 0;
    PLW_EVENTLOG_RECORD records = NULL;
    UINT64 startRecordId = 0;
    PWSTR pFilter = NULL;

    if (fpExport == NULL) return -1;
    if (pEventLogHandle == NULL) return -1;

    /*
     * CSV fields:
     *    LogfileName,Date,Time,Source,Type,Category,SourceID,User,Computer,Description,Data
     */
    fprintf(fpExport, "LogfileName,");
    fprintf(fpExport, "Date,");
    fprintf(fpExport, "Time,");
    fprintf(fpExport, "Source,");
    fprintf(fpExport, "Type,");
    fprintf(fpExport, "Category,");
    fprintf(fpExport, "SourceId,");
    fprintf(fpExport, "User,");
    fprintf(fpExport, "Computer,");
    fprintf(fpExport, "Description,");
    fprintf(fpExport, "Data\n");

    do
    {
        LW_SAFE_FREE_MEMORY(pFilter);
        dwError = LwAllocateWc16sPrintfW(
                        &pFilter,
                        L"(%ws) AND EventRecordId >= %llu",
                        pwszSqlFilter,
                        (long long unsigned)startRecordId);
        BAIL_ON_EVT_ERROR(dwError);

        if (records)
        {
            LwEvtFreeRecordArray(
                entriesRead,
                records);
            records = NULL;
        }
        dwError = LwEvtReadRecords(
                    pEventLogHandle,
                    pageSize,
                    pFilter,
                    &entriesRead,
                    &records);
        BAIL_ON_EVT_ERROR(dwError);

        for (i = 0; i < entriesRead; i++)
        {
            startRecordId = records[i].EventRecordId + 1;
            dwError = ExportEventRecord(&(records[i]), fpExport);
            BAIL_ON_EVT_ERROR(dwError);
        }

        fflush(fpExport);
    } while (entriesRead == pageSize && entriesRead > 0);

 cleanup:

    LW_SAFE_FREE_MEMORY(pFilter);
    if (records)
    {
        LwEvtFreeRecordArray(
            entriesRead,
            records);
    }

    return dwError;

error:

    goto cleanup;

}

