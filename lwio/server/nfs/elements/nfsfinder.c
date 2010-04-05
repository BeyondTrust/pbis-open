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
 *        nfsfinder.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        File and Directory object finder
 *
 * Author: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
NfsFinderFreeRepository(
    IN PNFS_FINDER_REPOSITORY pFinderRepository
    );

static
int
NfsFinderCompareSearchSpaces(
    IN PVOID pKey1,
    IN PVOID pKey2
    );

static
VOID
NfsFinderFreeData(
    IN PVOID pData
    );

static
VOID
NfsFinderFreeSearchSpace(
    IN PNFS_SEARCH_SPACE pSearchSpace
    );

NTSTATUS
NfsFinderCreateRepository(
    OUT PHANDLE phFinderRepository
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_FINDER_REPOSITORY pFinderRepository = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_FINDER_REPOSITORY),
                    (PVOID*)&pFinderRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    pFinderRepository->refCount = 1;

    pthread_mutex_init(&pFinderRepository->mutex, NULL);
    pFinderRepository->pMutex = &pFinderRepository->mutex;

    ntStatus = LwRtlRBTreeCreate(
                    &NfsFinderCompareSearchSpaces,
                    NULL,
                    &NfsFinderFreeData,
                    &pFinderRepository->pSearchSpaceCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    *phFinderRepository = pFinderRepository;

cleanup:

    return ntStatus;

error:

    *phFinderRepository = NULL;

    if (pFinderRepository)
    {
        NfsFinderFreeRepository(pFinderRepository);
    }

    goto cleanup;
}

