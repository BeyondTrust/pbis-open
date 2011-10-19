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
 *        lwio-semaphore.h
 *
 * Abstract:
 *
 *        Likewise IO
 *
 *        Semaphore Code
 *
 * Author: Danilo Almeida (dalmeida@likewise.com)
 */

#include "rdr.h"

// Need to check the OS after including config.h, which is
// included by includes.h.

#if defined(__LWI_DARWIN__)
#define UNIX_TO_NTSTATUS(Error) \
   ((Error) ? LwErrnoToNtStatus(Error) : STATUS_SUCCESS)

NTSTATUS
SMBSemaphoreInit(
    OUT PLSMB_SEMAPHORE pSemaphore,
    IN DWORD Count
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN IsMutexInitialized = FALSE;
    BOOLEAN IsConditionInitialized = FALSE;
    int error = 0;

    error = pthread_mutex_init(&pSemaphore->Mutex, NULL);
    ntStatus = UNIX_TO_NTSTATUS(error);
    BAIL_ON_NT_STATUS(ntStatus);
    IsMutexInitialized = TRUE;

    error = pthread_cond_init(&pSemaphore->Condition, NULL);
    ntStatus = UNIX_TO_NTSTATUS(error);
    BAIL_ON_NT_STATUS(ntStatus);
    IsConditionInitialized = TRUE;

    pSemaphore->Count = Count;

error:
    if (ntStatus)
    {
        if (IsConditionInitialized)
        {
            pthread_cond_destroy(&pSemaphore->Condition);
        }
        if (IsMutexInitialized)
        {
            pthread_mutex_destroy(&pSemaphore->Mutex);
        }
    }

    return ntStatus;
}

NTSTATUS
SMBSemaphoreWait(
    IN PLSMB_SEMAPHORE pSemaphore
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    int error = 0;

    LWIO_LOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    while (pSemaphore->Count <= 0)
    {
        error = pthread_cond_wait(&pSemaphore->Condition, &pSemaphore->Mutex);
        ntStatus = UNIX_TO_NTSTATUS(error);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    pSemaphore->Count--;

error:
    LWIO_UNLOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    return ntStatus;
}

NTSTATUS
SMBSemaphorePost(
    IN PLSMB_SEMAPHORE pSemaphore
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    int error = 0;

    LWIO_LOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    pSemaphore->Count++;
    error = pthread_cond_signal(&pSemaphore->Condition);
    assert(!error);
    ntStatus = UNIX_TO_NTSTATUS(error);

    LWIO_UNLOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    return ntStatus;
}

VOID
SMBSemaphoreDestroy(
    IN OUT PLSMB_SEMAPHORE pSemaphore
    )
{
    int error = 0;
    error = pthread_cond_destroy(&pSemaphore->Condition);
    if (error)
    {
        LWIO_LOG_ERROR("Failed to destroy semaphore condition [code: %d]", error);
    }
    error = pthread_mutex_destroy(&pSemaphore->Mutex);
    if (error)
    {
        LWIO_LOG_ERROR("Failed to destroy semaphore mutex [code: %d]", error);
    }
    pSemaphore->Count = 0;
}
#endif /* __LWI_DARWIN__ */
