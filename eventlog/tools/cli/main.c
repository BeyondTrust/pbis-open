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

#define ACTION_NONE 0
#define ACTION_SHOW 1
#define ACTION_TABLE 2
#define ACTION_COUNT 3
#define ACTION_EXPORT 4
#define ACTION_DELETE 5
#define ACTION_HELP 6
#define ACTION_LAST 7
#define ACTION_LIST 8

static
void
ShowUsage()
{
    fputs(
"Usage: eventlog-cli [-h] | --list | \n"
" { [-s <filter>] | [-c <filter>] | [-t <filter>] | [-d <filter>] }\n"
" { [-e [<csv_path>|-]] }\n"
" { [--days <int>] | [--hours <int>] | [--mins <int>] } <ip_address>\n"
"\n"
"   -h        Show help\n"
"   --list    List named SQL filters.\n"
"   -s        Shows a detailed, human-readable listing of the records matching\n"
"                filter.\n"
"   -t        Shows a summary table of the records matching filter.\n"
"   -c        Shows a count of the number of records matching filter.\n"
"   -d        Deletes the records matching filter.\n"
"   -e        Exports CSV data from the database to the given path, or - to\n"
"               print to the command line. May be combined with -s to filter\n"
"               records.\n"
"\n"
"   filter    Either an SQL filter, or a - for all records, or a named SQL\n"
"                filter.\n"
"\n"
"Examples:\n"
"   Prints details for events 1-10:\n"
"   eventlog-cli -s 1-10 127.0.0.1\n"
"\n"
"   Prints table for all events:\n"
"   eventlog-cli -t - 127.0.0.1\n\n"
"   Count all events matching the SQL WHILE expression:\n"
"   eventlog-cli -c \"(EventRecordId < 1000) AND (EventType = 'Warning')\" 127.0.0.1\n"
"\n"
"Valid Field Names:\n"
"    EventTableCategoryId (integer)\n"
"    EventRecordId (integer)\n"
"    EventType     (varchar(128))\n"
"    EventDateTime (integer - seconds since Jan. 1 1970)\n"
"    EventSource   (varchar(256))\n"
"    EventCategory (varchar(128))\n"
"    EventSourceId (integer)\n"
"    User          (varchar(128))\n"
"    Computer      (varchar(128))\n"
"    Description   (TEXT))\n"
"    Data          (varchar(128))\n\n",
    stdout);
}

