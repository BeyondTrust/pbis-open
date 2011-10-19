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
 *        Likewise IO (LWIO)
 *
 *        Tool to set the SMBSS Log Level at runtime
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    LWIO_LOG_LEVEL* pLogLevel
    );

VOID
ShowUsage();

DWORD
PrintLogInfo(
    PLWIO_LOG_INFO pLogInfo
    );

DWORD
MapErrorCode(
    DWORD dwError
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    LWIO_LOG_LEVEL logLevel = LWIO_LOG_LEVEL_ERROR;
    PIO_CONTEXT pContext = (HANDLE)NULL;
    PLWIO_LOG_INFO pLogInfo = NULL;

    if (geteuid() != 0) {
        fprintf(stderr, "This program requires super-user privileges.\n");
        dwError = EACCES;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = ParseArgs(argc, argv, &logLevel);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwIoOpenContext(&pContext);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwIoSetLogLevel(
        (HANDLE) pContext,
        logLevel);
    BAIL_ON_LWIO_ERROR(dwError);

    fprintf(stdout, "The log level was set successfully\n\n");

    dwError = LwIoGetLogInfo(
        (HANDLE) pContext,
        &pLogInfo);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = PrintLogInfo(pLogInfo);
    BAIL_ON_LWIO_ERROR(dwError);

cleanup:

    if (pLogInfo)
    {
        LwIoFreeLogInfo(pLogInfo);
    }

    if (pContext != NULL) {
        LwIoCloseContext(pContext);
    }

    LwIoShutdown();

    return (dwError);

error:

    dwError = MapErrorCode(dwError);

    fprintf(
        stderr,
        "Failed to set log level.  Error code %u (%s).\n%s\n",
        dwError,
        LW_PRINTF_STRING(LwNtStatusToName(dwError)),
        LW_PRINTF_STRING(LwNtStatusToDescription(dwError)));

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    LWIO_LOG_LEVEL* pLogLevel
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    LWIO_LOG_LEVEL logLevel = LWIO_LOG_LEVEL_ERROR;
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
                        logLevel = LWIO_LOG_LEVEL_ERROR;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "warning"))
                    {
                        logLevel = LWIO_LOG_LEVEL_WARNING;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "info"))
                    {
                        logLevel = LWIO_LOG_LEVEL_INFO;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "verbose"))
                    {
                        logLevel = LWIO_LOG_LEVEL_VERBOSE;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "debug"))
                    {
                        logLevel = LWIO_LOG_LEVEL_DEBUG;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "trace"))
                    {
                        logLevel = LWIO_LOG_LEVEL_TRACE;
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
    printf("Usage: lw-smb-set-log-level {error, warning, info, verbose, debug, trace}\n");
}

DWORD
PrintLogInfo(
    PLWIO_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;

    fprintf(stdout, "Current log settings:\n");
    fprintf(stdout, "=================\n");
    switch(pLogInfo->logTarget)
    {
        case LWIO_LOG_TARGET_DISABLED:
            fprintf(stdout, "Logging is currently disabled\n");
            break;
        case LWIO_LOG_TARGET_CONSOLE:
            fprintf(stdout, "SMB Server is logging to console\n");
            break;
        case LWIO_LOG_TARGET_FILE:
            fprintf(stdout, "SMB Server is logging to file.\n");
            fprintf(stdout, "Log file path: %s\n", pLogInfo->pszPath);
            break;
        case LWIO_LOG_TARGET_SYSLOG:
            fprintf(stdout, "SMB Server is logging to syslog\n");
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LWIO_ERROR(dwError);
    }

    fprintf(stdout, "Maximum allowed log level: ");
    switch(pLogInfo->maxAllowedLogLevel)
    {
        case LWIO_LOG_LEVEL_ERROR:
            fprintf(stdout, "%s\n", "error");
            break;
        case LWIO_LOG_LEVEL_WARNING:
            fprintf(stdout, "%s\n", "warning");
            break;
        case LWIO_LOG_LEVEL_INFO:
            fprintf(stdout, "%s\n", "info");
            break;
        case LWIO_LOG_LEVEL_VERBOSE:
            fprintf(stdout, "%s\n", "verbose");
            break;
        case LWIO_LOG_LEVEL_DEBUG:
            fprintf(stdout, "%s\n", "debug");
            break;
        case LWIO_LOG_LEVEL_TRACE:
            fprintf(stdout, "%s\n", "trace");
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LWIO_ERROR(dwError);
    }

error:

    return dwError;
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

            dwError2 = LWIO_ERROR_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}
