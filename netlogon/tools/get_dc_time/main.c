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
 *        Likewise Site Manager
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