DWORD
ParseFilter(
    PCSTR pszPseudoSqlFilter,
    PCSTR pszUserForFilter,
    DWORD dwSecondsForFilter,
    PWSTR* ppwszSqlFilter
    )
{
    DWORD dwError = 0;
    PWSTR pwszSqlFilter = NULL;

    PSTR pszXmlSqlFilter = NULL;
    PSTR pszUserFilter = NULL;
    PSTR pszDateFilter = NULL;
    PSTR ppszFilters[3];
    DWORD dwFilters = 0;
    size_t i;
    PSTR pszFinalFilter = NULL;
    DWORD dwFinalFilterLength = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszUserForFilter))
    {
        dwError = LwAllocateStringPrintf(
                    &pszUserFilter,
                    " User='%s' ",
                    pszUserForFilter);
        BAIL_ON_EVT_ERROR(dwError);

        ppszFilters[dwFilters++] = pszUserFilter;
    }

    if (dwSecondsForFilter != 0)
    {
        dwError = LwAllocateStringPrintf(
                    &pszDateFilter,
                    " EventDateTime >= %lu ",
                    time(NULL) - dwSecondsForFilter);
        BAIL_ON_EVT_ERROR(dwError);

        ppszFilters[dwFilters++] = pszDateFilter;
    }

    if (dwFilters)
    {
        for (i = 0; i < dwFilters; i++)
        {
            dwFinalFilterLength += strlen(" AND ");
            dwFinalFilterLength += strlen(ppszFilters[i]);
        }

        dwError = LwAllocateMemory(
                    sizeof(char) * (dwFinalFilterLength + 1),
                    (PVOID*)&pszFinalFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwFinalFilterLength = 0;
        for (i = 0; i < dwFilters; i++)
        {
            strcpy(pszFinalFilter + dwFinalFilterLength, " AND ");
            dwFinalFilterLength += strlen(" AND ");
            strcpy(pszFinalFilter + dwFinalFilterLength, ppszFilters[i]);
            dwFinalFilterLength += strlen(ppszFilters[i]);
        }
    }
    else
    {
        dwError = LwAllocateStringPrintf(&pszFinalFilter, "");
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (pszPseudoSqlFilter == NULL ||
            *pszPseudoSqlFilter == '\0' ||
            strcmp(pszPseudoSqlFilter, "-") == 0)
    {
        if (pszFinalFilter[0] != 0)
        {
            dwError = LwAllocateWc16sPrintfW(
                        &pwszSqlFilter,
                        L"1%s",
                        pszFinalFilter);
            BAIL_ON_EVT_ERROR(dwError);
        }
        pwszSqlFilter = NULL;
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
                            L"EventRecordId >= %d AND EventRecordID <= %d%s",
                            startID,
                            endID,
                            pszFinalFilter);
                BAIL_ON_EVT_ERROR(dwError);
            }
            else
            {
                dwError = LwAllocateWc16sPrintfW(
                            &pwszSqlFilter,
                            L"EventRecordId = %d%s",
                            startID,
                            pszFinalFilter);
                BAIL_ON_EVT_ERROR(dwError);
            }
        }
        else
        {
            dwError =XmlGetSqlQuery(pszPseudoSqlFilter, TRUE, &pszXmlSqlFilter);
            if (dwError == 0)
            {
                dwError = LwAllocateWc16sPrintfW(
                            &pwszSqlFilter,
                            L"%s%s",
                            pszXmlSqlFilter,
                            pszFinalFilter);
            }
            else if (dwError == APP_ERROR_REPORT_NOT_FOUND)
            {
                dwError = LwAllocateWc16sPrintfW(
                            &pwszSqlFilter,
                            L"(%s%s)",
                            pszPseudoSqlFilter,
                            pszFinalFilter);
            }
            BAIL_ON_EVT_ERROR(dwError);
        }
    }

    *ppwszSqlFilter = pwszSqlFilter;

