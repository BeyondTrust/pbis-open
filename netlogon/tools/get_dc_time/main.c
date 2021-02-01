/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 *
 *        Client Test Program - LWNetGetDCTime
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"
#include "lwerror.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
void
ShowUsage()
{
    printf("Usage: get-dc-time <target domain FQDN>\n");
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszTargetFQDN
    )
{

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszTargetFQDN = NULL;

    do {
        pszArg = argv[iArg++];
        if (IsNullOrEmptyString(pszArg))
        {
            break;
        }
        if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
            dwError = LWNetAllocateString(pszArg, &pszTargetFQDN);
            BAIL_ON_LWNET_ERROR(dwError);
        }
        
    } while (iArg < argc);

    
    if(IsNullOrEmptyString(pszTargetFQDN))
    {
        ShowUsage();
        exit(0);
    }

    cleanup:
        *ppszTargetFQDN = pszTargetFQDN;
    return dwError;

    error:
        LWNET_SAFE_FREE_STRING(pszTargetFQDN);
        *ppszTargetFQDN = NULL;
    goto cleanup;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD  dwError = 0;
    PCSTR  pszTimeFormat = "%Y-%m-%d %H:%M:%S %Z";
    PSTR   pszTargetFQDN = NULL;
    CHAR   szTime[256] = "";
    LWNET_UNIX_TIME_T dcTime = 0;
    time_t dcTimeCopy = 0;
    struct tm dcDateTime = { 0 };
    CHAR  szErrorBuf[1024];
    
    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszTargetFQDN);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetDCTime(
                pszTargetFQDN,
                &dcTime);
    BAIL_ON_LWNET_ERROR(dwError); 

    dcTimeCopy = dcTime;
    if (!localtime_r(&dcTimeCopy, &dcDateTime))
    {
        fprintf(stderr, "Error: Failed to convert DC time\n");
        dwError = ERROR_INVALID_TIME;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (!strftime(szTime, sizeof(szTime)-1, pszTimeFormat, &dcDateTime))
    {
        fprintf(stderr, "Error: Failed to format DC time\n");
        dwError = ERROR_INVALID_TIME;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    printf("DC TIME: %s\n", szTime);

error:

    if (dwError)
    {
        DWORD dwLen = LwGetErrorString(dwError, szErrorBuf, 1024);

        if (dwLen)
        {
            fprintf(
                stderr,
                "Failed to query time on domain controller.  Error code %u (%s).\n%s\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                szErrorBuf);
        }
        else
        {
            fprintf(
                stderr,
                "Failed to query time on domain controller.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
        }
    }

    LWNET_SAFE_FREE_STRING(pszTargetFQDN);
    
    return (dwError);
}
