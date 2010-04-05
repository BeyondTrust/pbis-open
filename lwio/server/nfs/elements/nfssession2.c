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
 *        nfssession.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        Session Object
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
NfsSession2AcquireTreeId_inlock(
   PLWIO_NFS_SESSION_2 pSession,
   PULONG              pulTid
   );

static
int
NfsSession2TreeCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
NfsSession2TreeRelease(
    PVOID pTree
    );

static
VOID
NfsSession2Free(
    PLWIO_NFS_SESSION_2 pSession
    );

static
NTSTATUS
NfsSession2RundownTreeRbTreeVisit(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

NTSTATUS
NfsSession2Create(
    ULONG64              ullUid,
    PLWIO_NFS_SESSION_2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_SESSION_2 pSession = NULL;

    LWIO_LOG_DEBUG("Creating session [uid:%lu]", ullUid);

    ntStatus = NfsAllocateMemory(
                    sizeof(LWIO_NFS_SESSION_2),
                    (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->refcount = 1;

    pthread_rwlock_init(&pSession->mutex, NULL);
    pSession->pMutex = &pSession->mutex;

    pSession->ullUid = ullUid;

    LWIO_LOG_DEBUG("Associating session [object:0x%x][uid:%lu]",
                    pSession,
                    ullUid);

    ntStatus = LwRtlRBTreeCreate(
                    &NfsSession2TreeCompare,
                    NULL,
                    &NfsSession2TreeRelease,
                    &pSession->pTreeCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsFinderCreateRepository(&pSession->hFinderRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    NFS_ELEMENTS_INCREMENT_SESSIONS;

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        NfsSession2Release(pSession);
    }

    goto cleanup;
}

NTSTATUS
NfsSession2FindTree(
    PLWIO_NFS_SESSION_2 pSession,
    ULONG               ulTid,
    PLWIO_NFS_TREE_2*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_NFS_TREE_2 pTree = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    pTree = pSession->lruTree[ulTid % NFS_LRU_CAPACITY];

    if (!pTree || (pTree->ulTid != ulTid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pSession->pTreeCollection,
                        &ulTid,
                        (PVOID*)&pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->lruTree[ ulTid % NFS_LRU_CAPACITY ] = pTree;
    }

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_NETWORK_NAME_DELETED;
    }

    *ppTree = NULL;

    goto cleanup;
}

NTSTATUS
NfsSession2RemoveTree(
    PLWIO_NFS_SESSION_2 pSession,
    ULONG               ulTid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_NFS_TREE_2 pTree = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    pTree = pSession->lruTree[ ulTid % NFS_LRU_CAPACITY ];
    if (pTree && (pTree->ulTid == ulTid))
    {
        pSession->lruTree[ ulTid % NFS_LRU_CAPACITY ] = NULL;
    }

    ntStatus = LwRtlRBTreeRemove(
                    pSession->pTreeCollection,
                    &ulTid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsSession2CreateTree(
    PLWIO_NFS_SESSION_2 pSession,
    PNFS_SHARE_INFO     pShareInfo,
    PLWIO_NFS_TREE_2*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_TREE_2 pTree = NULL;
    BOOLEAN bInLock = FALSE;
    ULONG   ulTid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = NfsSession2AcquireTreeId_inlock(
                    pSession,
                    &ulTid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsTree2Create(
                    ulTid,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pSession->pTreeCollection,
                    &pTree->ulTid,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        NfsTree2Release(pTree);
    }

    goto cleanup;
}

PLWIO_NFS_SESSION_2
NfsSession2Acquire(
    PLWIO_NFS_SESSION_2 pSession
    )
{
    LWIO_LOG_DEBUG("Acquiring session [uid:%u]", pSession->ullUid);

    InterlockedIncrement(&pSession->refcount);

    return pSession;
}

VOID
NfsSession2Release(
    PLWIO_NFS_SESSION_2 pSession
    )
{
    LWIO_LOG_DEBUG("Releasing session [uid:%u]", pSession->ullUid);

    if (InterlockedDecrement(&pSession->refcount) == 0)
    {
        NFS_ELEMENTS_DECREMENT_SESSIONS;

        NfsSession2Free(pSession);
    }
}

VOID
NfsSession2Rundown(
    PLWIO_NFS_SESSION_2 pSession
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    LwRtlRBTreeTraverse(
            pSession->pTreeCollection,
            LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
            NfsSession2RundownTreeRbTreeVisit,
            NULL);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);
}

static
NTSTATUS
NfsSession2AcquireTreeId_inlock(
   PLWIO_NFS_SESSION_2 pSession,
   PULONG              pulTid
   )
{
    NTSTATUS ntStatus = 0;
    ULONG    ulCandidateTid = pSession->ulNextAvailableTid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_NFS_TREE_2 pTree = NULL;

        /* 0 is never a valid tid */

        if ((ulCandidateTid == 0) || (ulCandidateTid == UINT32_MAX))
        {
            ulCandidateTid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pSession->pTreeCollection,
                        &ulCandidateTid,
                        (PVOID*)&pTree);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            ulCandidateTid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((ulCandidateTid != pSession->ulNextAvailableTid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_LINKS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulTid = ulCandidateTid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    ulCandidateTid++;
    pSession->ulNextAvailableTid = ulCandidateTid ? ulCandidateTid : 1;

cleanup:

    return ntStatus;

error:

    *pulTid = 0;

    goto cleanup;
}

static
int
NfsSession2TreeCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG pTid1 = (PULONG)pKey1;
    PULONG pTid2 = (PULONG)pKey2;

    assert (pTid1 != NULL);
    assert (pTid2 != NULL);

    if (*pTid1 > *pTid2)
    {
        return 1;
    }
    else if (*pTid1 < *pTid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
NfsSession2TreeRelease(
    PVOID pTree
    )
{
    NfsTree2Release((PLWIO_NFS_TREE_2)pTree);
}

static
VOID
NfsSession2Free(
    PLWIO_NFS_SESSION_2 pSession
    )
{
    LWIO_LOG_DEBUG("Freeing session [object:0x%x][uid:%u]",
                    pSession,
                    pSession->ullUid);

    if (pSession->pMutex)
    {
        pthread_rwlock_destroy(&pSession->mutex);
        pSession->pMutex = NULL;
    }

    if (pSession->pTreeCollection)
    {
        LwRtlRBTreeFree(pSession->pTreeCollection);
    }

    if (pSession->hFinderRepository)
    {
        NfsFinderCloseRepository(pSession->hFinderRepository);
    }

    IO_SAFE_FREE_MEMORY(pSession->pszClientPrincipalName);

    if (pSession->pIoSecurityContext) {
        IoSecurityDereferenceSecurityContext(&pSession->pIoSecurityContext);
    }

    NfsFreeMemory(pSession);
}

static
NTSTATUS
NfsSession2RundownTreeRbTreeVisit(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_NFS_TREE_2 pTree = (PLWIO_NFS_TREE_2)pData;

    if (pTree)
    {
        NfsTree2Rundown(pTree);
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
