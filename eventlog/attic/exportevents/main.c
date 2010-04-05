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
 * Utility to export events to CSV
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
    printf("Usage: exportevents <csv output path>\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[],
    FILE** pfpExport)
{
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    FILE* fpExport = stdout;

    do {
        pszArg = argv[iArg++];

        if (pszArg == NULL || *pszArg == '\0')
        {
	    break;
        }
        else if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
	    fpExport = fopen(pszArg, "a");
	    if(fpExport == NULL) {
		printf("File not found: %s\n", pszArg);
		ShowUsage();
		exit(0);
	    }
        }
    } while (iArg < argc);

    *pfpExport = fpExport;
    
    return dwError;

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
    FILE* fpExport = NULL;

    evt_init_logging_to_file(LOG_LEVEL_ERROR, "");

    dwError = ParseArgs(argc, argv, &fpExport);

    BAIL_ON_EVT_ERROR(dwError);

    TRY
    {

	dwError = LWIOpenEventLog(&bindingHandle, &hEventLog, &pszBindingString, "127.0.0.1", "127.0.0.1");
        BAIL_ON_EVT_ERROR(dwError);

        dwError = ReadAndExportEvents(bindingHandle, hEventLog, fpExport);
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
	EVT_LOG_ERROR("Failed to dump events. Error code [%d]\n", dwError);
    }

    return dwError;
}

