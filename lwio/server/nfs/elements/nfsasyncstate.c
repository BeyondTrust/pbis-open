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
 *        nfsasyncstate.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        Asynchronous State
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
VOID
NfsAsyncStateFree(
    PLWIO_ASYNC_STATE pAsyncState
    );

ULONG64
NfsAsyncStateBuildId(
    ULONG  ulPid,
    USHORT usMid
    )
{
    return (((ULONG64)ulPid) << 32) | ((ULONG64)usMid);
}

NTSTATUS
NfsAsyncStateCreate(
    ULONG64                         ullAsyncId,
    USHORT                          usCommand,
    HANDLE                          hAsyncState,
    PFN_LWIO_NFS_CANCEL_ASYNC_STATE pfnCancelAsyncState,
    PFN_LWIO_NFS_FREE_ASYNC_STATE   pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*              ppAsyncState
    )
{
    NTSTATUS          ntStatus    = STATUS_SUCCESS;
    PLWIO_ASYNC_STATE pAsyncState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(LWIO_ASYNC_STATE),
                    (PVOID*)&pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pAsyncState->refcount = 1;

    pthread_rwlock_init(&pAsyncState->mutex, NULL);
    pAsyncState->pMutex = &pAsyncState->mutex;

    pAsyncState->ullAsyncId        = ullAsyncId;
    pAsyncState->usCommand         = usCommand;
    pAsyncState->hAsyncState       = hAsyncState;
    pAsyncState->pfnFreeAsyncState = pfnFreeAsyncState;
    pAsyncState->pfnCancelAsyncState = pfnCancelAsyncState;

    *ppAsyncState = pAsyncState;

cleanup:

    return ntStatus;

error:

    *ppAsyncState = NULL;

    goto cleanup;
}

VOID
NfsAsyncStateCancel(
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    if (pAsyncState->hAsyncState && pAsyncState->pfnCancelAsyncState)
    {
        pAsyncState->pfnCancelAsyncState(pAsyncState->hAsyncState);
    }
}

PLWIO_ASYNC_STATE
NfsAsyncStateAcquire(
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    InterlockedIncrement(&pAsyncState->refcount);

    return pAsyncState;
}

VOID
NfsAsyncStateRelease(
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    if (InterlockedDecrement(&pAsyncState->refcount) == 0)
    {
        NfsAsyncStateFree(pAsyncState);
    }
}

static
VOID
NfsAsyncStateFree(
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    if (pAsyncState->hAsyncState && pAsyncState->pfnFreeAsyncState)
    {
        pAsyncState->pfnFreeAsyncState(pAsyncState->hAsyncState);
    }

    if (pAsyncState->pMutex)
    {
        pthread_rwlock_destroy(&pAsyncState->mutex);
    }

    NfsFreeMemory(pAsyncState);
}
