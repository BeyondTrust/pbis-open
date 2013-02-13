/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        log.c
 *
 * Abstract:
 *
 *        Likewise Task Client
 *
 *        Log Request Handler
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
DWORD
LwTaskCliGetLogInfo(
    VOID
    );

static
DWORD
LwTaskCliSetLogLevel(
    PCSTR pszLogLevel
    );

DWORD
LwTaskHandleLogRequest(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;

    if (!argc)
    {
        goto cleanup;
    }

    if (!strcmp(argv[0], "get-info"))
    {
        dwError = LwTaskCliGetLogInfo();
    }
    else if (!strcmp(argv[0], "set-level"))
    {
        if (argc < 2)
        {
            dwError = ERROR_BAD_ARGUMENTS;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwError = LwTaskCliSetLogLevel(argv[1]);
    }
    else
    {
        dwError = ERROR_BAD_ARGUMENTS;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskCliGetLogInfo(
    VOID
    )
{
    DWORD dwError = 0;
    PLW_TASK_CLIENT_CONNECTION pConnection = NULL;
    PLW_TASK_LOG_INFO pLogInfo = NULL;

    dwError = LwTaskOpenServer(&pConnection);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskGetLogInfo(pConnection, &pLogInfo);
    BAIL_ON_LW_TASK_ERROR(dwError);

    fprintf(stdout, "Log settings: \n\n");

    fprintf(stdout,
            "Maximum allowed log level: %s\n",
            LwTaskLogLevelGetLabel(pLogInfo->maxAllowedLogLevel));

    switch (pLogInfo->logTarget)
    {
        case LW_TASK_LOG_TARGET_SYSLOG:

            fprintf(stdout, "Target: syslog\n");

            break;

        case LW_TASK_LOG_TARGET_FILE:

            fprintf(stdout, "Target: file\n");
            fprintf(stdout, "Location: %s\n",
                    LW_TASK_SAFE_LOG_STRING(pLogInfo->pszPath));

            break;

        case LW_TASK_LOG_TARGET_CONSOLE:

            fprintf(stdout, "Target: console\n");

            break;

        case LW_TASK_LOG_TARGET_DISABLED:

            fprintf(stdout, "Target: disabled\n");

            break;

        default:

            fprintf(stdout, "Target: unknown\n");

            break;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pLogInfo);

    if (pConnection)
    {
        LwTaskCloseServer(pConnection);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskCliSetLogLevel(
    PCSTR pszLogLevel
    )
{
    DWORD dwError = 0;
    PLW_TASK_CLIENT_CONNECTION pConnection = NULL;
    LW_TASK_LOG_LEVEL logLevel = LW_TASK_LOG_LEVEL_ERROR;

    if (!strcmp(pszLogLevel, "error"))
    {
        logLevel = LW_TASK_LOG_LEVEL_ERROR;
    } else if (!strcmp(pszLogLevel, "warning"))
    {
        logLevel = LW_TASK_LOG_LEVEL_WARNING;
    } else if (!strcmp(pszLogLevel, "info"))
    {
        logLevel = LW_TASK_LOG_LEVEL_INFO;
    } else if (!strcmp(pszLogLevel, "verbose"))
    {
        logLevel = LW_TASK_LOG_LEVEL_VERBOSE;
    } else if (!strcmp(pszLogLevel, "debug"))
    {
        logLevel = LW_TASK_LOG_LEVEL_DEBUG;
    }
    else
    {
        dwError = ERROR_BAD_ARGUMENTS;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = LwTaskOpenServer(&pConnection);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSetLogLevel(pConnection, logLevel);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    if (pConnection)
    {
        LwTaskCloseServer(pConnection);
    }

    return dwError;

error:

    goto cleanup;
}
