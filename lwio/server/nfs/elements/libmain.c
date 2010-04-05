/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        File Sharing Essential Elements
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
NfsElementsInit(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int      iIter = 0;

    status = WireGetCurrentNTTime(&gNfsElements.llBootTime);
    BAIL_ON_NT_STATUS(status);

    while (!RAND_status() && (iIter++ < 10))
    {
        uuid_t uuid;
        CHAR   szUUID[37];

        memset(szUUID, 0, sizeof(szUUID));

        uuid_generate(uuid);
        uuid_unparse(uuid, szUUID);

        RAND_seed(szUUID, sizeof(szUUID));
    }

    status = NfsTimerInit(&gNfsElements.timer);
    BAIL_ON_NT_STATUS(status);

    pthread_rwlock_init(&gNfsElements.statsLock, NULL);
    gNfsElements.pStatsLock = &gNfsElements.statsLock;

error:

    return status;
}

NTSTATUS
NfsTimerPostRequest(
    IN  LONG64                 llExpiry,
    IN  PVOID                  pUserData,
    IN  PFN_NFS_TIMER_CALLBACK pfnTimerExpiredCB,
    OUT PNFS_TIMER_REQUEST*    ppTimerRequest
    )
{
    return NfsTimerPostRequestSpecific(
                &gNfsElements.timer,
                llExpiry,
                pUserData,
                pfnTimerExpiredCB,
                ppTimerRequest);
}

NTSTATUS
NfsTimerCancelRequest(
    IN  PNFS_TIMER_REQUEST pTimerRequest,
    PVOID*                 ppUserData
    )
{
    return NfsTimerCancelRequestSpecific(
                &gNfsElements.timer,
                pTimerRequest,
                ppUserData);
}

NTSTATUS
NfsElementsGetBootTime(
    PULONG64 pullBootTime
    )
{
    LONG64   llBootTime = 0LL;
    BOOLEAN  bInLock    = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gNfsElements.mutex);

    llBootTime = gNfsElements.llBootTime;

    LWIO_UNLOCK_MUTEX(bInLock, &gNfsElements.mutex);

    *pullBootTime = llBootTime;

    return STATUS_SUCCESS;
}

BOOLEAN
NfsElementsGetShareNameEcpEnabled(
    VOID
    )
{
    return gNfsElements.bShareNameEcpEnabled;
}

NTSTATUS
NfsElementsGetStats(
    PNFS_ELEMENTS_STATISTICS pStats
    )
{
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gNfsElements.statsLock);

    *pStats = gNfsElements.stats;

    LWIO_UNLOCK_RWMUTEX(bInLock, &gNfsElements.statsLock);

    return STATUS_SUCCESS;
}

NTSTATUS
NfsElementsResetStats(
    VOID
    )
{
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gNfsElements.statsLock);

    memset(&gNfsElements.stats, 0, sizeof(gNfsElements.stats));

    LWIO_UNLOCK_RWMUTEX(bInLock, &gNfsElements.statsLock);

    return STATUS_SUCCESS;
}

NTSTATUS
NfsElementsShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = NfsTimerIndicateStop(&gNfsElements.timer);
    BAIL_ON_NT_STATUS(ntStatus);

    NfsTimerFreeContents(&gNfsElements.timer);

    if (gNfsElements.pHintsBuffer != NULL)
    {
        NfsFreeMemory(gNfsElements.pHintsBuffer);
        gNfsElements.pHintsBuffer = NULL;
        gNfsElements.ulHintsLength = 0;
    }

    if (gNfsElements.pStatsLock)
    {
        pthread_rwlock_destroy(&gNfsElements.statsLock);
        gNfsElements.pStatsLock = NULL;
    }

error:

    return ntStatus;
}

