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
 * Eventlog utility that retrieves events over RPC from the Eventlog service.
 *
 */

#define _CRT_SECURE_NO_WARNINGS
#include "includes.h"

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#define ACTION_NONE 0
#define ACTION_SHOW 1
#define ACTION_TABLE 2
#define ACTION_COUNT 3
#define ACTION_DELETE 6
#define ACTION_LAST 6

int
wmain(
    int argc,
    WCHAR* argv[]
    );

static
void
ShowUsage()
{
    printf("Usage: lw-eventlog-cli [-h] | { [-s [<sql_filter>|-]] | [-c [<sql_filter>|-]] | [-t [<sql_filter> | -]]\n");
    printf(" | [-i <csv_path> [<table_category_id>]] | [-e [<csv_path>|-]] | [-d [<sql_filter>|-]] }  <ip_address>\n\n");
    printf("\t-h\tShow help\n");
    printf("\t-s\tShows a detailed, human-readable listing of the records matching sql_filter, or - for all records.\n");
    printf("\t-t\tShows a summary table of the records matching sql_filter, or - for all records.\n");
    printf("\t-c\tShows a count of the number of records matching sql_filter, or - for all records.\n");
    printf("\t-i\tImports CSV data to the database from the given path\n");
    printf("\t-e\tExports CSV data from the database to the given path, or - to print to the command line\n");
    printf("\t-d\tDeletes the records matching sql_filter, or - to delete all records, re-initializing the database\n");
    printf("\nExamples:\n");
    printf("\tPrints details for events 1-10:\n");
    printf("\tlw-eventlog-cli -s 1-10 127.0.0.1\n\n");
    printf("\tPrints table for all events:\n");
    printf("\tlw-eventlog-cli -t - 127.0.0.1\n\n");
    printf("\tCount all events matching the SQL WHILE expression:\n");
    printf("\tlw-eventlog-cli -c \"(EventRecordId < 1000) AND (EventType = 'Warning')\" 127.0.0.1\n");
    printf("\n");
    printf("Valid Field Names:\n");
    printf("\tEventLogname  (varchar(128))\n");
    printf("\tEventTableCategoryId (integer)\n");
    printf("\tEventRecordId (integer)\n");
    printf("\tEventType     (varchar(128))\n");
    printf("\tEventTime     (integer - seconds since Jan. 1 1970)\n");
    printf("\tEventSource   (varchar(256))\n");
    printf("\tEventCategory (varchar(128))\n");
    printf("\tEventSourceId (integer)\n");
    printf("\tUser          (varchar(128))\n");
    printf("\tComputer      (varchar(128))\n");
    printf("\tDescription   (varchar(256))\n\n");

}


DWORD
AddEventRecord(
    PCOLLECTOR_HANDLE pEventLogHandle,
    LWCOLLECTOR_RECORD eventRecord
    )
{
    return CltrWriteRecords((HANDLE)pEventLogHandle, 1, &eventRecord);
}


