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

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#define ACTION_NONE 0
#define ACTION_SHOW 1
#define ACTION_TABLE 2
#define ACTION_COUNT 3
#define ACTION_EXPORT 4
#define ACTION_DELETE 5
#define ACTION_LAST 6

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


static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PDWORD action,
    char** pstrArgCopy,
    PSTR ipAddress,
    PSTR *ppszEventTableCategoryId,
    PBOOLEAN pbEventTableCategoryIdInCSV)
{
    int iArg = 1;
    DWORD maxIndex = argc-1;
    PSTR pszArg = NULL;
    PSTR pstrArgLocal = NULL;

    DWORD dwError = 0;
    DWORD actionLocal = 0;
    PSTR   pszEventTableCategoryId = 0;
    BOOLEAN bEventTableCategoryIdInCSV = TRUE;
    PSTR fqdn = NULL;;

    if (iArg > maxIndex || maxIndex > 8) {
        ShowUsage();
        exit(0);
    }

    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0') {
        ShowUsage();
        exit(0);
    }
    else if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0)) {
        ShowUsage();
        exit(0);
    }
    else if (strcmp(pszArg, "-s") == 0) {
        actionLocal = ACTION_SHOW;
    }
    else if (strcmp(pszArg, "-t") == 0) {
        actionLocal = ACTION_TABLE;
    }
    else if (strcmp(pszArg, "-c") == 0) {
        actionLocal = ACTION_COUNT;
    }
    else if (strcmp(pszArg, "-e") == 0) {
        actionLocal = ACTION_EXPORT;
    }
    else if (strcmp(pszArg, "-d") == 0) {
        actionLocal = ACTION_DELETE;
    }
    else {
        ShowUsage();
        exit(0);
    }

    if (iArg > maxIndex) {
        ShowUsage();
        exit(0);
    }

    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0') {
        ShowUsage();
        exit(0);
    }
    else {
        dwError = EVTAllocateMemory(strlen(pszArg)+1, (PVOID*)(&pstrArgLocal));
        BAIL_ON_EVT_ERROR(dwError);
        strcpy(pstrArgLocal, pszArg);
    }

    while (iArg+2 <= maxIndex)
    {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            ShowUsage();
            exit(0);
        }
        else {
            ShowUsage();
            exit(0);
        }
    }

    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else {
        if(!LWIIsLocalHost(ipAddress)) {
            switch(LWIStr2Inet4Addr(pszArg, &fqdn)) {
                case 1:
                case 2:
                    strcpy(ipAddress, pszArg);
                    break;

                default: // Success
                    strncpy(ipAddress, fqdn, 255);

                    if(fqdn) {
                        free(fqdn);
                    }
                    break;
            }
        }
        else {
            strcpy(ipAddress, pszArg);
        }

        fprintf(stderr, "Connecting to %s ...\n", ipAddress);
    }

    *action = actionLocal;
    *pstrArgCopy = pstrArgLocal;
    *ppszEventTableCategoryId = pszEventTableCategoryId;
    *pbEventTableCategoryIdInCSV = bEventTableCategoryIdInCSV;

 error:

    return dwError;
}



