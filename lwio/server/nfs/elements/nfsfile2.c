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
 *        nfsfile2.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        File Object (Version 2)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
VOID
NfsFile2Free(
    PLWIO_NFS_FILE_2 pFile
    );

NTSTATUS
NfsFile2Create(
    PSMB2_FID               pFid,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_NFS_FILE_2*       ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_FILE_2 pFile = NULL;

    LWIO_LOG_DEBUG("Creating file [fid: (persistent:%08X)(volatile:%08X)]",
                   pFid->ullPersistentId,
                   pFid->ullVolatileId);

    ntStatus = NfsAllocateMemory(
                    sizeof(LWIO_NFS_FILE_2),
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->refcount = 1;

    pthread_rwlock_init(&pFile->mutex, NULL);
    pFile->pMutex = &pFile->mutex;

    ntStatus = NfsAllocateStringW(pwszFilename, &pFile->pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->fid = *pFid;
    pFile->hFile = *phFile;
    *phFile = NULL;
    pFile->pFilename = *ppFilename;
    *ppFilename = NULL;
    pFile->desiredAccess = desiredAccess;
    pFile->allocationSize = allocationSize;
    pFile->fileAttributes = fileAttributes;
    pFile->shareAccess = shareAccess;
    pFile->createDisposition = createDisposition;
    pFile->createOptions = createOptions;

    LWIO_LOG_DEBUG( "Associating file [object:0x%x]"
                    "[fid: (persistent:%08X)(volatile:%08X)]",
                    pFile,
                    pFid->ullPersistentId,
                    pFid->ullVolatileId);

    NFS_ELEMENTS_INCREMENT_OPEN_FILES;

    *ppFile = pFile;

cleanup:

    return ntStatus;

error:

    *ppFile = NULL;

    if (pFile)
    {
        NfsFile2Release(pFile);
    }

    goto cleanup;
}

NTSTATUS
NfsFile2SetOplockState(
    PLWIO_NFS_FILE_2               pFile,
    HANDLE                         hOplockState,
    PFN_LWIO_NFS_FREE_OPLOCK_STATE pfnFreeOplockState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    if (pFile->hOplockState && pFile->pfnFreeOplockState)
    {
        pFile->pfnFreeOplockState(pFile->hOplockState);
        pFile->hOplockState       = NULL;
        pFile->pfnFreeOplockState = NULL;
    }

    pFile->hOplockState       = hOplockState;
    pFile->pfnFreeOplockState = pfnFreeOplockState;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;
}

HANDLE
NfsFile2RemoveOplockState(
    PLWIO_NFS_FILE_2 pFile
    )
{
    HANDLE  hOplockState = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    hOplockState = pFile->hOplockState;

    pFile->hOplockState       = NULL;
    pFile->pfnFreeOplockState = NULL;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return hOplockState;
}

VOID
NfsFile2ResetOplockState(
    PLWIO_NFS_FILE_2 pFile
    )
{
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    if (pFile->hOplockState && pFile->pfnFreeOplockState)
    {
        pFile->pfnFreeOplockState(pFile->hOplockState);
        pFile->hOplockState       = NULL;
        pFile->pfnFreeOplockState = NULL;
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

VOID
NfsFile2SetOplockLevel(
    PLWIO_NFS_FILE_2 pFile,
    UCHAR            ucOplockLevel
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pFile->ucCurrentOplockLevel = ucOplockLevel;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

UCHAR
NfsFile2GetOplockLevel(
    PLWIO_NFS_FILE_2 pFile
    )
{
    UCHAR ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pFile->mutex);

    ucOplockLevel = pFile->ucCurrentOplockLevel;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ucOplockLevel;
}

PLWIO_NFS_FILE_2
NfsFile2Acquire(
    PLWIO_NFS_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG("Acquiring file [fid: (persistent:%08X)(volatile:%08X)]",
                    pFile->fid.ullPersistentId,
                    pFile->fid.ullVolatileId);

    InterlockedIncrement(&pFile->refcount);

    return pFile;
}

VOID
NfsFile2Release(
    PLWIO_NFS_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG("Releasing file [fid: (persistent:%08X)(volatile:%08X)]",
                    pFile->fid.ullPersistentId,
                    pFile->fid.ullVolatileId);

    if (InterlockedDecrement(&pFile->refcount) == 0)
    {
        NFS_ELEMENTS_DECREMENT_OPEN_FILES;

        NfsFile2Free(pFile);
    }
}

VOID
NfsFile2Rundown(
    PLWIO_NFS_FILE_2 pFile
    )
{
    if (pFile->hFile)
    {
        IoCancelFile(pFile->hFile);
    }
}

static
VOID
NfsFile2Free(
    PLWIO_NFS_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG( "Freeing file [object:0x%x]"
                    "[fid: (persistent:%08X)(volatile:%08X)]",
                    pFile,
                    pFile->fid.ullPersistentId,
                    pFile->fid.ullVolatileId);

    if (pFile->pMutex)
    {
        pthread_rwlock_destroy(&pFile->mutex);
        pFile->pMutex = NULL;
    }

    if (pFile->pFilename)
    {
        if (pFile->pFilename->FileName)
        {
            NfsFreeMemory (pFile->pFilename->FileName);
        }

        NfsFreeMemory(pFile->pFilename);
    }

    if (pFile->hFile)
    {
        IoCloseFile(pFile->hFile);
    }

    if (pFile->pwszFilename)
    {
        NfsFreeMemory(pFile->pwszFilename);
    }

    if (pFile->searchSpace.pwszSearchPattern)
    {
        NfsFreeMemory(pFile->searchSpace.pwszSearchPattern);
    }

    if (pFile->searchSpace.pwszSearchPatternRaw)
    {
        NfsFreeMemory(pFile->searchSpace.pwszSearchPatternRaw);
    }

    if (pFile->searchSpace.pFileInfo)
    {
        NfsFreeMemory(pFile->searchSpace.pFileInfo);
    }

    NfsFreeMemory(pFile);
}
