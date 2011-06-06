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
 *        memapi.c
 *
 * Abstract:
 *        Plugin API for registry memory provider backend
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#include "includes.h"

NTSTATUS
_MemCreateHkeyReply(
    IN PMEMREG_NODE pSubKey,
    OUT PHKEY phkResult)
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE phKeyResponse = NULL;
    PREG_KEY_CONTEXT pRetKey = NULL;
    PREG_KEY_HANDLE *phKeyResult = (PREG_KEY_HANDLE *)phkResult;

    status = LW_RTL_ALLOCATE(
                 (PVOID*)&phKeyResponse,
                 PREG_KEY_HANDLE,
                 sizeof(*phKeyResponse));
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE(
                 (PVOID*)&pRetKey,
                 PREG_KEY_CONTEXT,
                 sizeof(*pRetKey));
    BAIL_ON_NT_STATUS(status);

    pRetKey->hNode = pSubKey;
    phKeyResponse->pKey = pRetKey;
    *phKeyResult = phKeyResponse;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pRetKey);
    LWREG_SAFE_FREE_MEMORY(phKeyResponse);
    goto cleanup;
}

NTSTATUS
MemProvider_Initialize(
    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable,
    const PWSTR* ppwszRootKeyNames
    )
{
    NTSTATUS status = 0;
    MEMDB_IMPORT_FILE_CTX importCtx = {0};
    PREG_DB_CONNECTION pConn = NULL;
    PMEMREG_NODE pDbRoot = NULL;

    setlocale(LC_ALL, "");
    status = LW_RTL_ALLOCATE(
                 (PVOID*)&pConn,
                 REG_DB_CONNECTION,
                 sizeof(*pConn));
    BAIL_ON_NT_STATUS(status);
    memset(pConn, 0, sizeof(*pConn));

    /* This may not exist after install. If exists don't care this fails */
    mkdir(MEMDB_EXPORT_DIR, 0700);

    status = MemDbOpen(&pDbRoot);
    BAIL_ON_NT_STATUS(status);

    /* 
     * This is the pointer that holds the memory database
     */
    pConn->pMemReg = pDbRoot;
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_rwlock_init(&pConn->lock, NULL)));
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_mutex_init(&pConn->ExportMutex, NULL)));
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_mutex_init(&pConn->ExportMutexStop, NULL)));
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_cond_init(&pConn->ExportCond, NULL)));
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_cond_init(&pConn->ExportCondStop, NULL)));

    /* Must initialize database root here; used by following import/export */
    MemRegRootInit(pConn);

    /* Initialize memory registry from data previously saved to file */
    importCtx.fileName = MEMDB_EXPORT_FILE;
    status = MemDbImportFromFile(
                 MEMDB_EXPORT_FILE,
                 pfImportFile,
                 &importCtx);
    BAIL_ON_NT_STATUS(status);

    /*
     * Start export to save file thread
     */
    status = MemDbStartExportToFileThread();
    BAIL_ON_NT_STATUS(status);

    *ppFnTable = &gRegMemProviderAPITable;
cleanup:
    if (status == LWREG_ERROR_PARSE || status == LWREG_ERROR_SYNTAX)
    {
        status = LW_STATUS_FILE_CORRUPT_ERROR;
    }
    return status;

error:
    MemDbClose(pConn);
    MemRegRootInit(NULL);
    goto cleanup;
}


/*
 * Function to determine if any node in a subtree has a non-zero refcount.
 */
void *
pfMemRegSubTreeRefCount(
    PMEMREG_NODE pEntry, 
    PVOID userContext,
    PWSTR subStringPrefix,
    NTSTATUS *pStatus)
{
    if (pEntry && pStatus && pEntry->NodeRefCount >= 1)
    {
        *pStatus = STATUS_RESOURCE_IN_USE;
    }
    return NULL;
}


static void *pfDeleteNodeCallback(
    PMEMREG_NODE pEntry, 
    PVOID userContext,
    PWSTR subStringPrefix,
    NTSTATUS *pStatus)
{
    /* Check current node ref count, and bail if it isn't 0 */
    if (pEntry->NodeRefCount >= 1)
    {
        *pStatus = STATUS_RESOURCE_IN_USE;
        BAIL_ON_REG_ERROR(*pStatus);
    }
    *pStatus = MemRegStoreDeleteNode(pEntry);

cleanup:
    return NULL;
error:
    goto cleanup;
}


