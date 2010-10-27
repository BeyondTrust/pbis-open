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
Nfs3WorkerMain(
    PVOID pData
    );

static
NTSTATUS
Nfs3GetNextExecContext(
    struct timespec*    pTimespec,
    PNFS3_EXEC_CONTEXT* ppContext
    );

static
BOOLEAN
Nfs3WorkerMustStop(
    PNFS3_WORKER_CONTEXT pContext
    );

static
VOID
Nfs3WorkerIndicateStopContext(
    PNFS3_WORKER_CONTEXT pContext
    );

NTSTATUS
Nfs3WorkerInit(
    PNFS3_WORKER pWorker,
    ULONG        ulCpu
    )
{
    NTSTATUS ntStatus = 0;
    LONG lError = 0;
    pthread_attr_t threadAttr;
    pthread_attr_t* pThreadAttr = NULL;

    memset(&pWorker->context, 0, sizeof(pWorker->context));

    pthread_mutex_init(&pWorker->context.mutex, NULL);
    pWorker->context.pMutex = &pWorker->context.mutex;

    pWorker->context.bStop = FALSE;
    pWorker->context.workerId = pWorker->workerId;

    // Create threadAttr with affinity to ulCpu, ignoring errors
    lError = pthread_attr_init(&threadAttr);
    ntStatus = LwErrnoToNtStatus(lError);
    if (!NT_SUCCESS(ntStatus))
    {
        LWIO_LOG_ERROR("Error initializing pthread_attr_t");
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = LwRtlSetAffinityThreadAttribute(&threadAttr, ulCpu);
        if (!NT_SUCCESS(ntStatus))
        {
            LWIO_LOG_ERROR("Error setting thread affinity");
            ntStatus = STATUS_SUCCESS;
        }
        else
        {
            pThreadAttr = &threadAttr;
        }
    }

    ntStatus = pthread_create(
                    &pWorker->worker,
                    pThreadAttr,
                    &Nfs3WorkerMain,
                    &pWorker->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pWorker->pWorker = &pWorker->worker;

error:

    return ntStatus;
}

static
PVOID
Nfs3WorkerMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_WORKER_CONTEXT pWorkerContext = (PNFS3_WORKER_CONTEXT)pData;
    PNFS3_EXEC_CONTEXT pExecContext = NULL;
    struct timespec ts = {0, 0};

    LWIO_LOG_DEBUG("Srv worker [id:%u] starting", pWorkerContext->workerId);

    while (!Nfs3WorkerMustStop(pWorkerContext))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

        ntStatus = Nfs3GetNextExecContext(&ts, &pExecContext);
        if (ntStatus == STATUS_IO_TIMEOUT)
        {
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (Nfs3WorkerMustStop(pWorkerContext))
        {
            break;
        }

        if (pExecContext)
        {
            if (Nfs3IsValidExecContext(pExecContext))
            {
                NTSTATUS ntStatus2 = Nfs3ProtocolExecute(pExecContext);
                if (ntStatus2)
                {
                    LWIO_LOG_VERBOSE("Failed to execute server task [code:%d]",
                                     ntStatus2);
                }
            }

            Nfs3ReleaseExecContext(pExecContext);
            pExecContext = NULL;
        }
    }

cleanup:

    if (pExecContext)
    {
        Nfs3ReleaseExecContext(pExecContext);
    }

    LWIO_LOG_DEBUG("Srv worker [id:%u] stopping", pWorkerContext->workerId);

    return NULL;

error:

    goto cleanup;
}

VOID
Nfs3WorkerIndicateStop(
    PNFS3_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        Nfs3WorkerIndicateStopContext(&pWorker->context);
    }
}

VOID
Nfs3WorkerFreeContents(
    PNFS3_WORKER pWorker
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
NTSTATUS
Nfs3GetNextExecContext(
    struct timespec*    pTimespec,
    PNFS3_EXEC_CONTEXT* ppExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_EXEC_CONTEXT pExecContext = NULL;

    ntStatus = Nfs3ProdConsTimedDequeue(
                    gNfs3Globals.pWorkQueue,
                    pTimespec,
                    (PVOID*)&pExecContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        goto error;
    }

    *ppExecContext = pExecContext;

cleanup:

    return ntStatus;

error:

    *ppExecContext = NULL;

    goto cleanup;
}

static
BOOLEAN
Nfs3WorkerMustStop(
    PNFS3_WORKER_CONTEXT pWorkerContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pWorkerContext->mutex);

    bStop = pWorkerContext->bStop;

    pthread_mutex_unlock(&pWorkerContext->mutex);

    return bStop;
}

static
VOID
Nfs3WorkerIndicateStopContext(
    PNFS3_WORKER_CONTEXT pWorkerContext
    )
{
    pthread_mutex_lock(&pWorkerContext->mutex);

    pWorkerContext->bStop = TRUE;

    pthread_mutex_unlock(&pWorkerContext->mutex);
}
