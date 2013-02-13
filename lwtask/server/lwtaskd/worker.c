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

#include "includes.h"

static
PVOID
LwTaskWorkerMain(
    PVOID pData
    );

static
DWORD
LwTaskSrvExecute(
    PLW_TASK_CONTEXT pContext
    );

static
DWORD
LwTaskSrvExecuteMigrate(
    PLW_TASK_CONTEXT pContext
    );

static
DWORD
LwTaskGetNextExecutionContext(
    struct timespec*  pTimespec,
    PLW_TASK_CONTEXT* ppContext
    );

static
BOOLEAN
LwTaskWorkerMustStop(
    PLW_TASK_SRV_WORKER_CONTEXT pContext
    );

static
VOID
LwTaskWorkerIndicateStopContext(
    PLW_TASK_SRV_WORKER_CONTEXT pContext
    );

DWORD
LwTaskWorkerInit(
    PLW_TASK_SRV_WORKER pWorker
    )
{
    DWORD dwError = 0;

    memset(&pWorker->context, 0, sizeof(pWorker->context));

    pthread_mutex_init(&pWorker->context.mutex, NULL);
    pWorker->context.pMutex = &pWorker->context.mutex;

    pWorker->context.bStop = FALSE;
    pWorker->context.dwWorkerId = pWorker->dwWorkerId;

    dwError = pthread_create(
                    &pWorker->worker,
                    NULL,
                    &LwTaskWorkerMain,
                    &pWorker->context);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pWorker->pWorker = &pWorker->worker;

error:

    return dwError;
}

static
PVOID
LwTaskWorkerMain(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PLW_TASK_SRV_WORKER_CONTEXT pWorkContext = (PLW_TASK_SRV_WORKER_CONTEXT)pData;
    PLW_TASK_CONTEXT pContext = NULL;
    struct timespec ts = {0, 0};

    LW_TASK_LOG_DEBUG("worker [id:%u] starting", pWorkContext->dwWorkerId);

    while (!LwTaskWorkerMustStop(pWorkContext))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

        dwError = LwTaskGetNextExecutionContext(&ts, &pContext);
        if (dwError == ERROR_TIMEOUT)
        {
            dwError = 0;
        }
        BAIL_ON_LW_TASK_ERROR(dwError);

        if (LwTaskWorkerMustStop(pWorkContext))
        {
            break;
        }

        if (pContext)
        {
            if (LwTaskIsValidContext(pContext))
            {
                DWORD dwError2 = LwTaskSrvExecute(pContext);
                if (dwError2)
                {
                    LW_TASK_LOG_VERBOSE("Failed to execute server task [code:%d]", dwError2);
                }
            }

            LwTaskReleaseContext(pContext);
            pContext = NULL;
        }
    }

cleanup:

    if (pContext)
    {
        LwTaskReleaseContext(pContext);
    }

    LW_TASK_LOG_DEBUG("worker [id:%u] stopping", pWorkContext->dwWorkerId);

    return NULL;

error:

    goto cleanup;
}

