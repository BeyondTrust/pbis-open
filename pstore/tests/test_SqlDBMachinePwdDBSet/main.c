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
 *        Test Program for exercising SqlDBSetPwdEntry
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
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
    char*  argv[],
    PSTR*  ppszDomainSID,
    PSTR*  ppszDomainName,
    PSTR*  ppszDomainDnsName,
    PSTR*  ppszHostName,
    PSTR*  ppszMachineAccountName,
    PSTR*  ppszMachineAccountPassword,
    PDWORD pdwPwdClientModifyTimestamp,
    PDWORD pdwSchannelType
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
    
    PSTR  pszDomainSID = NULL;
    PSTR  pszDomainName = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostName = NULL;
    PSTR  pszMachineAccountName = NULL;
    PSTR  pszMachineAccountPassword = NULL;
    DWORD dwPwdClientModifyTimestamp = 0;
    DWORD dwSchannelType = 0;

    HANDLE hDB = 0;
    PMACHINE_ACCT_INFO pAcct = NULL;
    
    lwps_init_logging_to_file(LOG_LEVEL_VERBOSE, TRUE, "");

    dwError = ParseArgs(argc,
                        argv, 
			&pszDomainSID,
                        &pszDomainName,
                        &pszDomainDnsName,
                        &pszHostName, 
                        &pszMachineAccountName,
                        &pszMachineAccountPassword,
                        &dwPwdClientModifyTimestamp,
                        &dwSchannelType);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBDbInitGlobals();
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateMemory(
                   sizeof(MACHINE_ACCT_INFO),
                   (PVOID*)&pAcct);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsAllocateString(pszDomainSID, &pAcct->pszDomainSID);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsAllocateString(pszDomainName, &pAcct->pszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateString(pszDomainDnsName, &pAcct->pszDomainDnsName);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsAllocateString(pszHostName, &pAcct->pszHostName);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsAllocateString(pszMachineAccountName,
                                 &pAcct->pszMachineAccountName);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsAllocateString(pszMachineAccountPassword,
                                 &pAcct->pszMachineAccountPassword);
    BAIL_ON_LWPS_ERROR(dwError);
    
    pAcct->tPwdCreationTimestamp = time(NULL);

    pAcct->tPwdClientModifyTimestamp = dwPwdClientModifyTimestamp;

    pAcct->dwSchannelType = dwSchannelType;

    dwError = SqlDBSetPwdEntry(hDB, pAcct);
    BAIL_ON_LWPS_ERROR(dwError);

    printf("SUCCESS!\n");
    
    SqlDBClose(hDB);
    
cleanup:

    LWPS_SAFE_FREE_STRING(pszDomainSID);
    LWPS_SAFE_FREE_STRING(pszDomainName);
    LWPS_SAFE_FREE_STRING(pszDomainDnsName);
    LWPS_SAFE_FREE_STRING(pszHostName);
    LWPS_SAFE_FREE_STRING(pszMachineAccountName);
    LWPS_SAFE_FREE_STRING(pszMachineAccountPassword);

    lwps_close_log();
 
    return (dwError);

error:


    if (dwError == LWPS_ERROR_INVALID_SID)
    {
        fprintf(stderr, "INVALID SID entered!\n");
        fprintf(stderr, "Format: S-<revision>-<auth>-[auth1-auth2-...authN]-[RID]\n");
        ShowUsage();
    }
    else 
    {
        fprintf(stderr, "Failed to write to database. Error code [%u]\n", dwError);
    }
    
    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszDomainSID,
    PSTR*  ppszDomainName,
    PSTR*  ppszDomainDnsName,
    PSTR*  ppszHostName,
    PSTR*  ppszMachineAccountName,
    PSTR*  ppszMachineAccountPassword,
    PDWORD pdwPwdClientModifyTimestamp,
    PDWORD pdwSchannelType
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_DOMAIN,
            PARSE_MODE_DOMAIN_DNS,
            PARSE_MODE_HOST,
            PARSE_MODE_MACHINE_NAME,
            PARSE_MODE_MACHINE_PASSWORD,
            PARSE_MODE_CLIENT_TIMESTAMP,
            PARSE_MODE_SCHANNEL
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR  pszDomainSID = NULL;
    PSTR  pszDomainName = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostName = NULL;
    PSTR  pszMachineAccountName = NULL;
    PSTR  pszMachineAccountPassword = NULL;
    DWORD dwPwdClientModifyTimestamp = 0;
    DWORD dwSchannelType = 0;
    PSTR  pszEndPtr = NULL;

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
                else
                {
                    dwError = LwpsAllocateString(pszArg, &pszDomainSID);
                    BAIL_ON_LWPS_ERROR(dwError);
                    parseMode = PARSE_MODE_DOMAIN;
                }
                break;
                
            case PARSE_MODE_DOMAIN:
                dwError = LwpsAllocateString(pszArg, &pszDomainName);
                BAIL_ON_LWPS_ERROR(dwError);
                parseMode = PARSE_MODE_DOMAIN_DNS;
                break;

                
            case PARSE_MODE_DOMAIN_DNS:
                dwError = LwpsAllocateString(pszArg, &pszDomainDnsName);
                BAIL_ON_LWPS_ERROR(dwError);
                parseMode = PARSE_MODE_HOST;
                break;
                
            case PARSE_MODE_HOST:
                dwError = LwpsAllocateString(pszArg, &pszHostName);
                BAIL_ON_LWPS_ERROR(dwError);
                parseMode = PARSE_MODE_MACHINE_NAME;
                break;
                
            case PARSE_MODE_MACHINE_NAME:
                dwError = LwpsAllocateString(pszArg, &pszMachineAccountName);
                BAIL_ON_LWPS_ERROR(dwError);
                parseMode = PARSE_MODE_MACHINE_PASSWORD;
                break;
                
            case PARSE_MODE_MACHINE_PASSWORD:
                dwError = LwpsAllocateString(pszArg, &pszMachineAccountPassword);
                BAIL_ON_LWPS_ERROR(dwError);
                parseMode = PARSE_MODE_CLIENT_TIMESTAMP;
                break;     

                
            case PARSE_MODE_CLIENT_TIMESTAMP:
	        dwPwdClientModifyTimestamp = 
                        strtoll(pszArg, &pszEndPtr, 10);
                if (!pszEndPtr || (pszEndPtr == pszArg) || *pszEndPtr)
                {
                    dwError = LWPS_ERROR_DATA_ERROR;
                    BAIL_ON_LWPS_ERROR(dwError);
                }
                parseMode = PARSE_MODE_SCHANNEL;
                break;     
                
	        case PARSE_MODE_SCHANNEL:
	            dwSchannelType = atol(pszArg);
                BAIL_ON_LWPS_ERROR(dwError);
                break;     
        }
    } while (iArg < argc);

    if (IsNullOrEmptyString(pszDomainSID) ||
        IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszDomainDnsName) ||
        IsNullOrEmptyString(pszHostName) ||
        IsNullOrEmptyString(pszMachineAccountName) ||
        IsNullOrEmptyString(pszMachineAccountPassword)) 
    {
       ShowUsage();
       exit(1);
    }

    *ppszDomainSID = pszDomainSID;
    *ppszDomainName = pszDomainName;
    *ppszDomainDnsName = pszDomainDnsName;
    *ppszHostName = pszHostName;
    *ppszMachineAccountName = pszMachineAccountName;
    *ppszMachineAccountPassword = pszMachineAccountPassword;
    *pdwPwdClientModifyTimestamp = dwPwdClientModifyTimestamp;
    *pdwSchannelType = dwSchannelType;    

cleanup:
    
    return dwError;

error:

    LWPS_SAFE_FREE_STRING(pszDomainSID);
    LWPS_SAFE_FREE_STRING(pszDomainName);
    LWPS_SAFE_FREE_STRING(pszDomainDnsName);
    LWPS_SAFE_FREE_STRING(pszHostName);
    LWPS_SAFE_FREE_STRING(pszMachineAccountName);
    LWPS_SAFE_FREE_STRING(pszMachineAccountPassword);

    *ppszDomainSID = NULL;
    *ppszDomainName = NULL;
    *ppszDomainDnsName = NULL;
    *ppszHostName = NULL;
    *ppszMachineAccountName = NULL;
    *ppszMachineAccountPassword = NULL;
    *pdwPwdClientModifyTimestamp = 0;
    *pdwSchannelType = 0;

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: test-sqldb-set <domainSID> <domainName> <domainDnsName> <hostName> ");
    printf("<machineAccountName> <machineAccountPassword> <pwdClientModifyTimestamp> <schannelType>\n");
}

