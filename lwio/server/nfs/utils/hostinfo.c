/*
 * Copyright Likewise Software    2004-2009
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
 *        hostinfo.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - NFS
 *
 *        Utilities
 *
 *        Producer Consumer Queue
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
NfsFreeHostInfo(
    PNFS_HOST_INFO pHostinfo
    );

NTSTATUS
NfsAcquireHostInfo(
    PNFS_HOST_INFO  pOrigHostInfo,
    PNFS_HOST_INFO* ppNewHostInfo
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PSTR pszDomain = NULL;
    PNFS_HOST_INFO pHostInfo = NULL;
    PNFS_HOST_INFO pNewHostInfo = NULL;

    if (!pOrigHostInfo)
    {
        ntStatus = NfsAllocateMemory(
                        sizeof(NFS_HOST_INFO),
                        (PVOID*)&pHostInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pHostInfo->refcount = 1;

        pthread_rwlock_init(&pHostInfo->mutex, NULL);
        pHostInfo->pMutex = &pHostInfo->mutex;

        dwError = LWNetGetCurrentDomain(&pszDomain);
        switch (dwError)
        {
        case ERROR_SUCCESS:
            pHostInfo->bIsJoined = TRUE;
            ntStatus = SMBAllocateString(pszDomain, &pHostInfo->pszDomain);
            break;
        case ERROR_NOT_JOINED:
            pHostInfo->bIsJoined = FALSE;
            ntStatus = SMBAllocateString("", &pHostInfo->pszDomain);
            break;
        default:
            // Need proper WinError -> NTSTATUS mapping here
            ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwioGetHostInfo(&pHostInfo->pszHostname);
        BAIL_ON_NT_STATUS(ntStatus);

        pNewHostInfo = pHostInfo;
    }
    else
    {
        pNewHostInfo = pOrigHostInfo;
        InterlockedIncrement(&pNewHostInfo->refcount);
    }

    *ppNewHostInfo = pNewHostInfo;

cleanup:

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
    }

    return ntStatus;

error:

    *ppNewHostInfo = NULL;

    if (pHostInfo)
    {
        NfsFreeHostInfo(pHostInfo);
    }

    goto cleanup;
}

VOID
NfsReleaseHostInfo(
    PNFS_HOST_INFO pHostinfo
    )
{
    if (InterlockedDecrement(&pHostinfo->refcount) == 0)
    {
        NfsFreeHostInfo(pHostinfo);
    }
}

static
VOID
NfsFreeHostInfo(
    PNFS_HOST_INFO pHostinfo
    )
{
    if (pHostinfo->pMutex)
    {
        pthread_rwlock_destroy(&pHostinfo->mutex);
        pHostinfo->pMutex = NULL;
    }
    LWIO_SAFE_FREE_STRING(pHostinfo->pszHostname);
    LWIO_SAFE_FREE_STRING(pHostinfo->pszDomain);

    NfsFreeMemory(pHostinfo);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
