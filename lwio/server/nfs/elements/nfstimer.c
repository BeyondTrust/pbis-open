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
 *        nfstimer.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        Timer
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
PVOID
NfsTimerMain(
    IN  PVOID pData
    );

static
NTSTATUS
NfsTimerGetNextRequest_inlock(
    IN  PNFS_TIMER_CONTEXT  pContext,
    OUT PNFS_TIMER_REQUEST* ppTimerRequest
    );

static
NTSTATUS
NfsTimerDetachRequest_inlock(
    IN OUT PNFS_TIMER_CONTEXT pContext,
    IN OUT PNFS_TIMER_REQUEST pTimerRequest
    );

static
VOID
NfsTimerFree(
    IN  PNFS_TIMER_REQUEST pTimerRequest
    );

static
VOID
NfsTimerShutdownCB(
    PNFS_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    );

static
BOOLEAN
NfsTimerMustStop_inlock(
    IN  PNFS_TIMER_CONTEXT pContext
    );

static
VOID
NfsTimerStop(
    IN  PNFS_TIMER_CONTEXT pContext
    );

NTSTATUS
NfsTimerInit(
    IN  PNFS_TIMER pTimer
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    memset(&pTimer->context, 0, sizeof(pTimer->context));

    pthread_mutex_init(&pTimer->context.mutex, NULL);
    pTimer->context.pMutex = &pTimer->context.mutex;

    pthread_cond_init(&pTimer->context.event, NULL);
    pTimer->context.pEvent = &pTimer->context.event;

    pTimer->context.bStop = FALSE;

    status = pthread_create(
                    &pTimer->timerThread,
                    NULL,
                    &NfsTimerMain,
                    &pTimer->context);
    BAIL_ON_NT_STATUS(status);

    pTimer->pTimerThread = &pTimer->timerThread;

error:

    return status;
}

static
PVOID
NfsTimerMain(
    IN  PVOID pData
    )
{
    NTSTATUS status = 0;
    PNFS_TIMER_CONTEXT pContext = (PNFS_TIMER_CONTEXT)pData;
    PNFS_TIMER_REQUEST pTimerRequest = NULL;
    LONG64 llCurTime = 0LL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOG_DEBUG("Nfs timer starting");

    LWIO_LOCK_MUTEX(bInLock, &pContext->mutex);

    while (!NfsTimerMustStop_inlock(pContext))
    {
        int errCode = 0;
        BOOLEAN bRetryWait = FALSE;

        // If we have a current timer request, check if it is time to service it
        if (pTimerRequest)
        {
            status = WireGetCurrentNTTime(&llCurTime);
            BAIL_ON_NT_STATUS(status);

            if (llCurTime >= pTimerRequest->llExpiry)
            {
                NfsTimerDetachRequest_inlock(pContext, pTimerRequest);

                LWIO_UNLOCK_MUTEX(bInLock, &pContext->mutex);

                if (pTimerRequest->pfnTimerExpiredCB)
                {
                    // Timer has not been canceled
                    pTimerRequest->pfnTimerExpiredCB(
                                        pTimerRequest,
                                        pTimerRequest->pUserData);
                }

                LWIO_LOCK_MUTEX(bInLock, &pContext->mutex);
            }

            NfsTimerRelease(pTimerRequest);
            pTimerRequest = NULL;
        }

        // Get the next timer request in queue
        status = NfsTimerGetNextRequest_inlock(pContext, &pTimerRequest);
        if (status == STATUS_NOT_FOUND)
        {
            // If the queue is empty wait for a day or until a request arrives
            struct timespec tsLong = { .tv_sec = time(NULL) + 86400,
                                       .tv_nsec = 0 };

            status = STATUS_SUCCESS;

            do
            {
                bRetryWait = FALSE;

                errCode = pthread_cond_timedwait(
                                &pContext->event,
                                &pContext->mutex,
                                &tsLong);

                if (errCode == ETIMEDOUT)
                {
                    if (time(NULL) < tsLong.tv_sec)
                    {
                        bRetryWait = TRUE;
                        continue;
                    }

                    break;
                }

                status = LwErrnoToNtStatus(errCode);
                BAIL_ON_NT_STATUS(status);

            } while (bRetryWait && !NfsTimerMustStop_inlock(pContext));

            continue;
        }
        BAIL_ON_NT_STATUS(status);

        // At this point, we have a timer request - wait for its specified time
        do
        {
            struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};
            bRetryWait = FALSE;

            status = WireNTTimeToTimeSpec(pTimerRequest->llExpiry, &ts);
            BAIL_ON_NT_STATUS(status);

            errCode = pthread_cond_timedwait(
                            &pContext->event,
                            &pContext->mutex,
                            &ts);
            if (errCode == ETIMEDOUT)
            {
                status = WireGetCurrentNTTime(&llCurTime);
                BAIL_ON_NT_STATUS(status);

                if (llCurTime < pTimerRequest->llExpiry)
                {
                    bRetryWait = TRUE;
                    continue;
                }

                break;
            }

            status = LwErrnoToNtStatus(errCode);
            BAIL_ON_NT_STATUS(status);

        } while (bRetryWait && !NfsTimerMustStop_inlock(pContext));
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    if (pTimerRequest)
    {
        NfsTimerRelease(pTimerRequest);
    }

    LWIO_LOG_DEBUG("Nfs timer stopping");

    return NULL;

error:

    LWIO_LOG_ERROR("Nfs timer stopping due to error [%d]", status);

    goto cleanup;
}