int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_HANDLE pEventLogHandle = NULL;
    EVENT_LOG_RECORD* eventRecords = NULL;
    DWORD nRecords = 0;
    DWORD currentRecord = 0;
    DWORD nRecordsPerPage = 500;
    PSTR  pszEventTableCategoryId = NULL;
    BOOLEAN bEventTableCategoryIdInCSV = FALSE;

    char ipAddress[256];

    char* sqlFilterChar = NULL;
    char* sqlFilterCharDefault = "eventRecordId >= 0";

    PSTR argCopy = NULL;
    DWORD action = ACTION_NONE;

    dwError = EVTInitLoggingToFile( LOG_LEVEL_ERROR,
                                    NULL);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = ParseArgs( argc,
                         argv,
                         &action,
                         &argCopy,
                         ipAddress,
                         &pszEventTableCategoryId,
                         &bEventTableCategoryIdInCSV);
    BAIL_ON_EVT_ERROR(dwError);

    if (action <= ACTION_NONE || action > ACTION_LAST) {
        EVT_LOG_VERBOSE("Invalid action: %d\n", action);
        ShowUsage();
        exit(0);
    }

    TRY
    {

        dwError = LWIOpenEventLog(ipAddress, (PHANDLE)&pEventLogHandle);
        BAIL_ON_EVT_ERROR(dwError);

        if (action == ACTION_EXPORT)
        {
            FILE* fpExport = NULL;

            if (strcmp(argCopy, "-") == 0)
            {
                fpExport = stdout;
            }
            else
            {
                fpExport = fopen(argCopy, "w");
            }

            if (fpExport != NULL)
            {
                dwError = ReadAndExportEvents(pEventLogHandle, fpExport);
            }
            else {
                dwError = -1;
                EVT_LOG_VERBOSE("Unable to open file %s for writing.\n", argCopy);
            }
            BAIL_ON_EVT_ERROR(dwError);
        }

        else {
            if (argCopy == NULL || *argCopy == '\0' || strcmp(argCopy, "-") == 0) {
                sqlFilterChar = sqlFilterCharDefault;
            }
            else {

                DWORD startID = 0;
                DWORD endID = 0;
                int   nRead = 0;

                nRead = sscanf(argCopy, "%d-%d", &startID, &endID);
                if ((nRead == 0) || (nRead == EOF))
                {
                    printf("Error: An invalid event record range [%s] "
                           "was specified.\n",
                           argCopy);

                    dwError = EVT_ERROR_INVALID_PARAMETER;
                    BAIL_ON_EVT_ERROR(dwError);
                }

                if (startID > 0) {
                    dwError = EVTAllocateMemory(256, (PVOID*)(&sqlFilterChar));
                    BAIL_ON_EVT_ERROR(dwError);

                    if (endID > 0 && endID > startID) {
                        sprintf(sqlFilterChar, "EventRecordId >= %d AND EventRecordID <= %d", startID, endID);
                    }
                        else
                        {
                            sprintf(sqlFilterChar, "EventRecordId = %d", startID);
                        }
                    }
                    else {
                        sqlFilterChar = argCopy;
                    }
            }

            wchar16_t* sqlFilter = NULL;
            dwError = EVTAllocateMemory(sizeof(wchar16_t)*(1+strlen(sqlFilterChar)), (PVOID*)(&sqlFilter));
            BAIL_ON_EVT_ERROR(dwError);

            sw16printf(sqlFilter, "%s", sqlFilterChar);

            if (action == ACTION_COUNT)
            {
                dwError = LWICountEventLog((HANDLE)pEventLogHandle,
                                sqlFilter,
                                &nRecords);


                BAIL_ON_EVT_ERROR(dwError);
                printf("%d records found in database\n", nRecords);
            }
            else if (action == ACTION_SHOW)
            {
                do {

                    dwError = LWIReadEventLog((HANDLE)pEventLogHandle,
                                    currentRecord,
                                    nRecordsPerPage,
                                    sqlFilter,
                                    &nRecords,
                                    &eventRecords);

                    BAIL_ON_EVT_ERROR(dwError);

                    PrintEventRecords(stdout, eventRecords, nRecords, &currentRecord);

                } while (nRecords == nRecordsPerPage && nRecords > 0);
            }
            else if (action == ACTION_TABLE)
            {
                do {

                    dwError = LWIReadEventLog((HANDLE)pEventLogHandle,
                                    currentRecord,
                                    nRecordsPerPage,
                                    sqlFilter,
                                    &nRecords,
                                    &eventRecords);

                    BAIL_ON_EVT_ERROR(dwError);

                    PrintEventRecordsTable(stdout, eventRecords, nRecords, &currentRecord);

                } while (nRecords == nRecordsPerPage && nRecords > 0);
            }
            else if (action == ACTION_DELETE)
            {
                if (strcmp(sqlFilterChar, sqlFilterCharDefault) == 0)
                {
                    //This is probably a faster way of deleting
                    //everything, because it drops the
                    //table instead of running an SQL DELETE
                    dwError = LWIClearEventLog((HANDLE)pEventLogHandle);
                }
                else
                {
                    dwError = LWIDeleteFromEventLog(
                                    (HANDLE)pEventLogHandle,
                                    sqlFilter
                                    );
                }
                BAIL_ON_EVT_ERROR(dwError);
            }
            else
            {
                EVT_LOG_VERBOSE("Invalid action: %d\n", action);
                ShowUsage();
                exit(0);
            }
        }  //end else
    }  //end TRY
    CATCH_ALL
    {
        dwError = dcethread_exc_getstatus (THIS_CATCH);
        EVT_LOG_ERROR("Invalid Operation: %d\n", dwError);
    }
    ENDTRY;

 error:

    if (dwError != 0) {
        EVT_LOG_ERROR("The operation failed with error code [%d]\n", dwError);
    }

    if (sqlFilterChar != NULL && sqlFilterChar != sqlFilterCharDefault)
    {
        EVTFreeMemory(sqlFilterChar);
    }

    if (pEventLogHandle)
    {
        LWICloseEventLog((HANDLE)pEventLogHandle);
    }


    if (eventRecords) {
        RPCFreeMemory(eventRecords);
        EVTFreeMemory(eventRecords);
    }

    return dwError;
}