NTSTATUS
NfsFinderBuildSearchPath(
    IN              PWSTR    pwszPath,
    IN              PWSTR    pwszSearchPattern,
       OUT          PWSTR*   ppwszFilesystemPath,
       OUT          PWSTR*   ppwszSearchPattern,
    IN OUT OPTIONAL PBOOLEAN pbPathHasWildCards
    )
{
    NTSTATUS ntStatus = 0;
    wchar16_t wszStar[]         = {'*',  0};
    wchar16_t wszBackslash[]    = {'\\', 0};
    wchar16_t wszQuestionMark[] = {'?',  0};
    wchar16_t wszQuote[]        = {'\"', 0};
    wchar16_t wszGT[]           = {'>',  0};
    wchar16_t wszLT[]           = {'<',  0};
    wchar16_t wszDot[]          = {'.',  0};
    size_t    sLen = 0;
    PWSTR     pwszCursor = NULL;
    PWSTR     pwszLastSlash = NULL;
    PWSTR     pwszFilesystemPath = NULL;
    PWSTR     pwszSearchPattern3 = NULL;
    PWSTR     pwszSearchPattern2 = NULL;
    BOOLEAN   bPathHasWildCards  = FALSE;

    if (!pwszPath || !*pwszPath)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    sLen = wc16slen(pwszPath);

    while (pwszSearchPattern && *pwszSearchPattern &&
           (*pwszSearchPattern == wszBackslash[0]))
    {
          pwszSearchPattern++;
    }

    if (pwszSearchPattern && *pwszSearchPattern)
    {
        ntStatus = NfsAllocateStringW(pwszSearchPattern, &pwszSearchPattern3);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszCursor = pwszSearchPattern3;
    while (pwszCursor && *pwszCursor)
    {
        if (*pwszCursor == wszGT[0])
        {
            bPathHasWildCards = TRUE;
            *pwszCursor = wszQuestionMark[0];
        }
        else if (*pwszCursor == wszQuote[0])
        {
            PWSTR pwszNext = pwszCursor;

            pwszNext++;

            if (!pwszNext ||
                ((*pwszNext == wszQuestionMark[0] ||
                  *pwszNext == wszStar[0] ||
                  *pwszNext == wszGT[0] ||
                  *pwszNext == wszLT[0] ||
                  *pwszNext == wszQuote[0])))
            {
                bPathHasWildCards = TRUE;
                *pwszCursor = wszDot[0];
            }
        }
        else if (*pwszCursor == wszLT[0])
        {
            PWSTR pwszNext = pwszCursor;

            bPathHasWildCards = TRUE;

            pwszNext++;

            if (pwszNext ||
                (((*pwszNext == wszDot[0]) ||
                 (*pwszNext == wszQuote[0]) ||
                 (*pwszNext == wszLT[0]))))
            {
                *pwszCursor = wszStar[0];
            }
        }

        pwszCursor++;
    }

    pwszCursor = pwszSearchPattern3;

    while (pwszCursor && *pwszCursor)
    {
        if (*pwszCursor == wszBackslash[0])
        {
            pwszLastSlash = pwszCursor;
        }
        else if ((*pwszCursor == wszStar[0]) ||
                 (*pwszCursor == wszQuestionMark[0]))
        {
            bPathHasWildCards = TRUE;

            break;
        }

        pwszCursor++;
    }

    if (pwszLastSlash)
    {
        PBYTE pDataCursor = NULL;
        size_t sSuffixLen = 0;

        sSuffixLen = ((PBYTE)pwszLastSlash - (PBYTE)pwszSearchPattern3);

        ntStatus = NfsAllocateMemory(
                        sLen * sizeof(wchar16_t) + sizeof(wszBackslash[0]) + sSuffixLen + sizeof(wchar16_t),
                        (PVOID*)&pwszFilesystemPath);
        BAIL_ON_NT_STATUS(ntStatus);

        pDataCursor = (PBYTE)pwszFilesystemPath;
        memcpy(pDataCursor, (PBYTE)pwszPath, sLen * sizeof(wchar16_t));
        pDataCursor += sLen * sizeof(wchar16_t);

        *((wchar16_t*)pDataCursor) = wszBackslash[0];
        pDataCursor += sizeof(wszBackslash[0]);

        memcpy(pDataCursor, pwszSearchPattern, sSuffixLen);
    }
    else
    {
        ntStatus = NfsAllocateStringW(pwszPath, &pwszFilesystemPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszCursor = (pwszLastSlash ? ++pwszLastSlash : pwszSearchPattern3);
    if (pwszCursor && *pwszCursor)
    {
        ntStatus = NfsAllocateStringW(pwszCursor, &pwszSearchPattern2);
    }
    else
    {
        ntStatus = NfsAllocateStringW(wszStar, &pwszSearchPattern2);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszFilesystemPath = pwszFilesystemPath;
    *ppwszSearchPattern  = pwszSearchPattern2;

    if (pbPathHasWildCards)
    {
        *pbPathHasWildCards  = bPathHasWildCards;
    }

cleanup:

    NFS_SAFE_FREE_MEMORY(pwszSearchPattern3);

    return ntStatus;

error:

    *ppwszFilesystemPath = NULL;
    *ppwszSearchPattern  = NULL;

    if (pbPathHasWildCards)
    {
        *pbPathHasWildCards  = FALSE;
    }

    NFS_SAFE_FREE_MEMORY(pwszFilesystemPath);
    NFS_SAFE_FREE_MEMORY(pwszSearchPattern2);

    goto cleanup;
}


NTSTATUS
NfsFinderCreateSearchSpace(
    IN  PNFS_SHARE_INFO pShareInfo,
    IN  PIO_CREATE_SECURITY_CONTEXT pIoSecurityContext,
    IN  HANDLE         hFinderRepository,
    IN  PWSTR          pwszFilesystemPath,
    IN  PWSTR          pwszSearchPattern,
    IN  USHORT         usSearchAttrs,
    IN  ULONG          ulSearchStorageType,
    IN  SMB_INFO_LEVEL infoLevel,
    IN  BOOLEAN        bUseLongFilenames,
    IN  ACCESS_MASK    accessMask,
    OUT PHANDLE        phFinder,
    OUT PUSHORT        pusSearchId
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE      hFile = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    IO_FILE_NAME        fileName = {0};
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    PNFS_FINDER_REPOSITORY pFinderRepository = NULL;
    PNFS_SEARCH_SPACE pSearchSpace = NULL;
    USHORT   usCandidateSearchId = 0;
    BOOLEAN  bFound = FALSE;
    BOOLEAN  bInLock = FALSE;
    PIO_ECP_LIST pEcpList = NULL;

    pFinderRepository = (PNFS_FINDER_REPOSITORY)hFinderRepository;

    fileName.FileName = pwszFilesystemPath;

    ntStatus = NfsIoCreateFile(
                    pShareInfo,
                    &hFile,
                    NULL,
                    &ioStatusBlock,
                    pIoSecurityContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    accessMask,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                    FILE_OPEN,
                    0,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    &pEcpList
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    usCandidateSearchId = pFinderRepository->usNextSearchId;

    do
    {
        PNFS_SEARCH_SPACE pSearchSpace = NULL;

        if (!usCandidateSearchId || (usCandidateSearchId == UINT16_MAX))
        {
            usCandidateSearchId++;
        }

        ntStatus = LwRtlRBTreeFind(
                        pFinderRepository->pSearchSpaceCollection,
                        &usCandidateSearchId,
                        (PVOID*)&pSearchSpace);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = 0;
            bFound = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((usCandidateSearchId != pFinderRepository->usNextSearchId) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_OPENED_FILES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_SEARCH_SPACE),
                    (PVOID*)&pSearchSpace);
    BAIL_ON_NT_STATUS(ntStatus);

    pSearchSpace->refCount = 1;

    pthread_mutex_init(&pSearchSpace->mutex, NULL);
    pSearchSpace->pMutex = &pSearchSpace->mutex;

    pSearchSpace->usSearchId = usCandidateSearchId;

    ntStatus = LwRtlRBTreeAdd(
                    pFinderRepository->pSearchSpaceCollection,
                    &pSearchSpace->usSearchId,
                    pSearchSpace);
    BAIL_ON_NT_STATUS(ntStatus);

    pSearchSpace->hFile = hFile;
    hFile = NULL;
    pSearchSpace->infoLevel = infoLevel;
    pSearchSpace->usSearchAttrs = usSearchAttrs;
    pSearchSpace->ulSearchStorageType = ulSearchStorageType;
    pSearchSpace->bUseLongFilenames = bUseLongFilenames;

    ntStatus = NfsAllocateStringW(
                    pwszSearchPattern,
                    &pSearchSpace->pwszSearchPattern);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSearchSpace->refCount);

    pFinderRepository->usNextSearchId = usCandidateSearchId + 1;

    *phFinder = pSearchSpace;
    *pusSearchId = usCandidateSearchId;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    if (pEcpList)
    {
        IoRtlEcpListFree(&pEcpList);
    }

    return ntStatus;

error:

    *phFinder = NULL;
    *pusSearchId = 0;

    if (pSearchSpace)
    {
        NfsFinderReleaseSearchSpace(pSearchSpace);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    goto cleanup;
}

NTSTATUS
NfsFinderGetSearchSpace(
    IN  HANDLE  hFinderRepository,
    IN  USHORT  usSearchId,
    OUT PHANDLE phFinder
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_FINDER_REPOSITORY pFinderRepository = NULL;
    PNFS_SEARCH_SPACE pSearchSpace = NULL;
    BOOLEAN bInLock = FALSE;

    pFinderRepository = (PNFS_FINDER_REPOSITORY)hFinderRepository;

    LWIO_LOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pFinderRepository->pSearchSpaceCollection,
                    &usSearchId,
                    (PVOID*)&pSearchSpace);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSearchSpace->refCount);

    *phFinder = pSearchSpace;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    return ntStatus;

error:

    *phFinder = NULL;

    goto cleanup;
}

VOID
NfsFinderReleaseSearchSpace(
    IN HANDLE hFinder
    )
{
    PNFS_SEARCH_SPACE pSearchSpace = (PNFS_SEARCH_SPACE)hFinder;

    if (InterlockedDecrement(&pSearchSpace->refCount) == 0)
    {
        NfsFinderFreeSearchSpace(pSearchSpace);
    }
}

NTSTATUS
NfsFinderCloseSearchSpace(
    IN HANDLE hFinderRepository,
    IN USHORT usSearchId
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_FINDER_REPOSITORY pFinderRepository = NULL;
    BOOLEAN bInLock = FALSE;

    pFinderRepository = (PNFS_FINDER_REPOSITORY)hFinderRepository;

    LWIO_LOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pFinderRepository->pSearchSpaceCollection,
                    &usSearchId);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pFinderRepository->mutex);

    return ntStatus;

error:

    goto cleanup;
}

VOID
NfsFinderCloseRepository(
    IN HANDLE hFinderRepository
    )
{
    PNFS_FINDER_REPOSITORY pFinderRepository = NULL;

    pFinderRepository = (PNFS_FINDER_REPOSITORY)hFinderRepository;

    if (InterlockedDecrement(&pFinderRepository->refCount) == 0)
    {
        NfsFinderFreeRepository(pFinderRepository);
    }
}

static
VOID
NfsFinderFreeRepository(
    IN PNFS_FINDER_REPOSITORY pFinderRepository
    )
{
    if (pFinderRepository->pSearchSpaceCollection)
    {
        LwRtlRBTreeFree(pFinderRepository->pSearchSpaceCollection);
    }

    if (pFinderRepository->pMutex)
    {
        pthread_mutex_destroy(&pFinderRepository->mutex);
    }

    NfsFreeMemory(pFinderRepository);
}

static
int
NfsFinderCompareSearchSpaces(
    IN PVOID pKey1,
    IN PVOID pKey2
    )
{
    USHORT usKey1 = *((PUSHORT)pKey1);
    USHORT usKey2 = *((PUSHORT)pKey2);

    if (usKey1 > usKey2)
    {
        return 1;
    }
    else if (usKey1 < usKey2)
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
NfsFinderFreeData(
    IN PVOID pData
    )
{
    PNFS_SEARCH_SPACE pSearchSpace = (PNFS_SEARCH_SPACE)pData;

    if (InterlockedDecrement(&pSearchSpace->refCount) == 0)
    {
        NfsFinderFreeSearchSpace(pSearchSpace);
    }
}

static
VOID
NfsFinderFreeSearchSpace(
    IN PNFS_SEARCH_SPACE pSearchSpace
    )
{
    if (pSearchSpace->pMutex)
    {
        pthread_mutex_destroy(&pSearchSpace->mutex);
    }

    if (pSearchSpace->hFile)
    {
        IoCloseFile(pSearchSpace->hFile);
    }

    if (pSearchSpace->pFileInfo)
    {
        NfsFreeMemory(pSearchSpace->pFileInfo);
    }

    if (pSearchSpace->pwszSearchPattern)
    {
        SMBFreeMemory(pSearchSpace->pwszSearchPattern);

    }

    NfsFreeMemory(pSearchSpace);
}