VOID
MemProvider_Shutdown(
    PREGPROV_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    REG_DB_CONNECTION regDbConn = {0};
    NTSTATUS status = 0;
    PWSTR pwszRootKey = NULL;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pMemRegRoot = MemRegRoot();
    MEMDB_FILE_EXPORT_CTX exportCtx = {0};

    exportCtx.hNode = pMemRegRoot->pMemReg;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pMemRegRoot->lock);
    status = MemDbExportToFile(&exportCtx);
    BAIL_ON_REG_ERROR(status);

    regDbConn.pMemReg = pMemRegRoot->pMemReg;
    status = MemDbClose(&regDbConn);
    BAIL_ON_REG_ERROR(status);

    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_mutex_destroy(&pMemRegRoot->ExportMutex)));
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_mutex_destroy(&pMemRegRoot->ExportMutexStop)));
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_cond_destroy(&pMemRegRoot->ExportCond)));
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_cond_destroy(&pMemRegRoot->ExportCondStop)));

    LWREG_UNLOCK_RWMUTEX(bInLock, &pMemRegRoot->lock);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
        pthread_rwlock_destroy(&pMemRegRoot->lock)));

cleanup:
    LWREG_SAFE_FREE_MEMORY(pMemRegRoot->ExportCtx);
    LWREG_SAFE_FREE_MEMORY(pMemRegRoot);
    LWREG_SAFE_FREE_MEMORY(pwszRootKey);
    return;
  
error:
    goto cleanup;
}


NTSTATUS
MemCreateKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD dwReserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PMEMREG_NODE hRootKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};
    PWSTR pwszRootKey = NULL;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;
    ACCESS_MASK AccessGranted = 0;
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor = NULL;
    DWORD SecurityDescriptorLen = 0;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &MemRegRoot()->lock);
    if (!hKey)
    {
        status = LwRtlWC16StringAllocateFromCString(
                     &pwszRootKey,
                     HKEY_THIS_MACHINE);
        BAIL_ON_NT_STATUS(status);

        // Search for specified root key. If NULL, return HKTM.
        status = MemDbOpenKey(
                     Handle,
                     NULL,
                     pwszRootKey,
                     AccessDesired,
                     &hRootKey);
        BAIL_ON_NT_STATUS(status);
        regDbConn.pMemReg = hRootKey;
    }
    else
    {
        regDbConn.pMemReg = pKeyHandle->pKey->hNode;
    }

    if (!pServerState->pToken)
    {
        status = RegSrvCreateAccessToken(pServerState->peerUID,
                                         pServerState->peerGID,
                                         &pServerState->pToken);
        BAIL_ON_NT_STATUS(status);
    }

    if (pSecDescRel)
    {
        SecurityDescriptor = pSecDescRel;
        SecurityDescriptorLen = ulSecDescLength;
    }
    else
    {
        SecurityDescriptor =
            pKeyHandle->pKey->hNode->pNodeSd->SecurityDescriptor;
        SecurityDescriptorLen =
            pKeyHandle->pKey->hNode->pNodeSd->SecurityDescriptorLen;
    }

    if (SecurityDescriptor)
    {
        status = RegSrvAccessCheckKey(pServerState->pToken,
                                      SecurityDescriptor,
                                      SecurityDescriptorLen,
                                      AccessDesired,
                                      &AccessGranted);
    }

    if (STATUS_NO_TOKEN == status)
    {
        status = 0;
        AccessGranted = 0;
    }
    BAIL_ON_NT_STATUS(status);

    status = MemDbCreateKeyEx(
                 Handle,  // Access token is on this handle
                 &regDbConn,
                 pSubKey,
                 0, // IN DWORD dwReserved
                 NULL, // IN OPTIONAL PWSTR pClass
                 0, //IN DWORD dwOptions
                 AccessDesired, // IN ACCESS_MASK 
                 pSecDescRel, // IN OPTIONAL 
                 ulSecDescLength, // IN ULONG
                 &hSubKey,
                 pdwDisposition);
    BAIL_ON_NT_STATUS(status);

    status = _MemCreateHkeyReply(hSubKey, phkResult);
    BAIL_ON_NT_STATUS(status);

    pKeyHandle->AccessGranted = AccessGranted;
    hSubKey->NodeRefCount++;

    MemDbExportEntryChanged();

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    LWREG_SAFE_FREE_MEMORY(pwszRootKey);
    return status;

error:
    if (hRootKey)
    {
        // Close open root key
    }
    goto cleanup;
}


