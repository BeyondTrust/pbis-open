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
    printf("Usage: listevents [-q <sql_while_filter>] [-c|<Number of records in page>]\n");
    printf("\t-c\tReturns a count of the number of records in the database\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[],
    PDWORD pdwRecordsPerPage,
    BOOLEAN* returnCount,
    char** sqlFilter)
{
    int iArg = 1;
    PSTR pszArg = NULL;
    int nRecordsPerPage = 10;
    BOOLEAN returnCountLocal = FALSE;
    char* sqlFilterLocal = FALSE;
    DWORD dwError = 0;
   
    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0')
    {
	//do nothing
    }
    else {
	if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
	{
	    ShowUsage();
	    exit(0);
	}
	else if (strcmp(pszArg, "-q") == 0)
	{
	    pszArg = argv[iArg++];
	    if(pszArg != NULL && *pszArg != '\0') {
		dwError = EVTAllocateMemory(strlen(pszArg)+1, &sqlFilterLocal);
		strcpy(sqlFilterLocal, pszArg);
		pszArg = argv[iArg++];

		EVT_LOG_INFO("running SQL QUERY \"%s\"\n", sqlFilterLocal);
	    }
	    else {
		ShowUsage();
		exit(0);
	    }
	}
	
	if(pszArg == NULL || *pszArg == '\0') {
	    //do nothing
	}
	else if (strcmp(pszArg, "-c") == 0)
	{
	    returnCountLocal = TRUE;
	}
	else
	{
    	    nRecordsPerPage = atoi(pszArg);
	}
    }
   
    *pdwRecordsPerPage = nRecordsPerPage;
    *returnCount = returnCountLocal;
    *sqlFilter = sqlFilterLocal;

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
    EVENT_LOG_RECORD* pRecord = NULL;
    idl_long_int nRecords = 0;
    DWORD iRecord = 0;
    DWORD currentRecord = 0;
    DWORD nRecordsPerPage = 0;
    

    evt_init_logging_to_file(LOG_LEVEL_ERROR, "");

    BOOLEAN returnCount = FALSE;
    char* sqlFilterChar = NULL;
    const char* sqlFilterCharDefault = "eventRecordId > 0";

    ParseArgs(argc, argv, &nRecordsPerPage, &returnCount, &sqlFilterChar);

    if(sqlFilterChar == NULL || *sqlFilterChar == '\0') {
	sqlFilterChar = sqlFilterCharDefault;
    }
    
    wchar16_t* sqlFilter = NULL;
    dwError = EVTAllocateMemory(sizeof(wchar16_t)*(1+strlen(sqlFilterChar)), &sqlFilter);

    sw16printf(sqlFilter, "%s", sqlFilterChar);

    TRY
    {

    dwError = LWIOpenEventLog(&bindingHandle, &hEventLog, &pszBindingString, "127.0.0.1", "127.0.0.1");
    BAIL_ON_EVT_ERROR(dwError);

    if(returnCount) {
	
	dwError = LWICountEventLog(bindingHandle,
				hEventLog,
				&nRecords,
				sqlFilter);
				
	BAIL_ON_EVT_ERROR(dwError);
	

	printf("%d records found in database\n", nRecords);

	
	if (bindingHandle && hEventLog && pszBindingString)
	    LWICloseEventLog(bindingHandle, hEventLog, pszBindingString);
	
	return dwError;

    }

    else {

	do {
	
	    dwError = LWIReadEventLog(bindingHandle,
				   hEventLog,
				   currentRecord,
				   nRecordsPerPage,
				   &nRecords,
				   sqlFilter,
				   &eventRecords);
	    
	    BAIL_ON_EVT_ERROR(dwError);
	    
	    PrintEventRecords(stdout, eventRecords, nRecords, &currentRecord);
	    
	} while(nRecords == nRecordsPerPage && nRecords > 0);
	
    }  //end else
    }
    CATCH_ALL
    {
        exc_get_status (THIS_CATCH, &dwError);
        EVT_LOG_ERROR("Failed to open event log. Error code [%d]\n", dwError);
        BAIL_ON_EVT_ERROR(dwError);
    }
    ENDTRY;
    
 error:

    if(sqlFilterChar != NULL && sqlFilterChar != sqlFilterCharDefault) 
    {
	EVTFreeMemory(sqlFilterChar);
    }
    
    if (bindingHandle && hEventLog && pszBindingString)
	LWICloseEventLog(bindingHandle, hEventLog, pszBindingString);

    if(eventRecords) {
	RPCFreeMemory(eventRecords);
	
	EVTFreeMemory(eventRecords);
    }

    if(dwError != 0) {
	EVT_LOG_ERROR("Failed to dump events. Error code [%d]\n", dwError);
    }

    return dwError;
}