static
NTSTATUS
NfsTimerGetNextRequest_inlock(
    IN  PNFS_TIMER_CONTEXT  pContext,
    OUT PNFS_TIMER_REQUEST* ppTimerRequest
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFS_TIMER_REQUEST pTimerRequest = NULL;

    if (!pContext->pRequests)
    {
        status = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    pTimerRequest = pContext->pRequests;

    InterlockedIncrement(&pTimerRequest->refCount);

    *ppTimerRequest = pTimerRequest;

cleanup:

    return status;

error:

    *ppTimerRequest = NULL;

    goto cleanup;
}

static
NTSTATUS
NfsTimerDetachRequest_inlock(
    IN OUT PNFS_TIMER_CONTEXT pContext,
    IN OUT PNFS_TIMER_REQUEST pTimerRequest
    )
{
    if (pTimerRequest->pPrev)
    {
        pTimerRequest->pPrev->pNext = pTimerRequest->pNext;

        if (pTimerRequest->pNext)
        {
            pTimerRequest->pNext->pPrev = pTimerRequest->pPrev;
        }
    }
    else
    {
        pContext->pRequests = pTimerRequest->pNext;

        if (pTimerRequest->pNext)
        {
            pTimerRequest->pNext->pPrev = NULL;
        }
    }

    pTimerRequest->pPrev = NULL;
    pTimerRequest->pNext = NULL;

    // Removed from timer queue
    InterlockedDecrement(&pTimerRequest->refCount);

    return STATUS_SUCCESS;
}

NTSTATUS
NfsTimerPostRequestSpecific(
    IN  PNFS_TIMER             pTimer,
    IN  LONG64                 llExpiry,
    IN  PVOID                  pUserData,
    IN  PFN_NFS_TIMER_CALLBACK pfnTimerExpiredCB,
    OUT PNFS_TIMER_REQUEST*    ppTimerRequest
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFS_TIMER_REQUEST pTimerRequest = NULL;
    PNFS_TIMER_REQUEST pTimerIter = NULL;
    PNFS_TIMER_REQUEST pPrev = NULL;
    BOOLEAN bInLock = FALSE;

    if (!llExpiry)
    {
        status = STATUS_INVALID_PARAMETER_1;
        BAIL_ON_NT_STATUS(status);
    }
    if (!pfnTimerExpiredCB)
    {
        status = STATUS_INVALID_PARAMETER_3;
        BAIL_ON_NT_STATUS(status);
    }

    status = NfsAllocateMemory(
                    sizeof(NFS_TIMER_REQUEST),
                    (PVOID*)&pTimerRequest);
    BAIL_ON_NT_STATUS(status);

    pTimerRequest->refCount = 1;

    pTimerRequest->llExpiry = llExpiry;
    pTimerRequest->pUserData = pUserData;
    pTimerRequest->pfnTimerExpiredCB = pfnTimerExpiredCB;

    LWIO_LOCK_MUTEX(bInLock, &pTimer->context.mutex);

    for (pTimerIter = pTimer->context.pRequests;
         pTimerIter && (pTimerIter->llExpiry <= llExpiry);
         pPrev = pTimerIter, pTimerIter = pTimerIter->pNext);

    if (!pPrev)
    {
        pTimerRequest->pNext = pTimer->context.pRequests;
        if (pTimer->context.pRequests)
        {
            pTimer->context.pRequests->pPrev = pTimerRequest;
        }
        pTimer->context.pRequests = pTimerRequest;
    }
    else
    {
        pTimerRequest->pNext = pPrev->pNext;
        pTimerRequest->pPrev = pPrev;
        pPrev->pNext = pTimerRequest;
        if (pTimerRequest->pNext)
        {
            pTimerRequest->pNext->pPrev = pTimerRequest;
        }
    }

    // +1 for timer queue
    InterlockedIncrement(&pTimerRequest->refCount);

    LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

    pthread_cond_signal(&pTimer->context.event);

    // +1 for caller
    InterlockedIncrement(&pTimerRequest->refCount);

    *ppTimerRequest = pTimerRequest;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

    if (pTimerRequest)
    {
        NfsTimerRelease(pTimerRequest);
    }

    return status;

error:

    *ppTimerRequest = NULL;

    goto cleanup;
}

NTSTATUS
NfsTimerCancelRequestSpecific(
    IN  PNFS_TIMER         pTimer,
    IN  PNFS_TIMER_REQUEST pTimerRequest,
    OUT PVOID*             ppUserData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PNFS_TIMER_REQUEST pIter = NULL;
    PVOID              pUserData = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pTimer->context.mutex);

    for (pIter = pTimer->context.pRequests;
         pIter && (pIter != pTimerRequest);
         pIter = pIter->pNext);

    if (pIter)
    {
        PNFS_TIMER_REQUEST pPrev = pIter->pPrev;

        if (pPrev)
        {
            pPrev->pNext = pIter->pNext;
        }
        else
        {
            pTimer->context.pRequests = pIter->pNext;
        }

        if (pIter->pNext)
        {
            pIter->pNext->pPrev = pPrev;
        }

        pIter->pPrev = NULL;
        pIter->pNext = NULL;

        pIter->pfnTimerExpiredCB = NULL;
        pUserData = pIter->pUserData;
    }
    else
    {
        status = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

    pthread_cond_signal(&pTimer->context.event);

    *ppUserData = pUserData;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

    if (pIter)
    {
        NfsTimerRelease(pIter);
    }

    return status;

error:

    goto cleanup;
}

NTSTATUS
NfsTimerRelease(
    IN  PNFS_TIMER_REQUEST pTimerRequest
    )
{
    if (InterlockedDecrement(&pTimerRequest->refCount) == 0)
    {
        NfsTimerFree(pTimerRequest);
    }

    return STATUS_SUCCESS;
}

static
VOID
NfsTimerFree(
    IN  PNFS_TIMER_REQUEST pTimerRequest
    )
{
    NfsFreeMemory(pTimerRequest);
}

NTSTATUS
NfsTimerIndicateStop(
    IN  PNFS_TIMER pTimer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_TIMER_REQUEST pTimerRequest = NULL;

    if (pTimer->pTimerThread)
    {
        NfsTimerStop(&pTimer->context);

        // Wake up the timer thread
        ntStatus = NfsTimerPostRequest(
                        1LL,
                        NULL,
                        &NfsTimerShutdownCB,
                        &pTimerRequest);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    if (pTimerRequest)
    {
        NfsTimerRelease(pTimerRequest);
    }

    return ntStatus;
}

static
VOID
NfsTimerShutdownCB(
    PNFS_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    )
{
}

VOID
NfsTimerFreeContents(
    IN  PNFS_TIMER pTimer
    )
{
    if (pTimer->pTimerThread)
    {
        NfsTimerStop(&pTimer->context);

        pthread_join(pTimer->timerThread, NULL);
    }

    if (pTimer->context.pEvent)
    {
        pthread_cond_destroy(&pTimer->context.event);
        pTimer->context.pEvent = NULL;
    }

    while(pTimer->context.pRequests)
    {
        PNFS_TIMER_REQUEST pRequest = pTimer->context.pRequests;

        pTimer->context.pRequests = pTimer->context.pRequests->pNext;

        NfsTimerRelease(pRequest);
    }

    if (pTimer->context.pMutex)
    {
        pthread_mutex_destroy(&pTimer->context.mutex);
        pTimer->context.pMutex = NULL;
    }
}

static
BOOLEAN
NfsTimerMustStop_inlock(
    IN  PNFS_TIMER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    bStop = pContext->bStop;

    return bStop;
}

static
VOID
NfsTimerStop(
    IN  PNFS_TIMER_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);
}