NTSTATUS
MemOpenKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_HANDLE *phKeyResult = (PREG_KEY_HANDLE *)phkResult;
    PMEMREG_NODE pSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;
    BOOLEAN bInLock = FALSE;

    if (!pServerState->pToken)
    {
        status = RegSrvCreateAccessToken(pServerState->peerUID,
                                         pServerState->peerGID,
                                         &pServerState->pToken);
        BAIL_ON_NT_STATUS(status);
    }

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &MemRegRoot()->lock);
    if (!hKey)
    {
        // Search for specified root key. If NULL, return HKTM.
        status = MemDbOpenKey(
                     Handle,
                     NULL,
                     pwszSubKey,
                     AccessDesired,
                     &pSubKey);
        BAIL_ON_NT_STATUS(status);
    }
    else if (pKeyHandle->pKey->hNode)
    {
        regDbConn.pMemReg = pKeyHandle->pKey->hNode;
        status = MemDbOpenKey(
                     Handle,
                     &regDbConn,
                     pwszSubKey,
                     AccessDesired,
                     &pSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    if (phKeyResult)
    {
        status = _MemCreateHkeyReply(
                     pSubKey,
                     phkResult);
        BAIL_ON_NT_STATUS(status);
    }


cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;

error:
    goto cleanup;
}


VOID
MemCloseKey(
    IN HKEY hKey
    )
{
    BOOLEAN bInLock = FALSE;
    
    if (hKey)
    {
        LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &MemRegRoot()->lock);
        MemDbCloseKey(hKey);
        LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    }
}


NTSTATUS
MemDeleteKey(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE) hKey;
    PMEMREG_NODE hParentKey = NULL;
    PMEMREG_NODE hRegKey = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor = NULL;
    DWORD SecurityDescriptorLen = 0;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;
    ACCESS_MASK AccessGranted = 0;
    BOOLEAN bInLock = FALSE;

    if (pKeyHandle && pKeyHandle->pKey->hNode->pNodeSd)
    {
        SecurityDescriptor = 
            pKeyHandle->pKey->hNode->pNodeSd->SecurityDescriptor;
        SecurityDescriptorLen =
            pKeyHandle->pKey->hNode->pNodeSd->SecurityDescriptorLen;
    }

    if (SecurityDescriptor && pServerState && pServerState->pToken)
    {
        status = RegSrvAccessCheckKey(pServerState->pToken,
                                      SecurityDescriptor,
                                      SecurityDescriptorLen,
                                      KEY_WRITE,
                                      &AccessGranted);
    }

    if (STATUS_NO_TOKEN == status)
    {
        status = 0;
        AccessGranted = 0;
    }
    BAIL_ON_NT_STATUS(status);

    if (hKey)
    {
        hParentKey = pKeyHandle->pKey->hNode;
    }
    else
    {
        hParentKey = MemRegRoot()->pMemReg;
    }

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &MemRegRoot()->lock);

    status = MemRegStoreFindNode(
                 hParentKey,
                 pSubKey,
                 &hRegKey);
    BAIL_ON_NT_STATUS(status);

    /* Don't delete this node if it has subkeys */
    if (hRegKey->NodesLen > 0)
    {
        status = STATUS_KEY_HAS_CHILDREN;
        BAIL_ON_NT_STATUS(status);
    }

    /* Don't delete this node if there are still open handles to it */
    if (hRegKey->NodeRefCount >= 1)
    {
        status = STATUS_RESOURCE_IN_USE;
        BAIL_ON_NT_STATUS(status);
    }

    status = MemRegStoreDeleteNode(hRegKey);
    BAIL_ON_NT_STATUS(status);

    MemDbExportEntryChanged();

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;

error:
    goto cleanup;

}


NTSTATUS
MemQueryInfoKey(
    IN HANDLE Handle,
    IN HKEY hKey,
    OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pdwReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    REG_DB_CONNECTION regDbConn = {0};
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_STATUS(status);

    regDbConn.pMemReg = pKeyHandle->pKey->hNode;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &MemRegRoot()->lock);
    status = MemDbQueryInfoKey(
                 Handle,
                 &regDbConn,
    /*
     * A pointer to a buffer that receives the user-defined class of the key. 
     * This parameter can be NULL.
     */
                 NULL, //OUT OPTIONAL PWSTR pClass, 
                 NULL, // IN OUT OPTIONAL PDWORD pcClass,
                 NULL, /* This parameter is reserved and must be NULL. */
                 pcSubKeys,
                 pcMaxSubKeyLen,
                 pcMaxClassLen, /* implement this later */
                 pcValues,
                 pcMaxValueNameLen,
                 pcMaxValueLen,
                 pcbSecurityDescriptor,
                 pftLastWriteTime /* implement this later */
                 );

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemEnumKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pdwReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT PFILETIME pftLastWriteTime
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    REG_DB_CONNECTION regDbConn = {0};
    BOOLEAN bInLock = FALSE;

    regDbConn.pMemReg = pKeyHandle->pKey->hNode;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &MemRegRoot()->lock);
    status = MemDbEnumKeyEx(
                 Handle,
                 &regDbConn,
                 dwIndex,
                 pName,
                 pcName,
                 pdwReserved,
                 pClass,
                 pcClass,
                 pftLastWriteTime
    );
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);

    return status;
}