static
DWORD
ParseArgs(
    int argc,
    WCHAR* argv[],
    PDWORD action,
    WCHAR** ppwszArgCopy,
    PWSTR* ppwszIpAddress,
    PDWORD pdwEventTableCategoryId,
    PBOOLEAN pbEventTableCategoryIdInCSV)
{
    int iArg = 1;
    DWORD maxIndex = argc-1;
    PWSTR pwszArg = NULL;
    PWSTR pwszArgLocal = NULL;

    DWORD dwError = 0;
    DWORD actionLocal = 0;
    DWORD dwEventTableCategoryId = 0;
    BOOLEAN bEventTableCategoryIdInCSV = TRUE;

    if (iArg > (int)maxIndex || maxIndex > 8) {
        ShowUsage();
        exit(0);
    }

    pwszArg = argv[iArg++];
    if (pwszArg == NULL || *pwszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else if ((CltrWC16StringCompareWCNString(pwszArg, L"--help", 1) == 0) ||
            (CltrWC16StringCompareWCNString(pwszArg, L"-h", 1) == 0))
    {
        ShowUsage();
        exit(0);
    }
    else if (CltrWC16StringCompareWCNString(pwszArg, L"-s", 1) == 0)
    {
        actionLocal = ACTION_SHOW;
    }
    else if (CltrWC16StringCompareWCNString(pwszArg, L"-t", 1) == 0)
    {
        actionLocal = ACTION_TABLE;
    }
    else if (CltrWC16StringCompareWCNString(pwszArg, L"-c", 1) == 0)
    {
        actionLocal = ACTION_COUNT;
    }
    else if (CltrWC16StringCompareWCNString(pwszArg, L"-d", 1) == 0)
    {
        actionLocal = ACTION_DELETE;
    }
    else
    {
        ShowUsage();
        exit(0);
    }

    if (iArg > (int)maxIndex) {
        ShowUsage();
        exit(0);
    }

    pwszArg = argv[iArg++];
    if (pwszArg == NULL || *pwszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else
    {
        dwError = LwRtlWC16StringDuplicate(
                        &pwszArgLocal,
                        pwszArg);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    while (iArg+2 <= (int)maxIndex)
    {
        pwszArg = argv[iArg++];
        if (pwszArg == NULL || *pwszArg == '\0')
        {
            ShowUsage();
            exit(0);
        }
        else {
            ShowUsage();
            exit(0);
        }
    }

    pwszArg = argv[iArg++];
    if (pwszArg == NULL || *pwszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else
    {
        dwError = LwRtlWC16StringDuplicate(
                        ppwszIpAddress,
                        pwszArg);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    *action = actionLocal;
    *ppwszArgCopy = pwszArgLocal;
    *pdwEventTableCategoryId = dwEventTableCategoryId;
    *pbEventTableCategoryIdInCSV = bEventTableCategoryIdInCSV;

cleanup:

    return dwError;

error:
    *action = 0;
    *ppwszArgCopy = NULL;
    *pdwEventTableCategoryId = 0;
    *pbEventTableCategoryIdInCSV = 0;
    LwRtlWC16StringFree(&pwszArgLocal);

    goto cleanup;
}

#ifndef _WIN32
int main(
    int argc,
    char* ppszArgv[]
)
{
    DWORD dwError = 0;
    PWSTR* ppwszArgv = NULL;
    DWORD dwIndex = 0;

    dwError = CltrAllocateMemory(
                    argc * sizeof(ppwszArgv[0]),
                    (PVOID*)&ppwszArgv);
    BAIL_ON_CLTR_ERROR(dwError);

    for (dwIndex = 0; dwIndex < argc; dwIndex++)
    {
        dwError = LwRtlWC16StringAllocatePrintfW(
                        &ppwszArgv[dwIndex],
                        L"%ls",
                        ppszArgv[dwIndex]);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = wmain(argc, ppwszArgv);

cleanup:
    if (ppwszArgv)
    {
        for (dwIndex = 0; dwIndex < argc; dwIndex++)
        {
            CLTR_SAFE_FREE_STRING(ppwszArgv[dwIndex]);
        }
        CLTR_SAFE_FREE_MEMORY(ppwszArgv);
    }

    return dwError;

error:
    goto cleanup;
}
#endif

int
wmain(
    int argc,
    WCHAR* argv[]
    )
{
    DWORD dwError = 0;
    PCOLLECTOR_HANDLE pEventLogHandle = NULL;
    LWCOLLECTOR_RECORD* eventRecords = NULL;
    DWORD nRecords = 0;
    DWORD currentRecord = 0;
    DWORD nRecordsPerPage = 500;
    DWORD dwEventTableCategoryId = 0;
    BOOLEAN bEventTableCategoryIdInCSV = FALSE;
    ACCESS_TOKEN access = NULL;

    PWSTR pwszIpAddress = NULL;

    PCSTR pszSqlFilterCharDefault = "eventRecordId >= 0";
    wchar_t *pwszSqlFilterDefault = L"eventRecordId >= 0";
    WCHAR* pwszSqlFilter = NULL;

    PWSTR pwszArgCopy = NULL;
    PSTR pszArgCopy = NULL;
    DWORD action = ACTION_NONE;

    dwError = ParseArgs(
                argc,
                argv,
                &action,
                &pwszArgCopy,
                &pwszIpAddress,
                &dwEventTableCategoryId,
                &bEventTableCategoryIdInCSV
                );
    BAIL_ON_CLTR_ERROR(dwError);

    if (action <= ACTION_NONE || action > ACTION_LAST) {
        printf("Invalid action: %d\n", action);
        ShowUsage();
        exit(0);
    }
#ifdef _WIN32
    if (!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
            &access))
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }
#endif

    RpcTryExcept
    {
        dwError = CltrOpenCollector(
            pwszIpAddress,
            NULL,
            access,
            (PHANDLE)&pEventLogHandle);
        BAIL_ON_CLTR_ERROR(dwError);

        if (pwszArgCopy == NULL || *pwszArgCopy == '\0' ||
            (pwszArgCopy[0] == L'-' && pwszArgCopy[1] == 0))
        {
            dwError = LwRtlWC16StringAllocateFromCString(&pwszSqlFilter, pszSqlFilterCharDefault);
            BAIL_ON_CLTR_ERROR(dwError);
        }
        else {

            DWORD startID = 0;
            DWORD endID = 0;

            LwRtlCStringFree(&pszArgCopy);
            dwError = LwRtlCStringAllocateFromWC16String(&pszArgCopy, pwszArgCopy);
            BAIL_ON_CLTR_ERROR(dwError);

            sscanf(pszArgCopy, "%d-%d", &startID, &endID);

            if (startID > 0) {
                if (endID > 0 && endID > startID) {
                    dwError = LwRtlWC16StringAllocatePrintfW(
                                    &pwszSqlFilter,
                                    L"EventRecordId >= %d AND EventRecordID <= %d",
                                    startID,
                                    endID);
                    BAIL_ON_CLTR_ERROR(dwError);
                }
                else
                {
                    dwError = LwRtlWC16StringAllocatePrintfW(
                                    &pwszSqlFilter,
                                    L"EventRecordId = %d",
                                    startID);
                    BAIL_ON_CLTR_ERROR(dwError);
                }
            }
            else {
                pwszSqlFilter = pwszArgCopy;
                pwszArgCopy = 0;
            }
        }
        
        if (action == ACTION_COUNT)
        {
            dwError = CltrGetRecordCount((HANDLE)pEventLogHandle,
                pwszSqlFilter,
                &nRecords);


            BAIL_ON_CLTR_ERROR(dwError);
            printf("%d records found in database\n", nRecords);
        }
        else if (action == ACTION_SHOW)
        {
            do {

                dwError = CltrReadRecords((HANDLE)pEventLogHandle,
                    currentRecord,
                    nRecordsPerPage,
                    pwszSqlFilter,
                    &nRecords,
                    &eventRecords);

                BAIL_ON_CLTR_ERROR(dwError);

                PrintEventRecords(stdout, eventRecords, nRecords, &currentRecord);

            } while (nRecords == nRecordsPerPage && nRecords > 0);
        }
        else if (action == ACTION_TABLE)
        {
            do {

                dwError = CltrReadRecords((HANDLE)pEventLogHandle,
                    currentRecord,
                    nRecordsPerPage,
                    pwszSqlFilter,
                    &nRecords,
                    &eventRecords);

                BAIL_ON_CLTR_ERROR(dwError);

                PrintEventRecordsTable(stdout, eventRecords, nRecords, &currentRecord);

            } while (nRecords == nRecordsPerPage && nRecords > 0);
        }
        else if (action == ACTION_DELETE)
        {
            if (CltrWC16StringCompareWCNString(
                    pwszSqlFilter,
                    pwszSqlFilterDefault,
                    TRUE) == 0)
            {
                //This is probably a faster way of deleting
                //everything, because it drops the
                //table instead of running an SQL DELETE
                dwError = CltrClearRecords((HANDLE)pEventLogHandle);
            }
            else
            {
                dwError = CltrDeleteRecords(
                            (HANDLE)pEventLogHandle,
                            pwszSqlFilter);
            }
            BAIL_ON_CLTR_ERROR(dwError);
        }
        else
        {
            printf("Invalid action: %d\n", action);
            ShowUsage();
            exit(0);
        }
    }  //end TRY
    RpcExcept(1)
    {
        dwError = RpcExceptionCode();
        printf("Invalid Operation: %d\n", dwError);
    }
    RpcEndExcept

 cleanup:

#ifdef _WIN32
    if (access)
    {
        CloseHandle(access);
    }
#endif

    LwRtlCStringFree(&pszArgCopy);
    LwRtlWC16StringFree(&pwszIpAddress);

    if (pwszSqlFilter != NULL)
    {
        CltrFreeMemory(pwszSqlFilter);
    }

    if (pEventLogHandle)
    {
        CltrCloseCollector((HANDLE)pEventLogHandle);
    }


    if (eventRecords) {
        //RPCFreeMemory(eventRecords);
        CltrFreeMemory(eventRecords);
    }

    if (dwError != 0) {
        printf("Failed to dump events. Error code [%d]\n", dwError);
    }

    return dwError;

error:
    goto cleanup;
}
