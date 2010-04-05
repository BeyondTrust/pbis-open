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
 *        shareapi.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) Server subsystem
 *
 *        Server share list handling interface
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
NfsShareFindByName_inlock(
    IN  PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PNFS_SHARE_INFO*           ppShareInfo
    );

static
NTSTATUS
NfsShareRemoveFromList_inlock(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    );

static
int
NfsShareCompareInfo(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
NfsShareReleaseInfoHandle(
    PVOID hShareInfo
    );

static
VOID
NfsShareFreeInfo(
    PNFS_SHARE_INFO pShareInfo
    );

NTSTATUS
NfsShareInitList(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE   hRepository = NULL;
    HANDLE   hResume = NULL;
    PNFS_SHARE_INFO* ppShareInfoList = NULL;
    PNFS_SHARE_ENTRY pShareEntry = NULL;
    ULONG            ulBatchLimit  = 256;
    ULONG            ulNumSharesFound = 0;

    pthread_rwlock_init(&pShareList->mutex, NULL);
    pShareList->pMutex = &pShareList->mutex;

    pShareList->pShareEntry = NULL;

    ntStatus = LwRtlRBTreeCreate(
                    &NfsShareCompareInfo,
                    NULL,
                    &NfsShareReleaseInfoHandle,
                    &pShareList->pShareCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryBeginEnum(
                                            hRepository,
                                            ulBatchLimit,
                                            &hResume);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ULONG iShare = 0;

        if (ppShareInfoList)
        {
            NfsShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
            ppShareInfoList = NULL;
            ulNumSharesFound = 0;
        }

        ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryEnum(
                                                hRepository,
                                                hResume,
                                                &ppShareInfoList,
                                                &ulNumSharesFound);
        BAIL_ON_NT_STATUS(ntStatus);

        for (; iShare < ulNumSharesFound; iShare++)
        {
            PNFS_SHARE_INFO pShareInfo = ppShareInfoList[iShare];

            ntStatus = NfsAllocateMemory(
                            sizeof(NFS_SHARE_ENTRY),
                            (PVOID*)&pShareEntry);
            BAIL_ON_NT_STATUS(ntStatus);

            pShareEntry->pInfo = NfsShareAcquireInfo(pShareInfo);

            pShareEntry->pNext = pShareList->pShareEntry;
            pShareList->pShareEntry = pShareEntry;
            pShareEntry = NULL;

            ntStatus = LwRtlRBTreeAdd(
                            pShareList->pShareCollection,
                            pShareInfo->pwszName,
                            pShareInfo);
            BAIL_ON_NT_STATUS(ntStatus);

            InterlockedIncrement(&pShareInfo->refcount);
        }

    } while (ulNumSharesFound == ulBatchLimit);

cleanup:

    if (hResume)
    {
        gNfsShareApi.pFnTable->pfnShareRepositoryEndEnum(
                                    hRepository,
                                    hResume);
    }

    if (hRepository)
    {
        gNfsShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    if (ppShareInfoList)
    {
        NfsShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    return ntStatus;

error:

    NfsShareFreeListContents(pShareList);

    goto cleanup;
}

NTSTATUS
NfsShareFindByName(
    IN  PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PNFS_SHARE_INFO*           ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareList->mutex);

    ntStatus = NfsShareFindByName_inlock(
                        pShareList,
                        pwszShareName,
                        ppShareInfo);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;
}

static
NTSTATUS
NfsShareFindByName_inlock(
    IN  PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PNFS_SHARE_INFO*           ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_SHARE_INFO pShareInfo = NULL;

    ntStatus = LwRtlRBTreeFind(
                    pShareList->pShareCollection,
                    pwszShareName,
                    (PVOID*)&pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppShareInfo = NfsShareAcquireInfo(pShareInfo);

cleanup:

    return ntStatus;

error:

    *ppShareInfo = NULL;

    goto cleanup;
}


NTSTATUS
NfsShareAdd(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName,
    IN     PWSTR                      pwszSharePath,
    IN     PWSTR                      pwszShareComment,
    IN     PBYTE                      pSecDesc,
    IN     ULONG                      ulSecDescLen,
    IN     PWSTR                      pwszShareType
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PNFS_SHARE_ENTRY pShareEntry = NULL;
    PNFS_SHARE_INFO pShareInfo = NULL;
    wchar16_t wszServiceType[] = LWIO_NFS_SHARE_STRING_ID_DISK_W;
    wchar16_t wszShareComment[] = {0};
    PWSTR     pwszShareCommentRef =
                    (pwszShareComment ? pwszShareComment : &wszShareComment[0]);
    HANDLE hRepository = NULL;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (IsNullOrEmptyString(pwszSharePath))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pwszShareType)
    {
        pwszShareType = &wszServiceType[0];
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = NfsShareFindByName_inlock(
                        pShareList,
                        pwszShareName,
                        &pShareInfo);
    if (!ntStatus)
    {
        ntStatus = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_SHARE_INFO),
                    (PVOID*)&pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo->refcount = 1;

    pthread_rwlock_init(&pShareInfo->mutex, NULL);
    pShareInfo->pMutex = &pShareInfo->mutex;

    ntStatus = NfsAllocateStringW(
                    pwszShareName,
                    &pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAllocateStringW(
                    pwszSharePath,
                    &pShareInfo->pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAllocateStringW(
                        pwszShareCommentRef,
                        &pShareInfo->pwszComment);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulSecDescLen)
    {
        ntStatus = NfsShareSetSecurity(
                       pShareInfo,
                       (PSECURITY_DESCRIPTOR_RELATIVE)pSecDesc,
                       ulSecDescLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        pShareInfo->pSecDesc     = NULL;
        pShareInfo->ulSecDescLen = 0;
    }

    ntStatus = NfsShareMapServiceStringToIdW(
                    pwszShareType,
                    &pShareInfo->service);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo->bMarkedForDeletion = FALSE;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_SHARE_ENTRY),
                    (PVOID*)&pShareEntry);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareEntry->pInfo = NfsShareAcquireInfo(pShareInfo);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: We only allow adding disk sharess
    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryAdd(
                                            hRepository,
                                            pShareInfo->pwszName,
                                            pShareInfo->pwszPath,
                                            pShareInfo->pwszComment,
                                            (PBYTE)pShareInfo->pSecDesc,
                                            pShareInfo->ulSecDescLen,
                                            pwszShareType);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pShareList->pShareCollection,
                    pShareInfo->pwszName,
                    pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo = NULL;

    pShareEntry->pNext = pShareList->pShareEntry;
    pShareList->pShareEntry = pShareEntry;

cleanup:

    if (hRepository)
    {
        gNfsShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    if (pShareInfo)
    {
        NfsShareReleaseInfo(pShareInfo);
    }

    return ntStatus;

error:

    if (pShareEntry)
    {
        NfsShareFreeEntry(pShareEntry);
    }

    goto cleanup;
}

NTSTATUS
NfsShareUpdate(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PNFS_SHARE_INFO pShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszServiceType[] = LWIO_NFS_SHARE_STRING_ID_DISK_W;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bShareInLock = FALSE;
    HANDLE   hRepository = NULL;

    if (!pShareInfo || IsNullOrEmptyString(pShareInfo->pwszName))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryDelete(
                                            hRepository,
                                            pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bShareInLock, &pShareInfo->mutex);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryAdd(
                                            hRepository,
                                            pShareInfo->pwszName,
                                            pShareInfo->pwszPath,
                                            pShareInfo->pwszComment,
                                            (PBYTE)pShareInfo->pSecDesc,
                                            pShareInfo->ulSecDescLen,
                                            wszServiceType);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pShareInfo->mutex);

    gNfsShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    hRepository = NULL;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pShareInfo->mutex);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    if (hRepository)
    {
        gNfsShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    goto cleanup;
}

NTSTATUS
NfsShareDelete(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    HANDLE   hRepository = NULL;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareList->mutex);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryOpen(&hRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = gNfsShareApi.pFnTable->pfnShareRepositoryDelete(
                                            hRepository,
                                            pwszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    if (hRepository)
    {
        gNfsShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
        hRepository = NULL;
    }

    ntStatus = NfsShareRemoveFromList_inlock(
                        pShareList,
                        pwszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeRemove(
                        pShareList->pShareCollection,
                        pwszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hRepository)
    {
        gNfsShareApi.pFnTable->pfnShareRepositoryClose(hRepository);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
NfsShareRemoveFromList_inlock(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    )
{
    PNFS_SHARE_ENTRY pShareEntry = NULL;
    PNFS_SHARE_ENTRY pPrevShareEntry = NULL;
    ULONG ulRemoved = 0;

    pShareEntry = pShareList->pShareEntry;

    while (pShareEntry)
    {
        if (SMBWc16sCaseCmp(pwszShareName, pShareEntry->pInfo->pwszName) == 0)
        {
            if (pPrevShareEntry)
            {
                pPrevShareEntry->pNext = pShareEntry->pNext;
            }
            else
            {
                pShareList->pShareEntry = pShareEntry->pNext;
            }

            pShareEntry->pNext = NULL;
            NfsShareFreeEntry(pShareEntry);

            ulRemoved++;

            break;
        }

        pPrevShareEntry = pShareEntry;
        pShareEntry     = pShareEntry->pNext;
    }

    return (ulRemoved ? STATUS_SUCCESS : STATUS_NOT_FOUND);
}

NTSTATUS
NfsShareEnum(
    IN      PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    OUT     PNFS_SHARE_INFO**          pppShareInfo,
    IN  OUT PULONG                     pulNumEntries
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulCount = 0;
    BOOLEAN bInLock = FALSE;
    PNFS_SHARE_ENTRY pShareEntry = NULL;
    PNFS_SHARE_INFO* ppShares = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareList->mutex);

    /* Count the number of share entries */
    pShareEntry = pShareList->pShareEntry;
    while (pShareEntry)
    {
        pShareEntry = pShareEntry->pNext;
        ulCount++;
    }

    if (ulCount)
    {
        ULONG i = 0;

        ntStatus = NfsAllocateMemory(
                        ulCount * sizeof(PNFS_SHARE_INFO),
                        (PVOID*)&ppShares);
        BAIL_ON_NT_STATUS(ntStatus);

        pShareEntry = pShareList->pShareEntry;
        for (; i < ulCount; i++)
        {
            ntStatus = NfsShareDuplicateInfo(pShareEntry->pInfo, &ppShares[i]);
            BAIL_ON_NT_STATUS(ntStatus);

            pShareEntry = pShareEntry->pNext;
        }
    }

    *pppShareInfo   = ppShares;
    *pulNumEntries  = ulCount;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareList->mutex);

    return ntStatus;

error:

    if (ppShares)
    {
        NfsShareFreeInfoList(ppShares, ulCount);
    }

    *pppShareInfo   = NULL;
    *pulNumEntries = 0;

    goto cleanup;
}

NTSTATUS
NfsShareDuplicateInfo(
    PNFS_SHARE_INFO  pShareInfo,
    PNFS_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;
    PNFS_SHARE_INFO pShareInfoCopy = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_SHARE_INFO),
                    (PVOID*)&pShareInfoCopy);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfoCopy->refcount = 1;

    pthread_rwlock_init(&pShareInfoCopy->mutex, NULL);
    pShareInfoCopy->pMutex = &pShareInfoCopy->mutex;

    if (pShareInfo->pwszName)
    {
        ntStatus = NfsAllocateStringW(
                        pShareInfo->pwszName,
                        &pShareInfoCopy->pwszName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pShareInfo->pwszPath)
    {
        ntStatus = NfsAllocateStringW(
                        pShareInfo->pwszPath,
                        &pShareInfoCopy->pwszPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pShareInfo->pwszComment)
    {
        ntStatus = NfsAllocateStringW(
                        pShareInfo->pwszComment,
                        &pShareInfoCopy->pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pShareInfo->ulSecDescLen)
    {
        ntStatus = NfsShareSetSecurity(
                        pShareInfoCopy,
                        pShareInfo->pSecDesc,
                        pShareInfo->ulSecDescLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pShareInfoCopy->service = pShareInfo->service;

    pShareInfoCopy->bMarkedForDeletion = pShareInfo->bMarkedForDeletion;

    *ppShareInfo = pShareInfoCopy;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;

error:

    *ppShareInfo = NULL;

    if (pShareInfoCopy)
    {
        NfsShareFreeInfo(pShareInfoCopy);
    }

    goto cleanup;
}

static
int
NfsShareCompareInfo(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PWSTR pwszShareName1 = (PWSTR)pKey1;
    PWSTR pwszShareName2 = (PWSTR)pKey2;

    return SMBWc16sCaseCmp(pwszShareName1, pwszShareName2);
}

static
VOID
NfsShareReleaseInfoHandle(
    PVOID hShareInfo
    )
{
    NfsShareReleaseInfo((PNFS_SHARE_INFO)hShareInfo);
}

VOID
NfsShareFreeListContents(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList
    )
{
    if (pShareList->pShareEntry)
    {
        NfsShareFreeEntry(pShareList->pShareEntry);
        pShareList->pShareEntry = NULL;
    }

    if (pShareList->pShareCollection)
    {
        LwRtlRBTreeFree(pShareList->pShareCollection);
    }

    if (pShareList->pMutex)
    {
        pthread_rwlock_destroy(&pShareList->mutex);
        pShareList->pMutex = NULL;
    }
}

VOID
NfsShareFreeEntry(
    IN PNFS_SHARE_ENTRY pShareEntry
    )
{
    while (pShareEntry)
    {
        PNFS_SHARE_ENTRY pTmpShareEntry = pShareEntry;

        pShareEntry = pShareEntry->pNext;

        if (pTmpShareEntry->pInfo)
        {
            NfsShareReleaseInfo(pTmpShareEntry->pInfo);
        }

        NfsFreeMemory(pTmpShareEntry);
    }
}

VOID
NfsShareFreeInfoList(
    PNFS_SHARE_INFO* ppInfoList,
    ULONG            ulNumInfos
    )
{
    ULONG iInfo = 0;

    for (; iInfo < ulNumInfos; iInfo++)
    {
        PNFS_SHARE_INFO pShareInfo = ppInfoList[iInfo];

        if (pShareInfo)
        {
            NfsShareReleaseInfo(pShareInfo);
        }
    }

    NfsFreeMemory(ppInfoList);
}

PNFS_SHARE_INFO
NfsShareAcquireInfo(
    IN PNFS_SHARE_INFO pShareInfo
    )
{
    InterlockedIncrement(&pShareInfo->refcount);

    return pShareInfo;
}

VOID
NfsShareReleaseInfo(
    IN PNFS_SHARE_INFO pShareInfo
    )
{
    if (InterlockedDecrement(&pShareInfo->refcount) == 0)
    {
        NfsShareFreeInfo(pShareInfo);
    }
}

static
VOID
NfsShareFreeInfo(
    IN PNFS_SHARE_INFO pShareInfo
    )
{
    if (pShareInfo->pMutex)
    {
        pthread_rwlock_destroy(&pShareInfo->mutex);
    }

    if (pShareInfo->pwszName)
    {
        NfsFreeMemory(pShareInfo->pwszName);
    }
    if (pShareInfo->pwszPath)
    {
        NfsFreeMemory(pShareInfo->pwszPath);
    }
    if (pShareInfo->pwszComment)
    {
        NfsFreeMemory(pShareInfo->pwszComment);
    }

    NfsShareFreeSecurity(pShareInfo);

    NfsFreeMemory(pShareInfo);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