static
DWORD
LwTaskSrvExecute(
    PLW_TASK_CONTEXT pContext
    )
{
    DWORD dwError = 0;

    LwTaskSrvInitStatus(pContext->pTask);

    switch (pContext->pTask->taskType)
    {
        case LW_TASK_TYPE_MIGRATE:

            dwError = LwTaskSrvExecuteMigrate(pContext);

            break;

        default:

            dwError = ERROR_NOT_SUPPORTED;

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

    LwTaskSrvSetPercentComplete(pContext->pTask, 100);

cleanup:

    return dwError;

error:

    LwTaskSrvSetErrorCode(pContext->pTask, dwError);

    goto cleanup;
}

static
DWORD
LwTaskSrvExecuteMigrate(
    PLW_TASK_CONTEXT pContext
    )
{
    DWORD dwError = 0;
    PSTR  pszRemoteServer   = NULL; // Do not free
    PSTR  pszRemoteShare    = NULL; // Do not free
    PSTR  pszRemoteUser     = NULL; // Do not free
    PSTR  pszRemotePassword = NULL; // Do not free
    PLW_TASK_ARG pArg = NULL;       // Do not free
    PLW_SHARE_MIGRATION_CONTEXT pMigrateContext = NULL;

    LW_TASK_LOG_INFO(   "Starting execution of task [id:%d]",
                        pContext->pTask->dwTaskId);

    pArg = LwTaskFindArg(
                LW_TASK_MIGRATE_ARG_REMOTE_SERVER_NAME,
                LW_TASK_ARG_TYPE_STRING,
                pContext->pArgArray,
                pContext->dwNumArgs);
    if (!pArg)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    pszRemoteServer = pArg->pszArgValue;

    pArg = LwTaskFindArg(
                LW_TASK_MIGRATE_ARG_REMOTE_SHARE_NAME,
                LW_TASK_ARG_TYPE_STRING_MULTI_CSV,
                pContext->pArgArray,
                pContext->dwNumArgs);
    if (!pArg)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    pszRemoteShare = pArg->pszArgValue;

    pArg = LwTaskFindArg(
                LW_TASK_MIGRATE_ARG_REMOTE_USER_PRINCIPAL,
                LW_TASK_ARG_TYPE_STRING,
                pContext->pArgArray,
                pContext->dwNumArgs);
    if (!pArg)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    pszRemoteUser = pArg->pszArgValue;

    pArg = LwTaskFindArg(
                LW_TASK_MIGRATE_ARG_REMOTE_PASSWORD,
                LW_TASK_ARG_TYPE_STRING,
                pContext->pArgArray,
                pContext->dwNumArgs);
    if (!pArg)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    pszRemotePassword = pArg->pszArgValue;

    dwError = LwTaskMigrateCreateContext(
                        pszRemoteUser,
                        pszRemotePassword,
                        &pMigrateContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (IsNullOrEmptyString(pszRemoteShare) || strchr(pszRemoteShare, (int)','))
    {
        dwError = LwTaskMigrateMultipleSharesA(
                        pMigrateContext,
                        pszRemoteServer,
                        pszRemoteShare,
                        0);
    }
    else
    {
        dwError = LwTaskMigrateShareA(
                        pMigrateContext,
                        pszRemoteServer,
                        pszRemoteShare,
                        0);
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    LW_TASK_LOG_INFO(   "Completed execution of task [id:%d]",
                        pContext->pTask->dwTaskId);

    if (pMigrateContext)
    {
        LwTaskMigrateCloseContext(pMigrateContext);
    }

    return dwError;

error:

    LW_TASK_LOG_ERROR(  "Error executing task [id:%d] [code: %u]",
                        pContext->pTask->dwTaskId,
                        dwError);

    goto cleanup;
}

VOID
LwTaskWorkerIndicateStop(
    PLW_TASK_SRV_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        LwTaskWorkerIndicateStopContext(&pWorker->context);
    }
}

VOID
LwTaskWorkerFreeContents(
    PLW_TASK_SRV_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        // Someone must have already called SrvWorkerIndicateStop
        // and unblocked the prod/cons queue.

        pthread_join(pWorker->worker, NULL);
    }

    if (pWorker->context.pMutex)
    {
        pthread_mutex_destroy(pWorker->context.pMutex);
        pWorker->context.pMutex = NULL;
    }
}

static
DWORD
LwTaskGetNextExecutionContext(
    struct timespec*  pTimespec,
    PLW_TASK_CONTEXT* ppContext
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PLW_TASK_CONTEXT pContext = NULL;

    dwError = LwTaskProdConsTimedDequeue(
                    &gLwTaskSrvGlobals.workQueue,
                    pTimespec,
                    (PVOID*)&pContext);
    if (dwError != ERROR_SUCCESS)
    {
        goto error;
    }

    *ppContext = pContext;

cleanup:

    return dwError;

error:

    *ppContext = NULL;

    goto cleanup;
}

static
BOOLEAN
LwTaskWorkerMustStop(
    PLW_TASK_SRV_WORKER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
VOID
LwTaskWorkerIndicateStopContext(
    PLW_TASK_SRV_WORKER_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);
}
