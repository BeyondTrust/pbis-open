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
 * Eventlog parser to parse log files from Microsoft Event Viewer.
 *
 */
#include "cltr-base.h"

#define TABLE_ID_WIDTH          5
#define TABLE_TYPE_WIDTH        11
#define TABLE_DATE_WIDTH        10
#define TABLE_TIME_WIDTH        11
#define TABLE_SOURCE_WIDTH      25
#define TABLE_CATEGORY_WIDTH    4
#define TABLOC_BORDER TEXT(" | ")

static
DWORD
PrintEventRecordTableRow (
    PLWCOLLECTOR_RECORD pRecord,
    FILE* fp
    )
{

    DWORD dwError = 0;
    time_t eventTimeStruct = 0;
    wchar_t eventDate[256];
    wchar_t eventTime[256];
    WCHAR szNULL[] = {'<','n','u','l','l','>',0};

    if (pRecord == NULL) return -1;
    if (fp == NULL) return -1;

    //TableRow fields: RecordID,Type,Date,Time,Source,Category

    eventTimeStruct = (time_t) pRecord->event.dwEventDateTime;

    if (wcsftime(
                eventDate,
                sizeof(eventDate)/sizeof(eventDate[0]),
                L"%d/%m/%Y",
                localtime(&eventTimeStruct)) < 1)
    {
        dwError = LW_STATUS_BUFFER_OVERFLOW;
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (wcsftime(
                eventTime,
                sizeof(eventTime)/sizeof(eventTime[0]),
                L"%H:%M:%S",
                localtime(&eventTimeStruct)) < 1)
    {
        dwError = LW_STATUS_BUFFER_OVERFLOW;
        BAIL_ON_CLTR_ERROR(dwError);
    }
    fw16printfw(fp,
        L"%-*lld" L"%hhs%-*ws" L"%hhs%-*ws" L"%hhs%-*ws" L"%hhs%-*ws" L"%hhs%-*ws\n",
        TABLE_ID_WIDTH,
        pRecord->event.qwEventRecordId,
        TABLOC_BORDER, TABLE_TYPE_WIDTH,
        IsNullOrEmptyString(pRecord->event.pwszEventSource) ? szNULL : (PWSTR)pRecord->event.pwszEventType,
        TABLOC_BORDER, TABLE_DATE_WIDTH,
        eventDate,
        TABLOC_BORDER, TABLE_TIME_WIDTH,
        eventTime,
        TABLOC_BORDER, TABLE_SOURCE_WIDTH,
        IsNullOrEmptyString(pRecord->event.pwszEventSource) ? szNULL : pRecord->event.pwszEventSource,
        TABLOC_BORDER, TABLE_CATEGORY_WIDTH,
        IsNullOrEmptyString(pRecord->event.pwszEventSource) ? szNULL : pRecord->event.pwszEventCategory);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
PrintEventRecords(
    FILE* output,
    LWCOLLECTOR_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    wchar_t eventDate[256];
    wchar_t eventTime[256];
    wchar_t cltrDate[256];
    wchar_t cltrTime[256];
    WCHAR wszNULL[] = {'<','n','u','l','l','>',0};

    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;
    DWORD iRecord = 0; //changing from int to DWORD

    for (iRecord = 0; iRecord < nRecords; iRecord++)
    {
        LWCOLLECTOR_RECORD* pRecord = &(eventRecords[iRecord]);

        time_t eventTimeT = (time_t) pRecord->event.dwEventDateTime;
        time_t cltrTimeT = (time_t) pRecord->dwColDateTime;

        if (wcsftime(
                    eventDate,
                    sizeof(eventDate)/sizeof(eventDate[0]),
                    L"%d/%m/%Y",
                    localtime(&eventTimeT)) < 1)
        {
            dwError = LW_STATUS_BUFFER_OVERFLOW;
            BAIL_ON_CLTR_ERROR(dwError);
        }
        if (wcsftime(
                    eventTime,
                    sizeof(eventTime)/sizeof(eventTime[0]),
                    L"%H:%M:%S",
                    localtime(&eventTimeT)) < 1)
        {
            dwError = LW_STATUS_BUFFER_OVERFLOW;
            BAIL_ON_CLTR_ERROR(dwError);
        }
        if (wcsftime(
                    cltrDate,
                    sizeof(cltrDate)/sizeof(cltrDate[0]),
                    L"%d/%m/%Y",
                    localtime(&cltrTimeT)) < 1)
        {
            dwError = LW_STATUS_BUFFER_OVERFLOW;
            BAIL_ON_CLTR_ERROR(dwError);
        }
        if (wcsftime(
                    cltrTime,
                    sizeof(cltrTime)/sizeof(cltrTime[0]),
                    L"%H:%M:%S",
                    localtime(&cltrTimeT)) < 1)
        {
            dwError = LW_STATUS_BUFFER_OVERFLOW;
            BAIL_ON_CLTR_ERROR(dwError);
        }

        printf("Event Record: (%d/%d) (%d total)\n", iRecord+1, nRecords, ++totalRecordsLocal);
        printf("========================================\n");
        w16printfw(L"Collector Record ID..... %llu\n", (long long unsigned int)pRecord->qwColRecordId);
        w16printfw(L"Collection Date......... %ls\n", cltrDate);
        w16printfw(L"Collection Time......... %ls\n", cltrTime);
        w16printfw(L"Forwarded from.......... %ws\n",
                IsNullOrEmptyString(pRecord->pwszColComputer) ? wszNULL : (WCHAR*) (pRecord->pwszColComputer));
        w16printfw(L"Forwarded from Address.. %ws\n",
                IsNullOrEmptyString(pRecord->pwszColComputerAddress) ? wszNULL : (WCHAR*) (pRecord->pwszColComputerAddress));

        printf("Event Record ID......... %llu\n", (long long unsigned int)pRecord->event.qwEventRecordId);
        w16printfw(L"Event Logname........... %ws\n",
                IsNullOrEmptyString(pRecord->event.pwszLogname) ? wszNULL : pRecord->event.pwszLogname);
        w16printfw(L"Event Type.............. %ws\n",
                IsNullOrEmptyString(pRecord->event.pwszEventType) ? wszNULL : pRecord->event.pwszEventType);
        w16printfw(L"Event Date.............. %ls\n", eventDate);
        w16printfw(L"Event Time.............. %ls\n", eventTime);
        w16printfw(L"Event Source............ %ws\n",
                IsNullOrEmptyString(pRecord->event.pwszEventSource) ? wszNULL : pRecord->event.pwszEventSource);
        w16printfw(L"Event Category.......... %ws\n",
                IsNullOrEmptyString(pRecord->event.pwszEventCategory) ? wszNULL : pRecord->event.pwszEventCategory);
        printf("Event Source ID......... %d\n", pRecord->event.dwEventSourceId);
        w16printfw(L"Event User.............. %ws\n",
                IsNullOrEmptyString(pRecord->event.pwszUser) ? wszNULL : (WCHAR*) (pRecord->event.pwszUser));
        w16printfw(L"Event Computer.......... %ws\n",
                IsNullOrEmptyString(pRecord->event.pwszComputer) ? wszNULL : (WCHAR*) (pRecord->event.pwszComputer));
        w16printfw(L"Event Description....... %ws\n",
                IsNullOrEmptyString(pRecord->event.pwszDescription) ? wszNULL : (WCHAR*) (pRecord->event.pwszDescription));
        printf("========================================\n");
    }

    *totalRecords = totalRecordsLocal;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
PrintEventRecordsTable(
    FILE* output,
    LWCOLLECTOR_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;

    fw16printfw(
            output,
            L"%-*hhs" L"%hhs%-*hhs" L"%hhs%-*hhs" L"%hhs%-*hhs" L"%hhs%-*hhs" L"%hhs%-*hhs" L"\n",
            TABLE_ID_WIDTH,
            "Id",
            TABLOC_BORDER, TABLE_TYPE_WIDTH,
            "Type",
            TABLOC_BORDER, TABLE_DATE_WIDTH,
            "Date",
            TABLOC_BORDER, TABLE_TIME_WIDTH,
            "Time",
            TABLOC_BORDER, TABLE_SOURCE_WIDTH,
            "Source",
            TABLOC_BORDER, TABLE_CATEGORY_WIDTH,
            "Category");

    for (i = 0; i < nRecords; i++) 
    {
        dwError = PrintEventRecordTableRow(&(eventRecords[i]), output);
        BAIL_ON_CLTR_ERROR(dwError);
        totalRecordsLocal++;
    }


error:

    *totalRecords = totalRecordsLocal;

    return dwError;
}
