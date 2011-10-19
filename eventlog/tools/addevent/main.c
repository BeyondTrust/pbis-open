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
 * Test utility to add a single event over RPC
 *
 */
#include "config.h"
#include "eventsys.h"
#include "eventlog.h"
#include "eventdefs.h"
#include "eventutils.h"

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

//space: ' '
#define FIRST_LETTER ((char)32)

//tilde: '~'
#define LAST_LETTER ((char)126)

#define NUM_LETTERS ((DWORD)LAST_LETTER - (DWORD)(FIRST_LETTER))

#define RAND_LETTER() ((char)(((double)rand() /  (double)RAND_MAX) * (double)NUM_LETTERS) + FIRST_LETTER)

#define TRY DCETHREAD_TRY
#define CATCH_ALL DCETHREAD_CATCH_ALL(THIS_CATCH)
#define ENDTRY DCETHREAD_ENDTRY

void
ShowUsage();

int
IsNumber(char* strNum);

static
DWORD
BuildEventRecord(
    PEVENT_LOG_RECORD* ppEventRecord
    )
{
    DWORD dwError = 0;
    PEVENT_LOG_RECORD pEventRecord = NULL;

    dwError = EVTAllocateMemory(sizeof(EVENT_LOG_RECORD), (PVOID*)&pEventRecord);
    BAIL_ON_EVT_ERROR(dwError);

    pEventRecord->dwEventRecordId = 0;

    dwError = EVTAllocateString("Application", (PSTR*)(&pEventRecord->pszEventTableCategoryId));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateString("Information", (PSTR*)(&pEventRecord->pszEventType));
    BAIL_ON_EVT_ERROR(dwError);

    pEventRecord->dwEventDateTime = time(NULL);

    dwError = EVTAllocateString("addeventUtility", (PSTR*)(&pEventRecord->pszEventSource));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateString("SpecificCategory", (PSTR*)(&pEventRecord->pszEventCategory));
    BAIL_ON_EVT_ERROR(dwError);

    pEventRecord->dwEventSourceId = 1;

    dwError = EVTAllocateString("SpecificUser", (PSTR*)(&pEventRecord->pszUser));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateString("SpecificComputer", (PSTR*)(&pEventRecord->pszComputer));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateString(
    "a useful description, within quotes",
    (PSTR*)(&pEventRecord->pszDescription));
    
    dwError = EVTAllocateString(
    "Hex string representing the associated error code of this event",
    (PSTR*)(&pEventRecord->pszData));

    BAIL_ON_EVT_ERROR(dwError);

    *ppEventRecord = pEventRecord;

cleanup:

    return dwError;

error:

    if (pEventRecord)
        LWIFreeEventRecord(pEventRecord);

    *ppEventRecord = NULL;

    goto cleanup;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = 0;
    DWORD i = 0;
    DWORD j = 0;
    DWORD num_iters = 0;
    char type[1024];
    char category[1024];
    char description[1024];
    char rand_str[1024];
    char hostname[256];

    EVENT_LOG_RECORD* pEventRecord = NULL;

    if (argc > 1)
    {
        if(IsNumber(argv[1]))
        {
            ShowUsage();
            exit(0);
        }
        
        num_iters = atoi(argv[1]);
        if( num_iters < 0 || num_iters > 1000 )
        {
            fprintf(stdout, "Please enter the number between 1-1000\n");
            ShowUsage();
            exit(0);
        }
    }

    dwError = gethostname( hostname,
                           sizeof(hostname));
    BAIL_ON_EVT_ERROR(dwError);

    TRY
    {

        dwError = LWIOpenEventLogEx(hostname,
                        "System",             // char* pszEventTableCategoryId
                        "DefaultEventSource",      //char * pszEventSource
                        123,                       //DWORD dwEventSourceId
                        "DefaultUser",             //char * pszUser
                        "DefaultComputer",          //char * pszComputer
                        &hEventLog);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LWIWriteEventLog(hEventLog,
                        "smallishEventType",
                        "littleCategory",
                        "shortDescription",
                        "<null>");
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LWISetEventLogUser(hEventLog,
                         "AStrangeUser");
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LWIWriteEventLog(hEventLog,
                        "smallishEventType",
                        "littleCategory",
                        "A slightly different shortDescription",
                        "<null>");
        BAIL_ON_EVT_ERROR(dwError);

        dwError = BuildEventRecord(&pEventRecord);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LWIWriteEventLogBase(hEventLog, *pEventRecord);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LWICloseEventLog(hEventLog);
        BAIL_ON_EVT_ERROR(dwError);
        hEventLog = 0;

        //Try this with most things set to null
        dwError = LWIOpenEventLogEx(NULL,   //target host -- should end up going to localhost
                        "Application",       		// char* pszEventTableCategoryId
                        "DefaultEventSource",      //char * pszEventSource
                        0,                       //DWORD dwEventSourceId
                        NULL,             //char * pszUser
                        NULL,          //char * pszComputer
                        &hEventLog);
        BAIL_ON_EVT_ERROR(dwError);


        for (i = 0; i < num_iters; i++) {
            memset(rand_str, 0, 61);
            for (j = 0; j < 60; j++) {
            rand_str[j] = RAND_LETTER();
            if (rand_str[j] == '"') {
                rand_str[j] = ' ';
            }
        }

        printf("adding record %d/%d with salt: %s.  \n", i+1, num_iters, rand_str);

        sprintf(type, "typeRandom%d", i);
        sprintf(category, "category%s", rand_str);
        sprintf(description, "description%d_%s", i, rand_str);

        dwError = LWIWriteEventLog(hEventLog,
                        type,
                        category,
                        description,
                        "<null>");
        BAIL_ON_EVT_ERROR(dwError);
    }


    dwError = LWICloseEventLog(hEventLog);
    BAIL_ON_EVT_ERROR(dwError);
    hEventLog = 0;

    }
    CATCH_ALL
    {
        dwError = dcethread_exc_getstatus (THIS_CATCH);
        EVT_LOG_ERROR("Unexpected error . Error code [%d]\n", dwError);
        BAIL_ON_EVT_ERROR(dwError);
    }
    ENDTRY;

    error:

    if (hEventLog)
    {
        LWICloseEventLog(hEventLog);
    }

    if (pEventRecord)
    {
        LWIFreeEventRecord(pEventRecord);
    }

    if (dwError != 0) {
        fprintf(stderr, "Error: Failed to add event\n");
        EVT_LOG_ERROR("Failed to add event. Error code [%d]\n", dwError);
    }

    return dwError;
}

void
ShowUsage()
{
    printf("Usage: lw-addevent [<number of events>] \n");
    printf("max <number of events> = 1000\n");
    printf("\nExamples:\n");
    printf("\tlw-addevent  - if no option is provided, writes three records\n");
    printf("\tlw-addevent 10 - if option is provided then writes the number of records provided\n");

}

int
IsNumber(char* strNum)
{
    int nLen = 0;
    int nIndex = 0;

    nLen = strlen(strNum);

    for (nIndex = 0; nIndex < nLen; nIndex++)
    {
        if (isdigit((int)strNum[nIndex]) == 0)
            return -1;
    }
    return 0;
}

