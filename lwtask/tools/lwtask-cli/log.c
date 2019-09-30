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
 *        log.c
 *
 * Abstract:
 *
 *        BeyondTrust Task Client
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
