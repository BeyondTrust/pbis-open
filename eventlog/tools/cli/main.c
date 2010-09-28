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
 * Eventlog utility that retrieves events over RPC from the Eventlog service.
 *
 */
#include "includes.h"
#include <popt.h>

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#define ACTION_NONE 0
#define ACTION_SHOW 1
#define ACTION_TABLE 2
#define ACTION_COUNT 3
#define ACTION_EXPORT 4
#define ACTION_DELETE 5
#define ACTION_HELP 6
#define ACTION_LAST 7

#define TRY DCETHREAD_TRY
#define CATCH_ALL DCETHREAD_CATCH_ALL(THIS_CATCH)
#define ENDTRY DCETHREAD_ENDTRY

static
void
ShowUsage()
{
    printf("Usage: lw-eventlog-cli [-h] | { [-s [<sql_filter>|-]] | [-c [<sql_filter>|-]] | [-t [<sql_filter> | -]]\n");
    printf(" | [-e [<csv_path>|-]] | [-d [<sql_filter>|-]] }  <ip_address>\n\n");
    printf("\t-h\tShow help\n");
    printf("\t-s\tShows a detailed, human-readable listing of the records matching sql_filter, or - for all records.\n");
    printf("\t-t\tShows a summary table of the records matching sql_filter, or - for all records.\n");
    printf("\t-c\tShows a count of the number of records matching sql_filter, or - for all records.\n");
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
    printf("\tEventTableCategoryId (integer)\n");
    printf("\tEventRecordId (integer)\n");
    printf("\tEventType     (varchar(128))\n");
    printf("\tEventTime     (integer - seconds since Jan. 1 1970)\n");
    printf("\tEventSource   (varchar(256))\n");
    printf("\tEventCategory (varchar(128))\n");
    printf("\tEventSourceId (integer)\n");
    printf("\tUser          (varchar(128))\n");
    printf("\tComputer      (varchar(128))\n");
    printf("\tDescription   (TEXT))\n\n");
    printf("\tData          (varchar(128))\n\n");
}


DWORD
AddEventRecord(
    PEVENT_LOG_HANDLE pEventLogHandle,
    EVENT_LOG_RECORD eventRecord
    )
{
    return LWIWriteEventLogBase((HANDLE)pEventLogHandle, eventRecord);
}

DWORD
ParseFilter(
    PCSTR pszPseudoSqlFilter,
    PWSTR* ppwszSqlFilter
    )
{
    DWORD dwError = 0;
    PWSTR pwszSqlFilter = NULL;
    PCSTR pszSqlFilterDefault = "eventRecordId >= 0";

    if (pszPseudoSqlFilter == NULL ||
            *pszPseudoSqlFilter == '\0' ||
            strcmp(pszPseudoSqlFilter, "-") == 0)
    {
        dwError = LwAllocateWc16sPrintfW(
                    &pwszSqlFilter,
                    L"%s",
                    pszSqlFilterDefault);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        DWORD startID = 0;
        DWORD endID = 0;
        int   nRead = 0;

        nRead = sscanf(pszPseudoSqlFilter, "%d-%d", &startID, &endID);

        if (nRead > 0 && nRead != EOF && startID > 0)
        {
            if (endID > 0 && endID > startID) {
                dwError = LwAllocateWc16sPrintfW(
                            &pwszSqlFilter,
                            L"EventRecordId >= %d AND EventRecordID <= %d",
                            startID,
                            endID);
                BAIL_ON_EVT_ERROR(dwError);
            }
            else
            {
                dwError = LwAllocateWc16sPrintfW(
                            &pwszSqlFilter,
                            L"EventRecordId = %d",
                            startID);
                BAIL_ON_EVT_ERROR(dwError);
            }
        }
        else
        {
            dwError = LwAllocateWc16sPrintfW(
                        &pwszSqlFilter,
                        L"%s",
                        pszPseudoSqlFilter);
            BAIL_ON_EVT_ERROR(dwError);
        }
    }

    *ppwszSqlFilter = pwszSqlFilter;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszSqlFilter);
    *ppwszSqlFilter = NULL;
    goto cleanup;
}

