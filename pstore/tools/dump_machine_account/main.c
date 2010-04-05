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
 *        Test Program for exercising SqlDBGetPwdEntry
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "lwps-utils.h"
#include "lwps-def.h"
#include "lwps/lwps.h"

/***************************************************
 */

static void
ShowUsage(const char *szProgramName)
{
	printf("Usage: %s <domainDnsName>\n", szProgramName);
}


/***************************************************
 */

static DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszDomainDnsName
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_DONE
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR  pszDomainDnsName = NULL;
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
                    ShowUsage(argv[0]);
                    exit(0);
                }
                else
                {
                    dwError = LwpsAllocateString(pszArg, &pszDomainDnsName);
                    BAIL_ON_LWPS_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;
            default:
                dwError = LWPS_ERROR_INTERNAL;
                BAIL_ON_LWPS_ERROR(dwError);
        }
        
    } while ((parseMode != PARSE_MODE_DONE) && (iArg < argc));

    if (IsNullOrEmptyString(pszDomainDnsName))
    {
       ShowUsage(argv[0]);
       exit(1);
    }

    *ppszDomainDnsName = pszDomainDnsName;

cleanup:
    
    return dwError;

error:

    LWPS_SAFE_FREE_STRING(pszDomainDnsName);

    *ppszDomainDnsName = NULL;

    goto cleanup;
}



/***************************************************
 */

#define LW_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszReqDomainName;
    PSTR  pszDomainName = NULL;
    PSTR  pszDomainSID = NULL;
    PSTR  pszMachineAccount = NULL;
    PSTR  pszMachinePassword = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostName = NULL;
    PSTR  pszHostDnsDomain = NULL;
    PLWPS_PASSWORD_INFO pMachineAcctInfo = NULL;
    HANDLE hPasswordStore = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    
    dwError = ParseArgs(argc, argv, &pszReqDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsOpenPasswordStore(
                    LWPS_PASSWORD_STORE_DEFAULT,
                    &hPasswordStore);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsGetPasswordByDomainName(
                    hPasswordStore,
                    pszReqDomainName,
                    &pMachineAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    if (pMachineAcctInfo->pwszDomainName)
    {
        dwError = LwpsWc16sToMbs(
                        pMachineAcctInfo->pwszDomainName,
                        &pszDomainName);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if (pMachineAcctInfo->pwszSID)
    {
        dwError = LwpsWc16sToMbs(
                        pMachineAcctInfo->pwszSID,
                        &pszDomainSID);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if (pMachineAcctInfo->pwszMachineAccount)
    {
        dwError = LwpsWc16sToMbs(
                        pMachineAcctInfo->pwszMachineAccount,
                        &pszMachineAccount);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if (pMachineAcctInfo->pwszMachinePassword)
    {
        dwError = LwpsWc16sToMbs(
                        pMachineAcctInfo->pwszMachinePassword,
                        &pszMachinePassword);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if (pMachineAcctInfo->pwszDnsDomainName)
    {
        dwError = LwpsWc16sToMbs(
                        pMachineAcctInfo->pwszDnsDomainName,
                        &pszDomainDnsName);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if (pMachineAcctInfo->pwszHostname)
    {
        dwError = LwpsWc16sToMbs(
                        pMachineAcctInfo->pwszHostname,
                        &pszHostName);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if (pMachineAcctInfo->pwszHostDnsDomain)
    {
        dwError = LwpsWc16sToMbs(
                        pMachineAcctInfo->pwszHostDnsDomain,
                        &pszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    printf("\n");
    printf("DomainSID                = %s\n", LW_SAFE_LOG_STRING(pszDomainSID));
    printf("DomainName               = %s\n", LW_SAFE_LOG_STRING(pszDomainName));
    printf("Domain DNS Name          = %s\n", LW_SAFE_LOG_STRING(pszDomainDnsName));
    printf("HostName                 = %s\n", LW_SAFE_LOG_STRING(pszHostName));
    printf("HostName DNS Domain      = %s\n", LW_SAFE_LOG_STRING(pszHostDnsDomain));
    printf("Machine Account Name     = %s\n", LW_SAFE_LOG_STRING(pszMachineAccount));
    printf("Machine Account Password = %s\n", LW_SAFE_LOG_STRING(pszMachinePassword));
    printf("\n");    
    
cleanup:

    LWPS_SAFE_FREE_STRING(pszReqDomainName);
    LWPS_SAFE_FREE_STRING(pszDomainSID);
    LWPS_SAFE_FREE_STRING(pszDomainName);
    LWPS_SAFE_FREE_STRING(pszDomainDnsName);
    LWPS_SAFE_FREE_STRING(pszHostName);
    LWPS_SAFE_FREE_STRING(pszHostDnsDomain);
    LWPS_SAFE_FREE_STRING(pszMachineAccount);
    LWPS_SAFE_FREE_STRING(pszMachinePassword);
    
    if (pMachineAcctInfo) {
        LwpsFreePasswordInfo(hPasswordStore, pMachineAcctInfo);
    }

    if (hPasswordStore) {
       LwpsClosePasswordStore(hPasswordStore);
    }

    return (dwError);

error:

    dwErrorBufferSize = LwpsGetErrorString(dwError, NULL, 0);

    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;

        dwError2 = LwpsAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);

        if (!dwError2)
        {
            DWORD dwLen = LwpsGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);

            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr,
                        "Failed to read from database.  Error code %u.\n%s\n",
                        dwError, pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LWPS_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr,
                "Failed to read from database.  Error code %u.\n",
                dwError);
    }

    goto cleanup;
}

