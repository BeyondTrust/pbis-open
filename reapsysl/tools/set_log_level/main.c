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
 *        Reaper for syslog 
 *        
 *        Tool to set the daemon Log Level at runtime
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "rsys-system.h"
#include "rsys-def.h"
#include <rsys/rsys.h>
#include "rsys-logging.h"
#include "rsys-utils.h"
#include <lw/rtlmemory.h>
#include <lw/rtlstring.h>

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    RSysLogLevel* pLogLevel
    );

VOID
ShowUsage();

DWORD
PrintLogInfo(
    PRSYS_LOG_INFO pLogInfo
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    RSysLogLevel logLevel = RSYS_LOG_LEVEL_ERROR;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PRSYS_LOG_INFO pLogInfo = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    
    dwError = ParseArgs(argc, argv, &logLevel);
    BAIL_ON_RSYS_ERROR(dwError);
    
    dwError = RSysOpenServer(&hLsaConnection);
    BAIL_ON_RSYS_ERROR(dwError);
    
    dwError = RSysSetLogLevel(
                    hLsaConnection,
                    logLevel);
    BAIL_ON_RSYS_ERROR(dwError);
    
    fprintf(stdout, "The log level was set successfully\n\n");
    
    dwError = RSysGetLogInfo(
                    hLsaConnection,
                    &pLogInfo);
    BAIL_ON_RSYS_ERROR(dwError);
    
    dwError = PrintLogInfo(pLogInfo);
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:

    if (pLogInfo)
    {
        RSysFreeLogInfo(pLogInfo);
    }
    
    if (hLsaConnection != (HANDLE)NULL) {
        RSysCloseServer(hLsaConnection);
    }

    return (dwError);

error:

    dwErrorBufferSize = RSysGetErrorString(dwError, NULL, 0);
    
    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;
        
        dwError2 = RTL_ALLOCATE(
                        &pszErrorBuffer,
                        char,
                        dwErrorBufferSize);
        
        if (!dwError2)
        {
            DWORD dwLen = RSysGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);
            
            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to set log level.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        RtlCStringFree(&pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to set log level. Error code [%d]\n", dwError);
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    RSysLogLevel* pLogLevel
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    RSysLogLevel logLevel = RSYS_LOG_LEVEL_ERROR;
    BOOLEAN bLogLevelSpecified = FALSE;

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
                    if (!strcasecmp(pszArg, "error"))
                    {
                        logLevel = RSYS_LOG_LEVEL_ERROR;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "warning"))
                    {
                        logLevel = RSYS_LOG_LEVEL_WARNING;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "info"))
                    {
                        logLevel = RSYS_LOG_LEVEL_INFO;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "verbose"))
                    {
                        logLevel = RSYS_LOG_LEVEL_VERBOSE;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "debug"))
                    {
                        logLevel = RSYS_LOG_LEVEL_DEBUG;
                        bLogLevelSpecified = TRUE;
                    }
                    else
                    {
                        ShowUsage();
                        exit(1);
                    }
                }
                break;
        }
        
    } while (iArg < argc);
    
    if (!bLogLevelSpecified)
    {
        ShowUsage();
        exit(1);
    }

    *pLogLevel = logLevel;
    
    return dwError;
}

void
ShowUsage()
{
    printf("Usage: lw-set-log-level {error, warning, info, verbose}\n");
}

DWORD
PrintLogInfo(
    PRSYS_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    
    fprintf(stdout, "Current log settings:\n");
    fprintf(stdout, "=================\n");
    switch(pLogInfo->logTarget)
    {
        case RSYS_LOG_TARGET_DISABLED:
            fprintf(stdout, "Logging is currently disabled\n");
            break;
        case RSYS_LOG_TARGET_CONSOLE:
            fprintf(stdout, "LSA Server is logging to console\n");
            break;
        case RSYS_LOG_TARGET_FILE:
            fprintf(stdout, "LSA Server is logging to file.\n");
            fprintf(stdout, "Log file path: %s\n", pLogInfo->pszPath);
            break;
        case RSYS_LOG_TARGET_SYSLOG:
            fprintf(stdout, "LSA Server is logging to syslog\n");
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_RSYS_ERROR(dwError);
    }
    
    fprintf(stdout, "Maximum allowed log level: ");
    switch(pLogInfo->maxAllowedLogLevel)
    {
        case RSYS_LOG_LEVEL_ERROR:
            fprintf(stdout, "%s\n", "error");
            break;
        case RSYS_LOG_LEVEL_WARNING:
            fprintf(stdout, "%s\n", "warning");
            break;
        case RSYS_LOG_LEVEL_INFO:
            fprintf(stdout, "%s\n", "info");
            break;
        case RSYS_LOG_LEVEL_VERBOSE:
            fprintf(stdout, "%s\n", "verbose");
            break;
        case RSYS_LOG_LEVEL_DEBUG:
            fprintf(stdout, "%s\n", "debug");
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_RSYS_ERROR(dwError);
    }
    
error:

    return dwError;
}