cleanup:
    LW_SAFE_FREE_STRING(pszUserFilter);
    LW_SAFE_FREE_STRING(pszDateFilter);
    LW_SAFE_FREE_STRING(pszFinalFilter);

    LW_SAFE_FREE_STRING(pszXmlSqlFilter);

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
    PLW_EVENTLOG_CONNECTION pEventLogHandle = NULL;
    PLW_EVENTLOG_RECORD pEventRecords = NULL;
    DWORD nRecords = 0;
    UINT64 startRecordId = 0;
    DWORD currentRecord = 0;
    DWORD nRecordsPerPage = 500;

    PCSTR pszHostname = NULL;

    DWORD action = ACTION_NONE, dwFinalAction = ACTION_NONE;

    PCSTR pszExportPath = NULL;
    FILE* fpExport = NULL;

    PCSTR pszSqlFilter = NULL;
    PWSTR pwszSqlFilter = NULL;

    PSTR pszUserForFilter = NULL;
    DWORD dwDaysForFilter = 0;
    DWORD dwHoursForFilter = 0;
    DWORD dwMinutesForFilter = 0;
    DWORD dwSecondsForFilter = 0;
    PWSTR pFilter = NULL;

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
        {
            "user",
            '\0',
            POPT_ARG_STRING,
            &pszUserForFilter,
            ACTION_NONE,
            "Filter results to those items that match this user.",
            "filter"
        },
        {
            "days",
            '\0',
            POPT_ARG_LONG,
            &dwDaysForFilter,
            ACTION_NONE,
            "Filter results to those items in the past n days.",
            "filter"
        },
        {
            "hours",
            '\0',
            POPT_ARG_LONG,
            &dwHoursForFilter,
            ACTION_NONE,
            "Filter results to those items in the past n hours.",
            "filter"
        },
        {
            "mins",
            '\0',
            POPT_ARG_LONG,
            &dwMinutesForFilter,
            ACTION_NONE,
            "Filter results to those items in the past n minutes.",
            "filter"
        },
        {
            "list",
            'd',
            POPT_ARG_NONE,
            NULL,
            ACTION_LIST,
            "List named SQL filters.",
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

    while( (action = poptGetNextOpt(optCon)) != -1)
    {
        switch (action)
        {
            case ACTION_SHOW:
            case ACTION_TABLE:
            case ACTION_COUNT:
            case ACTION_DELETE:
                if (dwFinalAction == ACTION_NONE)
                {
                    dwFinalAction = action;
                }
                else if (dwFinalAction != ACTION_EXPORT)
                {
                    fprintf(stderr, "Use only one of -s, -t, -c, -d, -e, --list\n");
                    ShowUsage();
                    exit(1);
                }
                break;

            case ACTION_EXPORT:
                if (dwFinalAction == ACTION_SHOW ||
                    dwFinalAction == ACTION_TABLE ||
                    dwFinalAction == ACTION_COUNT ||
                    dwFinalAction == ACTION_DELETE)
                {
                    dwFinalAction = action;
                }
                else if (dwFinalAction == ACTION_EXPORT)
                {
                    fprintf(stderr, "Use only one -e\n");
                    ShowUsage();
                    exit(1);
                }
                else
                {
                    fprintf(stderr, "Use only one of -s, -t, -c, -d, -e, --list\n");
                    ShowUsage();
                    exit(1);
                }
                break;

            case ACTION_LIST:
                if (dwFinalAction != 0)
                {
                    fprintf(stderr, "Use only one of -s, -t, -c, -d, -e, --list\n");
                    ShowUsage();
                    exit(1);
                }
                dwFinalAction = action;
                break;

            case ACTION_HELP:
                ShowUsage();
                exit(0);
                break;
        }
    }

    if (dwFinalAction == 0)
    {
        fprintf(stderr, "Must have one of -s, -t, -c, -d, -e, or --list\n");
        ShowUsage();
        exit(1);
    }

    if (dwFinalAction == ACTION_LIST)
    {
        dwError = XmlList();
        BAIL_ON_EVT_ERROR(dwError);

        goto error; // Otherwise we have a lot of indenting.
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


    dwError = LwEvtOpenEventlog(
                    pszHostname,
                    &pEventLogHandle);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwMinutesForFilter != 0)
    {
        dwSecondsForFilter = dwMinutesForFilter * 60;
    }
    else if (dwHoursForFilter != 0)
    {
        dwSecondsForFilter = dwHoursForFilter * 60 * 60;
    }
    else if (dwDaysForFilter != 0)
    {
        dwSecondsForFilter = dwDaysForFilter * 24 * 60 * 60;
    }

    dwError = ParseFilter(
                pszSqlFilter,
                pszUserForFilter,
                dwSecondsForFilter,
                &pwszSqlFilter);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwFinalAction == ACTION_EXPORT)
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
            dwError = ReadAndExportEvents(pEventLogHandle, pwszSqlFilter, fpExport);
        }
        else
        {
            dwError = -1;
            fprintf(stderr, "Unable to open file %s for writing.\n", pszExportPath);
        }
        BAIL_ON_EVT_ERROR(dwError);
    }
    else if (dwFinalAction == ACTION_COUNT)
    {
        dwError = LwEvtGetRecordCount(
                        pEventLogHandle,
                        pwszSqlFilter,
                        &nRecords);
        BAIL_ON_EVT_ERROR(dwError);

        printf("%d records found in database\n", nRecords);
    }
    else if (dwFinalAction == ACTION_SHOW)
    {
        do
        {
            LW_SAFE_FREE_MEMORY(pFilter);
            if (pwszSqlFilter)
            {
                dwError = LwAllocateWc16sPrintfW(
                                &pFilter,
                                L"(%ws) AND EventRecordId >= %llu",
                                pwszSqlFilter,
                                (long long unsigned)startRecordId);
                BAIL_ON_EVT_ERROR(dwError);
            }
            else
            {
                dwError = LwAllocateWc16sPrintfW(
                                &pFilter,
                                L"EventRecordId >= %llu",
                                (long long unsigned)startRecordId);
                BAIL_ON_EVT_ERROR(dwError);
            }

            if (pEventRecords)
            {
                LwEvtFreeRecordArray(
                    nRecords,
                    pEventRecords);
                pEventRecords = NULL;
            }
            dwError = LwEvtReadRecords(
                        pEventLogHandle,
                        nRecordsPerPage,
                        pFilter,
                        &nRecords,
                        &pEventRecords);
            BAIL_ON_EVT_ERROR(dwError);

            PrintEventRecords(stdout, pEventRecords, nRecords, &currentRecord);
            if (nRecords > 0)
            {
                startRecordId = pEventRecords[nRecords - 1].EventRecordId;
            }
        } while (nRecords == nRecordsPerPage && nRecords > 0);
    }
    else if (dwFinalAction == ACTION_TABLE)
    {
        do
        {
            LW_SAFE_FREE_MEMORY(pFilter);
            if (pwszSqlFilter)
            {
                dwError = LwAllocateWc16sPrintfW(
                                &pFilter,
                                L"(%ws) AND EventRecordId >= %llu",
                                pwszSqlFilter,
                                (long long unsigned)startRecordId);
                BAIL_ON_EVT_ERROR(dwError);
            }
            else
            {
                dwError = LwAllocateWc16sPrintfW(
                                &pFilter,
                                L"EventRecordId >= %llu",
                                (long long unsigned)startRecordId);
                BAIL_ON_EVT_ERROR(dwError);
            }

            if (pEventRecords)
            {
                LwEvtFreeRecordArray(
                    nRecords,
                    pEventRecords);
                pEventRecords = NULL;
            }
            dwError = LwEvtReadRecords(
                        pEventLogHandle,
                        nRecordsPerPage,
                        pFilter,
                        &nRecords,
                        &pEventRecords);
            BAIL_ON_EVT_ERROR(dwError);

            PrintEventRecordsTable(stdout, pEventRecords, nRecords, &currentRecord);
            if (nRecords > 0)
            {
                startRecordId = pEventRecords[nRecords - 1].EventRecordId;
            }
        } while (nRecords == nRecordsPerPage && nRecords > 0);
    }
    else if (dwFinalAction == ACTION_DELETE)
    {
        dwError = LwEvtDeleteRecords(
                        pEventLogHandle,
                        pwszSqlFilter);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        fprintf(stderr, "Invalid action: %d\n", action);
        ShowUsage();
        exit(0);
    }

 error:
    if (dwError != 0)
    {
        fprintf(stderr, "The operation failed with error code [%d]\n", dwError);
    }
    LW_SAFE_FREE_MEMORY(pwszSqlFilter);
    LW_SAFE_FREE_MEMORY(pFilter);
    if (pEventLogHandle)
    {
        LwEvtCloseEventlog(pEventLogHandle);
    }
    if (pEventRecords)
    {
        LwEvtFreeRecordArray(
            nRecords,
            pEventRecords);
        pEventRecords = NULL;
    }
    poptFreeContext(optCon);
    if (fpExport)
    {
        fclose(fpExport);
    }

    return dwError;
}
