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
NfsWorkerMain(
    PVOID pData
    );

static
NTSTATUS
NfsGetNextExecutionContext(
    struct timespec*   pTimespec,
    PNFS_EXEC_CONTEXT* ppContext
    );

static
BOOLEAN
NfsWorkerMustStop(
    PLWIO_NFS_WORKER_CONTEXT pContext
    );

static
VOID
NfsWorkerIndicateStopContext(
    PLWIO_NFS_WORKER_CONTEXT pContext
    );

NTSTATUS
NfsWorkerInit(
    PLWIO_NFS_WORKER pWorker
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pWorker->context, 0, sizeof(pWorker->context));

    pthread_mutex_init(&pWorker->context.mutex, NULL);
    pWorker->context.pMutex = &pWorker->context.mutex;

    pWorker->context.bStop = FALSE;
    pWorker->context.workerId = pWorker->workerId;

    ntStatus = pthread_create(
                    &pWorker->worker,
                    NULL,
                    &NfsWorkerMain,
                    &pWorker->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pWorker->pWorker = &pWorker->worker;

error:

    return ntStatus;
}

static
PVOID
NfsWorkerMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_WORKER_CONTEXT pContext = (PLWIO_NFS_WORKER_CONTEXT)pData;
    PNFS_EXEC_CONTEXT pExecContext = NULL;
    struct timespec ts = {0, 0};

    LWIO_LOG_DEBUG("Nfs worker [id:%u] starting", pContext->workerId);

    while (!NfsWorkerMustStop(pContext))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

        ntStatus = NfsGetNextExecutionContext(&ts, &pExecContext);
        if (ntStatus == STATUS_IO_TIMEOUT)
        {
            ntStatus = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (NfsWorkerMustStop(pContext))
        {
            break;
        }

        if (pExecContext)
        {
            if (NfsIsValidExecContext(pExecContext))
            {
                NTSTATUS ntStatus2 = NfsProtocolExecute(pExecContext);
                if (ntStatus2)
                {
                    LWIO_LOG_ERROR("Failed to execute server task [code:%d]", ntStatus2);
                }
            }

            NfsReleaseExecContext(pExecContext);
            pExecContext = NULL;
        }
    }

cleanup:

    if (pExecContext)
    {
        NfsReleaseExecContext(pExecContext);
    }

    LWIO_LOG_DEBUG("Nfs worker [id:%u] stopping", pContext->workerId);

    return NULL;

error:

    goto cleanup;
}

VOID
NfsWorkerIndicateStop(
    PLWIO_NFS_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        NfsWorkerIndicateStopContext(&pWorker->context);
    }
}

VOID
NfsWorkerFreeContents(
    PLWIO_NFS_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        // Someone must have already called NfsWorkerIndicateStop
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
NTSTATUS
NfsGetNextExecutionContext(
    struct timespec*   pTimespec,
    PNFS_EXEC_CONTEXT* ppContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT pContext = NULL;

    status = NfsProdConsTimedDequeue(
                    &gSMBNfsGlobals.workQueue,
                    pTimespec,
                    (PVOID*)&pContext);
    if (status != STATUS_SUCCESS)
    {
        goto error;
    }

    *ppContext = pContext;

cleanup:

    return status;

error:

    *ppContext = NULL;

    goto cleanup;
}

static
BOOLEAN
NfsWorkerMustStop(
    PLWIO_NFS_WORKER_CONTEXT pContext
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
NfsWorkerIndicateStopContext(
    PLWIO_NFS_WORKER_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);
}

