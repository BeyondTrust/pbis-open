/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"
#include "handlemgr_p.h"

static
size_t
SMBHandleManagerHashKey(
    PCVOID pHandleId
    );

static
VOID
SMBHandleManagerFreeKey(
    const SMB_HASH_ENTRY *pEntry
    );

DWORD
SMBHandleManagerCreate(
    IN  SMBHANDLE dwHandleMax,
    OUT PSMB_HANDLE_MANAGER* ppHandleManager
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_MANAGER pHandleManager = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_HANDLE_MANAGER),
                    (PVOID*)&pHandleManager);
    BAIL_ON_LWIO_ERROR(dwError);

    pHandleManager->dwHandleMax = dwHandleMax ? dwHandleMax : SMB_DEFAULT_HANDLE_MAX;

    dwError = SMBBitVectorCreate(
                    pHandleManager->dwHandleMax,
                    &pHandleManager->pFreeHandleIndex);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBHashCreate(
                    pHandleManager->dwHandleMax % 421,
                    &SMBHashCompareUINT32,
                    &SMBHandleManagerHashKey,
                    &SMBHandleManagerFreeKey,
                    &pHandleManager->pHandleTable);
    BAIL_ON_LWIO_ERROR(dwError);

    *ppHandleManager = pHandleManager;

cleanup:

    return dwError;

error:

    *ppHandleManager = NULL;

    if (pHandleManager)
    {
        SMBHandleManagerFree(pHandleManager);
    }

    goto cleanup;
}

DWORD
SMBHandleManagerAddItem(
    IN  PSMB_HANDLE_MANAGER pHandleMgr,
    IN  SMBHandleType       hType,
    IN  PVOID               pItem,
    OUT PSMBHANDLE          phItem
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_TABLE_ENTRY pEntry = NULL;
    PDWORD pKey = NULL;
    DWORD dwAvailableIndex = 0;

    BAIL_ON_INVALID_POINTER(pHandleMgr);
    BAIL_ON_INVALID_POINTER(pItem);

    dwError = SMBBitVectorFirstUnsetBit(
                pHandleMgr->pFreeHandleIndex,
                &dwAvailableIndex);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBAllocateMemory(
                    sizeof(SMB_HANDLE_TABLE_ENTRY),
                    (PVOID*)&pEntry);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBAllocateMemory(
                    sizeof(DWORD),
                    (PVOID*)&pKey);
    BAIL_ON_LWIO_ERROR(dwError);

    pEntry->hType = hType;
    pEntry->pItem = pItem;

    *pKey = dwAvailableIndex;

    dwError = SMBHashSetValue(
                    pHandleMgr->pHandleTable,
                    pKey,
                    pEntry);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBBitVectorSetBit(
                    pHandleMgr->pFreeHandleIndex,
                    dwAvailableIndex);
    BAIL_ON_LWIO_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (dwError == LWIO_ERROR_NO_BIT_AVAILABLE)
    {
        dwError = LWIO_ERROR_OUT_OF_HANDLES;
    }

    LWIO_SAFE_FREE_MEMORY(pEntry);
    LWIO_SAFE_FREE_MEMORY(pKey);

    goto cleanup;
}

DWORD
SMBHandleManagerGetItem(
    IN  PSMB_HANDLE_MANAGER pHandleMgr,
    IN  SMBHANDLE           hItem,
    OUT SMBHandleType*      phType,
    OUT PVOID*              ppItem
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_TABLE_ENTRY pEntry = NULL;

    BAIL_ON_INVALID_POINTER(pHandleMgr);
    BAIL_ON_INVALID_POINTER(ppItem);
    BAIL_ON_INVALID_POINTER(phType);
    BAIL_ON_INVALID_SMBHANDLE(hItem);

    // Make sure this handle is currently assigned
    if (!SMBBitVectorIsSet(
                pHandleMgr->pFreeHandleIndex,
                hItem))
    {
        dwError = LWIO_ERROR_INVALID_HANDLE;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = SMBHashGetValue(
                pHandleMgr->pHandleTable,
                &hItem,
                (PVOID*)&pEntry);
    BAIL_ON_LWIO_ERROR(dwError);

    *phType = pEntry->hType;
    *ppItem = pEntry->pItem;

cleanup:

    return dwError;

error:

    if (ppItem)
    {
        *ppItem = NULL;
    }

    if (phType)
    {
        *phType = SMB_HANDLE_TYPE_UNKNOWN;
    }

    if (dwError == ENOENT)
    {
        dwError = LWIO_ERROR_INVALID_HANDLE;
    }

    goto cleanup;
}

DWORD
SMBHandleManagerDeleteItem(
    IN  PSMB_HANDLE_MANAGER     pHandleMgr,
    IN  SMBHANDLE               hItem,
    OUT OPTIONAL SMBHandleType* phType,
    OUT OPTIONAL PVOID*         ppItem
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_TABLE_ENTRY pEntry = NULL;
    PVOID pItem = NULL;
    SMBHandleType hType = SMB_HANDLE_TYPE_UNKNOWN;

    BAIL_ON_INVALID_POINTER(pHandleMgr);
    BAIL_ON_INVALID_SMBHANDLE(hItem);

    // Make sure this handle is currently assigned
    if (!SMBBitVectorIsSet(
                pHandleMgr->pFreeHandleIndex,
                hItem))
    {
        dwError = LWIO_ERROR_INVALID_HANDLE;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = SMBHashGetValue(
                pHandleMgr->pHandleTable,
                &hItem,
                (PVOID*)&pEntry);
    BAIL_ON_LWIO_ERROR(dwError);

    hType = pEntry->hType;
    pItem = pEntry->pItem;

    dwError = SMBHashRemoveKey(
                    pHandleMgr->pHandleTable,
                    &hItem);
    BAIL_ON_LWIO_ERROR(dwError);

    // Indicate that this id is available
    dwError = SMBBitVectorUnsetBit(
                pHandleMgr->pFreeHandleIndex,
                hItem);
    BAIL_ON_LWIO_ERROR(dwError);

    if (ppItem)
    {
        *ppItem = pItem;
    }

    if (phType)
    {
        *phType = hType;
    }

cleanup:

    return dwError;

error:

    if (ppItem)
    {
        *ppItem = NULL;
    }

    if (phType)
    {
        *phType = SMB_HANDLE_TYPE_UNKNOWN;
    }

    if (dwError == ENOENT)
    {
    dwError = LWIO_ERROR_INVALID_HANDLE;
    }

    goto cleanup;
}

VOID
SMBHandleManagerFree(
    IN PSMB_HANDLE_MANAGER pHandleManager
    )
{
    SMBHashSafeFree(&pHandleManager->pHandleTable);

    if (pHandleManager->pFreeHandleIndex)
    {
        SMBBitVectorFree(pHandleManager->pFreeHandleIndex);
    }

    SMBFreeMemory(pHandleManager);
}

static
size_t
SMBHandleManagerHashKey(
    PCVOID pHandleId
    )
{
    return (pHandleId ? *(uint32_t*)pHandleId : 0);
}

static
VOID
SMBHandleManagerFreeKey(
    const SMB_HASH_ENTRY *pEntry
    )
{
    PVOID pHandleTableEntry = (PVOID)pEntry->pKey;
    LWIO_SAFE_FREE_MEMORY(pHandleTableEntry);
}

