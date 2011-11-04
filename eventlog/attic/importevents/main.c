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
 * Utility to import events exported from Microsoft Event Viewer
 *
 */
#include "eventlog.h"
#include "evtbase.h"
#include "evtparser.h"
#include <compat/dcerpc.h>

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

static
void
ShowUsage()
{
    printf("Usage: importevents <path to csv file with events> {<table_category>}\n");
    printf("\ttable_category - This is an optional integer argument to importevents. It\n");
    printf("\tcontrols which table this event is displayed in within the event viewer.\n");
    printf("\tPossible Values:\n");
    printf("\t0 - Application\n");
    printf("\t1 - Web Browser\n");
    printf("\t2 - Security\n");
    printf("\t3 - System\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszFilepath,
    DWORD* eventTableCategoryId)
{
    DWORD dwError = 0;
    PSTR pszArg = NULL;
    PSTR pszFilepath = NULL;
    if(argc <= 1 || argc > 3) {
	ShowUsage();
	exit(0);
    }

    pszArg = argv[1];
    if (pszArg == NULL || *pszArg == '\0' || strcmp(pszArg, "--help") == 0 || strcmp(pszArg, "-h") == 0)
    {
	ShowUsage();
	exit(0);
    }
    dwError = EVTAllocateString(pszArg, &pszFilepath);
    BAIL_ON_EVT_ERROR(dwError);
    
    if(argc == 3) {
	*eventTableCategoryId = atoi(argv[2]);
    }

    *ppszFilepath = pszFilepath;

cleanup:

    return dwError;
    
error:

    EVT_SAFE_FREE_STRING(pszFilepath);

    *ppszFilepath = NULL;

    goto cleanup;
}

DWORD
AddEventRecord(
    handle_t bindingHandle,
    HANDLE hEventlog,
    EVENT_LOG_RECORD eventRecord
    )
{
    return LWIWriteEventLog(bindingHandle, hEventlog, eventRecord);
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    handle_t bindingHandle = 0;
    HANDLE hEventLog = 0;
    PSTR pszBindingString = NULL;

    DWORD eventTableCategoryId = (DWORD) -1;

    PSTR pszFilename = NULL;
    
    evt_init_logging_to_file(LOG_LEVEL_VERBOSE, "");

    dwError = ParseArgs(argc, argv, &pszFilename, &eventTableCategoryId);
    BAIL_ON_EVT_ERROR(dwError);

    if (IsNullOrEmptyString(pszFilename)) {
       EVT_LOG_ERROR("No path to the file containing events was specified.");
       ShowUsage();
       BAIL_ON_EVT_ERROR((dwError = EINVAL));
    }

    TRY
    {
	dwError = LWIOpenEventLog(&bindingHandle, &hEventLog, &pszBindingString, "127.0.0.1", "127.0.0.1");
	BAIL_ON_EVT_ERROR(dwError);

        dwError = ParseAndAddEvents(bindingHandle, hEventLog, pszFilename, eventTableCategoryId, AddEventRecord);
        BAIL_ON_EVT_ERROR(dwError);

    }
    CATCH_ALL
    {
        exc_get_status (THIS_CATCH, &dwError);
        EVT_LOG_ERROR("Unexpected error . Error code [%d]\n", dwError);
        BAIL_ON_EVT_ERROR(dwError);
    }
    ENDTRY;


  error:

    if (bindingHandle && hEventLog && pszBindingString)
	LWICloseEventLog(bindingHandle, hEventLog, pszBindingString);
        
    if(dwError != 0) {
	EVT_LOG_ERROR("Failed to import events. Error code [%d]\n", dwError);
    }

    return dwError;
}
