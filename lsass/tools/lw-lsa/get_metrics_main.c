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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Tool to get performance metrics from LSA Server
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"
#include "common.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
VOID
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwInfoLevel
    );

static
VOID
ShowUsage();

static
VOID
PrintMetricPack_0(
    PLSA_METRIC_PACK_0 pMetricPack
    );

static
VOID
PrintMetricPack_1(
    PLSA_METRIC_PACK_1 pMetricPack
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
get_metrics_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    DWORD dwInfoLevel = 0;
    PVOID pMetricPack = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    
    ParseArgs(argc, argv, &dwInfoLevel);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetMetrics(
                    hLsaConnection,
                    dwInfoLevel,
                    &pMetricPack);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (dwInfoLevel)
    {
        case 0:

            PrintMetricPack_0(
                (PLSA_METRIC_PACK_0)pMetricPack);

            break;

        case 1:

            PrintMetricPack_1(
                 (PLSA_METRIC_PACK_1)pMetricPack);

            break;

    }

cleanup:

    LW_SAFE_FREE_MEMORY(pMetricPack);
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return (dwError);

error:

    dwError = MapErrorCode(dwError);
    
    dwErrorBufferSize = LwGetErrorString(dwError, NULL, 0);
    
    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;
        
        dwError2 = LwAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);
        
        if (!dwError2)
        {
            DWORD dwLen = LwGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);
            
            if ((dwLen == dwErrorBufferSize) && !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
            {
                fprintf(
                    stderr,
                    "Failed to query metrics from LSA service.  Error code %u (%s).\n%s\n",
                    dwError,
                    LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                    pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(
            stderr,
            "Failed to query metrics from LSA service.  Error code %u (%s).\n",
            dwError,
            LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

VOID
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwInfoLevel
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL,
            PARSE_MODE_DONE
        } ParseMode;
        
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;

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
                else if (!strcmp(pszArg, "--level")) {
                    parseMode = PARSE_MODE_LEVEL;
                }
                else
                {
                    ShowUsage();
                    exit(1);
                }

                break;
                
            case PARSE_MODE_LEVEL:
                
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter an info level which is an unsigned integer.\n");
                    ShowUsage();
                    exit(1); 
                }
                
                dwInfoLevel = atoi(pszArg);
                parseMode = PARSE_MODE_DONE;
                
                break;
                
            case PARSE_MODE_DONE:
                
                ShowUsage();
                exit(1);

        }
        
    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage();
        exit(1);  
    }
    
    *pdwInfoLevel = dwInfoLevel;
}

void
ShowUsage()
{
    printf("Usage: lw-get-metrics { --level [0, 1] }\n");
}

VOID
PrintMetricPack_0(
    PLSA_METRIC_PACK_0 pMetricPack
    )
{
    printf("Failed authentications:       %llu\n", (unsigned long long)pMetricPack->failedAuthentications);
    printf("Failed user lookups by name:  %llu\n", (unsigned long long)pMetricPack->failedUserLookupsByName);
    printf("Failed user lookups by id:    %llu\n", (unsigned long long)pMetricPack->failedUserLookupsById);
    printf("Failed group lookups by name: %llu\n", (unsigned long long)pMetricPack->failedGroupLookupsByName);
    printf("Failed group lookups by id:   %llu\n", (unsigned long long)pMetricPack->failedGroupLookupsById);
    printf("Failed session opens:         %llu\n", (unsigned long long)pMetricPack->failedOpenSession);
    printf("Failed session closures:      %llu\n", (unsigned long long)pMetricPack->failedCloseSession);
    printf("Failed password changes:      %llu\n", (unsigned long long)pMetricPack->failedChangePassword);
}

VOID
PrintMetricPack_1(
    PLSA_METRIC_PACK_1 pMetricPack
    )
{
    printf("Successful authentications:           %llu\n", (unsigned long long)pMetricPack->successfulAuthentications);
    printf("Failed authentications:               %llu\n", (unsigned long long)pMetricPack->failedAuthentications);
    printf("Attempts to lookup/authenticate root: %llu\n", (unsigned long long)pMetricPack->rootUserAuthentications);
    printf("Successful user lookups by name:      %llu\n", (unsigned long long)pMetricPack->successfulUserLookupsByName);
    printf("Failed user lookups by name:          %llu\n", (unsigned long long)pMetricPack->failedUserLookupsByName);
    printf("Successful user lookups by id:        %llu\n", (unsigned long long)pMetricPack->successfulUserLookupsById);
    printf("Failed user lookups by id:            %llu\n", (unsigned long long)pMetricPack->failedUserLookupsById);
    printf("Successful group lookups by name:     %llu\n", (unsigned long long)pMetricPack->successfulGroupLookupsByName);
    printf("Failed group lookups by name:         %llu\n", (unsigned long long)pMetricPack->failedGroupLookupsByName);
    printf("Successful group lookups by id:       %llu\n", (unsigned long long)pMetricPack->successfulGroupLookupsById);
    printf("Failed group lookups by id:           %llu\n", (unsigned long long)pMetricPack->failedGroupLookupsById);
    printf("Successful session opens:             %llu\n", (unsigned long long)pMetricPack->successfulOpenSession);
    printf("Failed session opens:                 %llu\n", (unsigned long long)pMetricPack->failedOpenSession);
    printf("Successful session closures:          %llu\n", (unsigned long long)pMetricPack->successfulCloseSession);
    printf("Failed session closures:              %llu\n", (unsigned long long)pMetricPack->failedCloseSession);
    printf("Successful password changes:          %llu\n", (unsigned long long)pMetricPack->successfulChangePassword);
    printf("Failed password changes:              %llu\n", (unsigned long long)pMetricPack->failedChangePassword);
}

DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;
    
    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
            
            dwError2 = LW_ERROR_LSA_SERVER_UNREACHABLE;
            
            break;
            
        default:
            
            break;
    }
    
    return dwError2;
}