int
main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = NULL;
    EVENT_LOG_RECORD* eventRecords = NULL;
    DWORD nRecords = 0;
    DWORD currentRecord = 0;
    DWORD nRecordsPerPage = 500;

    PCSTR pszHostname = NULL;

    DWORD action = ACTION_NONE;
    PCSTR pszExportPath = NULL;
    PCSTR pszSqlFilter = NULL;
    PWSTR pwszSqlFilter = NULL;
    FILE* fpExport = NULL;

    struct poptOption optionsTable[] =
    {
        {
            "help",
            'h',
            POPT_ARG_NONE,
            NULL,
            ACTION_HELP,
            "Show help",
            NULL
        },
        {
            "show",
            's',
            POPT_ARG_STRING,
            &pszSqlFilter,
            ACTION_SHOW,
            "Shows a detailed, human-readable listing of the records matching sql_filter, or - for all records.",
            "filter"
        },
        {
            "table",
            't',
            POPT_ARG_STRING,
            &pszSqlFilter,
            ACTION_TABLE,
            "Shows a summary table of the records matching sql_filter, or - for all records.",
            "filter"
        },
        {
            "count",
            'c',
            POPT_ARG_STRING,
            &pszSqlFilter,
            ACTION_COUNT,
            "Shows a count of the number of records matching sql_filter, or - for all records.",
            "filter"
        },
        {
            "export",
            'e',
            POPT_ARG_STRING,
            &pszExportPath,
            ACTION_EXPORT,
            "Exports CSV data from the database to the given path, or - to print to the command line",
            "path"
        },
        {
            "delete",
            'd',
            POPT_ARG_STRING,
            &pszSqlFilter,
            ACTION_DELETE,
            "Deletes the records matching sql_filter, or - to delete all records, re-initializing the database",
            "filter"
        },
        POPT_AUTOHELP
        { NULL, 0, 0, NULL, 0}
    };

    poptContext optCon = { 0 };

    optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
    if (!optCon)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_EVT_ERROR(dwError);
    }

    poptSetOtherOptionHelp(optCon, "[OPTION] <hostname>");

    dwError = EVTInitLoggingToFile( LOG_LEVEL_ERROR,
                                    NULL);
    BAIL_ON_EVT_ERROR(dwError);

    action = poptGetNextOpt(optCon);

    // Allow exactly one option
    if (action == -1 || poptGetNextOpt(optCon) != -1)
    {
        ShowUsage();
        exit(1);
    }

    if (action == ACTION_HELP)
    {
        ShowUsage();
        exit(0);
    }

    pszHostname = poptGetArg(optCon);
    if (pszHostname == NULL)
    {
        fprintf(stderr, "Error: missing hostname\n");
        ShowUsage();
        exit(1);
    }

    if (poptGetArg(optCon) != NULL)
    {
        fprintf(stderr, "Error: more than one hostname specified\n");
        ShowUsage();
        exit(1);
    }

    if (action <= ACTION_NONE || action > ACTION_LAST) {
        EVT_LOG_VERBOSE("Invalid action: %d\n", action);
        ShowUsage();
        exit(0);
    }

    dwError = LWIOpenEventLog(pszHostname, (PHANDLE)&pEventLogHandle);
    BAIL_ON_EVT_ERROR(dwError);

    if (action == ACTION_EXPORT)
    {
        if (strcmp(pszExportPath, "-") == 0)
        {
            fpExport = stdout;
        }
        else
        {
            fpExport = fopen(pszExportPath, "w");
        }

        if (fpExport != NULL)
        {
            dwError = ReadAndExportEvents(pEventLogHandle, fpExport);
        }
        else
        {
            dwError = -1;
            EVT_LOG_VERBOSE("Unable to open file %s for writing.\n", pszExportPath);
        }
        BAIL_ON_EVT_ERROR(dwError);
    }
    else if (action == ACTION_COUNT)
    {
        dwError = ParseFilter(
            pszSqlFilter,
            &pwszSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LWICountEventLog((HANDLE)pEventLogHandle,
                        pwszSqlFilter,
                        &nRecords);


        BAIL_ON_EVT_ERROR(dwError);
        printf("%d records found in database\n", nRecords);
    }
    else if (action == ACTION_SHOW)
    {
        dwError = ParseFilter(
            pszSqlFilter,
            &pwszSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        do {

            dwError = LWIReadEventLog((HANDLE)pEventLogHandle,
                            currentRecord,
                            nRecordsPerPage,
                            pwszSqlFilter,
                            &nRecords,
                            &eventRecords);

            BAIL_ON_EVT_ERROR(dwError);

            PrintEventRecords(stdout, eventRecords, nRecords, &currentRecord);
        } while (nRecords == nRecordsPerPage && nRecords > 0);
    }
    else if (action == ACTION_TABLE)
    {
        dwError = ParseFilter(
            pszSqlFilter,
            &pwszSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        do {

            dwError = LWIReadEventLog((HANDLE)pEventLogHandle,
                            currentRecord,
                            nRecordsPerPage,
                            pwszSqlFilter,
                            &nRecords,
                            &eventRecords);

            BAIL_ON_EVT_ERROR(dwError);

            PrintEventRecordsTable(stdout, eventRecords, nRecords, &currentRecord);

        } while (nRecords == nRecordsPerPage && nRecords > 0);
    }
    else if (action == ACTION_DELETE)
    {
        dwError = ParseFilter(
            pszSqlFilter,
            &pwszSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LWIDeleteFromEventLog(
                        (HANDLE)pEventLogHandle,
                        pwszSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        EVT_LOG_VERBOSE("Invalid action: %d\n", action);
        ShowUsage();
        exit(0);
    }

 error:
    if (dwError != 0)
    {
        EVT_LOG_ERROR("The operation failed with error code [%d]\n", dwError);
    }
    LW_SAFE_FREE_MEMORY(pwszSqlFilter);
    if (pEventLogHandle)
    {
        LWICloseEventLog((HANDLE)pEventLogHandle);
    }
    if (eventRecords)
    {
        RPCFreeMemory(eventRecords);
        EVTFreeMemory(eventRecords);
    }
    poptFreeContext(optCon);
    if (fpExport)
    {
        fclose(fpExport);
    }

    return dwError;
}
