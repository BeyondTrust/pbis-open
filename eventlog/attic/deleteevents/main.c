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
 * Eventlog utility that deletes events over RPC from the Eventlog service.
 *
 */
#include "evtbase.h"
#include "eventlog.h"
#include <compat/dcerpc.h>

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

static
void
ShowUsage()
{
    printf("Usage: deleteevents {-|<sql expression>}\n");
    printf("Example:\n");
    printf("\tDeletes all events:\n");
    printf("\tdeleteevents -  \n\n");
    printf("\tDeletes all events matching the SQL WHILE expression:\n");
    printf("\tdeleteevents \"(EventRecordId < 1000) AND (EventType = 1)\"\n");
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
    printf("\tDescription   (TEXT)\n");
    printf("\Data           (varchar(128))\n");

}

static
int
ParseArgs(
    int argc,
    char* argv[],
    PSTR* pSqlQuery)
{
    PSTR pszArg = NULL;

    if (argc != 2)
    {
	ShowUsage();
	exit(0);
    }

    pszArg = argv[1];
    if (pszArg == NULL || *pszArg == '\0')
    {
	ShowUsage();
	exit(0);
    }
    
    if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
    {
	ShowUsage();
	exit(0);
    }
    else if (strcmp(pszArg, "-") != 0)
    {
	*pSqlQuery = pszArg;
    }
    
    return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = (HANDLE)NULL;
    handle_t bindingHandle = 0;
    PSTR pszBindingString = NULL;
    EVENT_LOG_RECORD* eventRecords = NULL;
    PSTR sqlFilterChar = NULL;

    evt_init_logging_to_file(LOG_LEVEL_ERROR, "");

    ParseArgs(argc, argv, &sqlFilterChar);
    
    wchar16_t* sqlFilter = NULL;

    if(sqlFilterChar != NULL) {
	dwError = EVTAllocateMemory(sizeof(wchar16_t)*(1+strlen(sqlFilterChar)), &sqlFilter);
	
	sw16printf(sqlFilter, "%s", sqlFilterChar);
    }

    TRY
    {

    dwError = LWIOpenEventLog(&bindingHandle, &hEventLog, &pszBindingString, "127.0.0.1", "127.0.0.1");
    BAIL_ON_EVT_ERROR(dwError);
    
    if(sqlFilter == NULL) {
	
	dwError = LWIClearEventLog(bindingHandle,
				hEventLog);
    }
    else {
	dwError = LWIDeleteFromEventLog(bindingHandle,
				     hEventLog,
				     sqlFilter);
    }
    BAIL_ON_EVT_ERROR(dwError);
    
    }
    CATCH_ALL
    {
        exc_get_status (THIS_CATCH, &dwError);
        EVT_LOG_ERROR("Unexpected error. Error code [%d]\n", dwError);
        BAIL_ON_EVT_ERROR(dwError);
    }
    ENDTRY;
    
 error:
    
    if (bindingHandle && hEventLog && pszBindingString)
	LWICloseEventLog(bindingHandle, hEventLog, pszBindingString);
        
    if(dwError != 0) {
	EVT_LOG_ERROR("Failed to delete events. Error code [%d]\n", dwError);
    }

    return dwError;
}