NTSTATUS
MemSetValueEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
    NTSTATUS status = 0;
    REG_DB_CONNECTION regDbConn = {0};
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE) hKey;
    BOOLEAN bInLock = FALSE;

    if (pValueName && wc16slen(pValueName) > MEMREG_MAX_VALUENAME_LEN)
    {
        status = STATUS_INVALID_BLOCK_LENGTH;
        BAIL_ON_NT_STATUS(status);
    }
    regDbConn.pMemReg = pKeyHandle->pKey->hNode;
    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &MemRegRoot()->lock);
    status = MemDbSetValueEx(
                 Handle,
                 &regDbConn,
                 pValueName,
                 dwReserved,
                 dwType,
                 pData,
                 cbData);
    BAIL_ON_NT_STATUS(status);

    MemDbExportEntryChanged();
cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemGetValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    NTSTATUS status = 0;
    REG_DB_CONNECTION regDbConn = {0};
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE) hKey;
    regDbConn.pMemReg = pKeyHandle->pKey->hNode;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &MemRegRoot()->lock);
    status = MemDbGetValue(
                 Handle,
                 &regDbConn,
                 pSubKey,
                 pValueName,
                 Flags,
                 pdwType,
                 pData,
                 pcbData);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDeleteKeyValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE) hKey;
    PMEMREG_NODE hSubKey = NULL;
    BOOLEAN bInLock = FALSE;

    hSubKey = pKeyHandle->pKey->hNode;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &MemRegRoot()->lock);
    if (pSubKey)
    {
        status = MemRegStoreFindNode(
                     hSubKey,
                     pSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    status = MemRegStoreDeleteNodeValue(
                 hSubKey,
                 pValueName);
    MemDbExportEntryChanged();
error:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;
}


NTSTATUS
MemDeleteValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE) hKey;
    PMEMREG_VALUE pRegValue = NULL;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &MemRegRoot()->lock);
    status = MemRegStoreFindNodeValue(
                 pKeyHandle->pKey->hNode,
                 pValueName,
                 &pRegValue);
    BAIL_ON_NT_STATUS(status);

    if (!pRegValue->Data)
    {
        status = STATUS_CANNOT_DELETE;
        BAIL_ON_NT_STATUS(status);
    }
    LWREG_SAFE_FREE_MEMORY(pRegValue->Data);
    pRegValue->DataLen = 0;
   
    MemDbExportEntryChanged();

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemEnumValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pdwReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    )
{
    NTSTATUS status = 0;
    REG_DB_CONNECTION regDbConn = {0};
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE) hKey;
    BOOLEAN bInLock = FALSE;

    regDbConn.pMemReg = pKeyHandle->pKey->hNode;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &MemRegRoot()->lock);
    status = MemDbEnumValue(
                 Handle,
                 &regDbConn,
                 dwIndex,
                 pValueName,
                 pcchValueName,
                 pdwReserved,
                 pType,
                 pData,
                 pcbData);

    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;
}


NTSTATUS
MemDeleteTree(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey
    )
{
    NTSTATUS status = 0;
    REG_DB_CONNECTION regDbConn = {0};
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE) hKey;
    BOOLEAN bInLock = FALSE;

    regDbConn.pMemReg = pKeyHandle->pKey->hNode;
    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &MemRegRoot()->lock);

    /*
     * First determine if any subkey has a non-zero refcount. Refuse
     * to delete the tree if this is true.
     */
    status = MemDbRecurseRegistry(
                 Handle,
                 &regDbConn,
                 pwszSubKey,
                 pfMemRegSubTreeRefCount,
                 NULL);
    BAIL_ON_NT_STATUS(status);
                 
    /* No keys are open, so proceed with the deletion of the subtree. */
    status = MemDbRecurseDepthFirstRegistry(
                 Handle,
                 &regDbConn,
                 pwszSubKey,
                 pfDeleteNodeCallback,
                 NULL);
    MemDbExportEntryChanged();

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &MemRegRoot()->lock);
    return status;
error:
    goto cleanup;
}



NTSTATUS
MemQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    )
{
    /* Not implemented yet, no one calls this function */
    return 0;
}
