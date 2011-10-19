/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS) 
 *        
 *        Test Program for displaying all machine password entries
 *        stored in sqldb.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "lwps-utils.h"
#include "lwps-def.h"
#include "lwps/lwps.h"
#include "lwps-provider.h"
#include "provider-main_p.h"
#include "db_p.h"

DWORD
ParseArgs(
    int    argc,
    char*  argv[]
    );

VOID
ShowUsage();


int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hDB = 0;
    PMACHINE_ACCT_INFO pAcct = NULL;
    PMACHINE_ACCT_INFO* ppAcct = NULL;
    DWORD dwNumEntries = 0;
    DWORD iEntry = 0;
    
    lwps_init_logging_to_file(LOG_LEVEL_VERBOSE, TRUE, "");

    dwError = ParseArgs(argc, argv);
    BAIL_ON_LWPS_ERROR(dwError);
 
    dwError = SqlDBDbInitGlobals();
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = SqlDBGetPwdEntries(
                    hDB,
                    &ppAcct,
                    &dwNumEntries);
    BAIL_ON_LWPS_ERROR(dwError);
    
    for (iEntry = 0; iEntry < dwNumEntries; iEntry++)
    {
        pAcct = *(ppAcct + iEntry);
        printf(" Row: %d\n", iEntry+1);
        printf("==========");
        printf("\nDomainSID = \"%s\"\n",
               pAcct->pszDomainSID);
        printf("DomainName = \"%s\"\n",
               pAcct->pszDomainName);
        printf("DomainDnsName = \"%s\"\n",
               pAcct->pszDomainDnsName);
        printf("HostName = \"%s\"\n",
               pAcct->pszHostName);
        printf("MachineAccountName = \"%s\"\n",
               pAcct->pszMachineAccountName);
        printf("MachineAccountPassword = \"%s\"\n",
               pAcct->pszMachineAccountPassword);
        printf("PwdCreationTimestamp = \"%u\"\n",
               (DWORD) pAcct->tPwdCreationTimestamp);
        printf("PwdClientModifyTimestamp = \"%u\"\n",
               (DWORD) pAcct->tPwdClientModifyTimestamp);
        printf("SchannelType = \"%u\"\n\n",
               (DWORD) pAcct->dwSchannelType);
    }

cleanup:

    if (ppAcct)
    {
        SqlDBFreeEntryList(ppAcct, dwNumEntries);
    }

    SqlDBClose(hDB);    

    lwps_close_log();
 
    return (dwError);

error:

    fprintf(stderr, "Failed to read from database. Error code [%u]\n", dwError);

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[]
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_DONE
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR  pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        switch (parseMode)
        {
            case PARSE_MODE_OPEN:
        
                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                break;
            default:
                dwError = LWPS_ERROR_INTERNAL;
                BAIL_ON_LWPS_ERROR(dwError);
        }
        
    } while ((parseMode != PARSE_MODE_DONE) && (iArg < argc));

cleanup:
    
    return dwError;

error:

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: test-show-sqldb-machinepwd-entries\n");
}


