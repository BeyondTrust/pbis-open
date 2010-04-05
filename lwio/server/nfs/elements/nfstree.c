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
 *        nfstree.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        Tree Object
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
NfsTreeAcquireFileId_inlock(
   PLWIO_NFS_TREE pTree,
   PUSHORT       pFid
   );

static
int
NfsTreeFileCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
NfsTreeFileRelease(
    PVOID pFile
    );

static
VOID
NfsTreeFree(
    PLWIO_NFS_TREE pTree
    );

static
int
NfsTreeAsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
NfsTreeAsyncStateRelease(
    PVOID pAsyncState
    );

static
NTSTATUS
NfsTreeRundownAsyncStatesRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
NfsTreeRundownFileRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

NTSTATUS
NfsTreeCreate(
    USHORT          tid,
    PNFS_SHARE_INFO pShareInfo,
    PLWIO_NFS_TREE* ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_TREE pTree = NULL;

    LWIO_LOG_DEBUG("Creating Tree [tid: %u]", tid);

    ntStatus = NfsAllocateMemory(sizeof(LWIO_NFS_TREE), (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->refcount = 1;

    pthread_rwlock_init(&pTree->mutex, NULL);
    pTree->pMutex = &pTree->mutex;

    pTree->tid = tid;

    LWIO_LOG_DEBUG("Associating Tree [object:0x%x][tid:%u]",
                    pTree,
                    tid);

    pTree->pShareInfo = NfsShareAcquireInfo(pShareInfo);

    ntStatus = LwRtlRBTreeCreate(
                    &NfsTreeFileCompare,
                    NULL,
                    &NfsTreeFileRelease,
                    &pTree->pFileCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeCreate(
                    &NfsTreeAsyncStateCompare,
                    NULL,
                    &NfsTreeAsyncStateRelease,
                    &pTree->pAsyncStateCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    NFS_ELEMENTS_INCREMENT_TREE_CONNECTS;

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        NfsTreeRelease(pTree);
    }

    goto cleanup;
}

NTSTATUS
NfsTreeFindFile(
    PLWIO_NFS_TREE  pTree,
    USHORT         fid,
    PLWIO_NFS_FILE* ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_FILE pFile = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    pFile = pTree->lruFile[fid % NFS_LRU_CAPACITY];

    if (!pFile || (pFile->fid != fid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pTree->pFileCollection,
                        &fid,
                        (PVOID*)&pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->lruFile[fid % NFS_LRU_CAPACITY] = pFile;
    }

    *ppFile = NfsFileAcquire(pFile);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_INVALID_HANDLE;
    }

    *ppFile = NULL;

    goto cleanup;
}

NTSTATUS
NfsTreeCreateFile(
    PLWIO_NFS_TREE          pTree,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_NFS_FILE*         ppFile
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_NFS_FILE pFile = NULL;
    USHORT  fid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = NfsTreeAcquireFileId_inlock(
                    pTree,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsFileCreate(
                    fid,
                    pwszFilename,
                    phFile,
                    ppFilename,
                    desiredAccess,
                    allocationSize,
                    fileAttributes,
                    shareAccess,
                    createDisposition,
                    createOptions,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pTree->pFileCollection,
                    &pFile->fid,
                    pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppFile = NfsFileAcquire(pFile);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppFile = NULL;

    if (pFile)
    {
        NfsFileRelease(pFile);
    }

    goto cleanup;
}

NTSTATUS
NfsTreeRemoveFile(
    PLWIO_NFS_TREE pTree,
    USHORT        fid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_NFS_FILE pFile = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    pFile = pTree->lruFile[ fid % NFS_LRU_CAPACITY ];
    if (pFile && (pFile->fid == fid))
    {
        pTree->lruFile[ fid % NFS_LRU_CAPACITY ] = NULL;
    }

    ntStatus = LwRtlRBTreeRemove(
                    pTree->pFileCollection,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsTreeAddAsyncState(
    PLWIO_NFS_TREE    pTree,
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PLWIO_ASYNC_STATE pAsyncState1 = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pTree->pAsyncStateCollection,
                    &pAsyncState->ullAsyncId,
                    (PVOID*)&pAsyncState1);
    switch (ntStatus)
    {
        case STATUS_NOT_FOUND:

            ntStatus = LwRtlRBTreeAdd(
                            pTree->pAsyncStateCollection,
                            &pAsyncState->ullAsyncId,
                            pAsyncState);
            BAIL_ON_NT_STATUS(ntStatus);

            NfsAsyncStateAcquire(pAsyncState);

            break;

        case STATUS_SUCCESS:

            ntStatus = STATUS_DUPLICATE_OBJECTID;

            break;

        default:

            ;
    }
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

NTSTATUS
NfsTreeFindAsyncState(
    PLWIO_NFS_TREE     pTree,
    ULONG64            ullAsyncId,
    PLWIO_ASYNC_STATE* ppAsyncState
    )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PLWIO_ASYNC_STATE pAsyncState = NULL;
    BOOLEAN           bInLock     = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pTree->pAsyncStateCollection,
                    &ullAsyncId,
                    (PVOID*)&pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAsyncState = NfsAsyncStateAcquire(pAsyncState);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppAsyncState = NULL;

    goto cleanup;
}

NTSTATUS
NfsTreeRemoveAsyncState(
    PLWIO_NFS_TREE pTree,
    ULONG64        ullAsyncId
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pTree->pAsyncStateCollection,
                    &ullAsyncId);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

BOOLEAN
NfsTreeIsNamedPipe(
    PLWIO_NFS_TREE pTree
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    bResult = (pTree->pShareInfo->service == SHARE_SERVICE_NAMED_PIPE);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    return bResult;
}

NTSTATUS
NfsGetTreeRelativePath(
    PWSTR  pwszOriginalPath,
    PWSTR* ppwszSpecificPath
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    wchar16_t wszBackSlash[] = { '\\', 0 };
    wchar16_t wszFwdSlash[]  = { '/',  0 };

    if ((*pwszOriginalPath != wszBackSlash[0]) &&
         (*pwszOriginalPath != wszFwdSlash[0]))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    pwszOriginalPath++;

    // Skip the device name
    while (!IsNullOrEmptyString(pwszOriginalPath) &&
           (*pwszOriginalPath != wszBackSlash[0]) &&
           (*pwszOriginalPath != wszFwdSlash[0]))
    {
        pwszOriginalPath++;
    }

    if (IsNullOrEmptyString(pwszOriginalPath) ||
        ((*pwszOriginalPath != wszBackSlash[0]) &&
         (*pwszOriginalPath != wszFwdSlash[0])))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszSpecificPath = pwszOriginalPath;

cleanup:

    return ntStatus;

error:

    *ppwszSpecificPath = NULL;

    goto cleanup;
}

PLWIO_NFS_TREE
NfsTreeAcquire(
    PLWIO_NFS_TREE pTree
    )
{
    LWIO_LOG_DEBUG("Acquiring tree [tid:%u]", pTree->tid);

    InterlockedIncrement(&pTree->refcount);

    return pTree;
}

VOID
NfsTreeRelease(
    PLWIO_NFS_TREE pTree
    )
{
    LWIO_LOG_DEBUG("Releasing tree [tid:%u]", pTree->tid);

    if (InterlockedDecrement(&pTree->refcount) == 0)
    {
        NFS_ELEMENTS_DECREMENT_TREE_CONNECTS;

        NfsTreeFree(pTree);
    }
}

VOID
NfsTreeRundown(
    PLWIO_NFS_TREE pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    LwRtlRBTreeTraverse(
                pTree->pAsyncStateCollection,
                LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                NfsTreeRundownAsyncStatesRbTreeVisit,
                NULL);

	if (pTree->pAsyncStateCollection)
	{
    	LwRtlRBTreeRemoveAll(pTree->pAsyncStateCollection);
	}

    LwRtlRBTreeTraverse(
            pTree->pFileCollection,
            LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
            NfsTreeRundownFileRbTreeVisit,
            NULL);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);
}

static
NTSTATUS
NfsTreeAcquireFileId_inlock(
   PLWIO_NFS_TREE pTree,
   PUSHORT       pFid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateFid = pTree->nextAvailableFid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_NFS_FILE pFile = NULL;

        /* 0 is never a valid fid */

        if ((candidateFid == 0) || (candidateFid == UINT16_MAX))
        {
            candidateFid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pTree->pFileCollection,
                        &candidateFid,
                        (PVOID*)&pFile);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            candidateFid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateFid != pTree->nextAvailableFid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_OPENED_FILES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pFid = candidateFid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    candidateFid++;
    pTree->nextAvailableFid = candidateFid ? candidateFid : 1;

cleanup:

    return ntStatus;

error:

    *pFid = 0;

    goto cleanup;
}

static
int
NfsTreeFileCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pFid1 = (PUSHORT)pKey1;
    PUSHORT pFid2 = (PUSHORT)pKey2;

    if (*pFid1 > *pFid2)
    {
        return 1;
    }
    else if (*pFid1 < *pFid2)
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
NfsTreeFileRelease(
    PVOID pFile
    )
{
    NfsFileRelease((PLWIO_NFS_FILE)pFile);
}

static
VOID
NfsTreeFree(
    PLWIO_NFS_TREE pTree
    )
{
    LWIO_LOG_DEBUG("Freeing tree [object:0x%x][tid:%u]",
                    pTree,
                    pTree->tid);

    if (pTree->pMutex)
    {
        pthread_rwlock_destroy(&pTree->mutex);
        pTree->pMutex = NULL;
    }

    if (pTree->pFileCollection)
    {
        LwRtlRBTreeFree(pTree->pFileCollection);
    }

    if (pTree->pAsyncStateCollection)
    {
        LwRtlRBTreeFree(pTree->pAsyncStateCollection);
    }

    if (pTree->hFile)
    {
        IoCloseFile(pTree->hFile);
    }

    if (pTree->pShareInfo)
    {
        NfsShareReleaseInfo(pTree->pShareInfo);
    }

    NfsFreeMemory(pTree);
}

static
int
NfsTreeAsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG64 pAsyncId1 = (PULONG64)pKey1;
    PULONG64 pAsyncId2 = (PULONG64)pKey2;

    if (*pAsyncId1 > *pAsyncId2)
    {
        return 1;
    }
    else if (*pAsyncId1 < *pAsyncId2)
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
NfsTreeAsyncStateRelease(
    PVOID pAsyncState
    )
{
    NfsAsyncStateRelease((PLWIO_ASYNC_STATE)pAsyncState);
}

static
NTSTATUS
NfsTreeRundownAsyncStatesRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_ASYNC_STATE pAsyncState = (PLWIO_ASYNC_STATE)pData;

    if (pAsyncState)
    {
        NfsAsyncStateCancel(pAsyncState);
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NfsTreeRundownFileRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_NFS_FILE pFile = (PLWIO_NFS_FILE)pData;

    if (pFile)
    {
        NfsFileRundown(pFile);
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
