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

static NTSTATUS
_MemCreateHkeyReply(
    IN MEM_REG_STORE_HANDLE pSubKey,
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

    pRetKey->hKey = pSubKey;
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

    status = MemDbOpen(&ghCacheConnection);
    if (status == 0)
    {
        *ppFnTable = &gRegMemProviderAPITable;
    }
    return status;
}


VOID
MemProvider_Shutdown(
    PREGPROV_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    MemDbClose(&ghCacheConnection);
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
    MEM_REG_STORE_HANDLE hRootKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};
    PWSTR pwszRootKey = NULL;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;
    ACCESS_MASK AccessGranted = 0;
    DWORD secLen = 0;

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
                     &hRootKey);
        BAIL_ON_NT_STATUS(status);
        regDbConn.pMemReg = hRootKey;
    }
    else
    {
        regDbConn.pMemReg = pKeyHandle->pKey->hKey;
    }
    if (ulSecDescLength > 0)
    {
        secLen = ulSecDescLength;
    }
    status = MemDbCreateKeyEx(
                 Handle,
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

    if (pSecDescRel)
    {
        status = RegSrvAccessCheckKey(pServerState->pToken,
                                      pSecDescRel,
                                      ulSecDescLength,
                                      AccessDesired,
                                      &AccessGranted);
    }

    if (STATUS_NO_TOKEN == status)
    {
        status = 0;
        AccessGranted = 0;
    }
    BAIL_ON_NT_STATUS(status);
    pKeyHandle->AccessGranted = AccessGranted;


cleanup:
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
    MEM_REG_STORE_HANDLE pSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;


    if (!hKey)
    {
        // Search for specified root key. If NULL, return HKTM.
        status = MemDbOpenKey(
                     Handle,
                     NULL,
                     pwszSubKey,
                     &pSubKey);
        BAIL_ON_NT_STATUS(status);
    }
    else if (pKeyHandle->pKey->hKey)
    {
        regDbConn.pMemReg = pKeyHandle->pKey->hKey;
//        regDbConn.lock = PTHREAD_MUTEX_INITIALIZER;
        status = MemDbOpenKey(
                     Handle,
                     &regDbConn,
                     pwszSubKey,
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

    if (!pServerState->pToken)
    {
        status = RegSrvCreateAccessToken(pServerState->peerUID,
                                         pServerState->peerGID,
                                         &pServerState->pToken);
        BAIL_ON_NT_STATUS(status);
    }



cleanup:
    return status;

error:
    goto cleanup;
}


VOID
MemCloseKey(
    IN HKEY hKey
    )
{
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    
    if (hKey)
    {
        LWREG_SAFE_FREE_MEMORY(pKeyHandle->pKey);
        LWREG_SAFE_FREE_MEMORY(pKeyHandle);
    }
}


NTSTATUS
MemDeleteKey(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    return 0;
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

    BAIL_ON_NT_STATUS(status);

    regDbConn.pMemReg = pKeyHandle->pKey->hKey;
    // regDbConn.lock = PTHREAD_MUTEX_INITIALIZER;
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

    regDbConn.pMemReg = pKeyHandle->pKey->hKey;

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

    regDbConn.pMemReg = pKeyHandle->pKey->hKey;
    status = MemDbSetValueEx(
                 Handle,
                 &regDbConn,
                 pValueName,
                 dwReserved,
                 dwType,
                 pData,
                 cbData);
    BAIL_ON_NT_STATUS(status);

cleanup:
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
    regDbConn.pMemReg = pKeyHandle->pKey->hKey;

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
    return 0;
}


NTSTATUS
MemDeleteValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    return 0;
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

    regDbConn.pMemReg = pKeyHandle->pKey->hKey;

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

    return status;
}


#if 1
/* Test call back function for MemDbRecurseRegistry() */
void *printf_func(MEM_REG_STORE_HANDLE pEntry, 
                  PVOID userContext,
                  PWSTR subStringPrefix)
{
    char *cString = NULL;
    DWORD index = 0;

    LwRtlCStringAllocateFromWC16String(&cString, (PWSTR) subStringPrefix);
    printf("[%s]\n", cString);
    LWREG_SAFE_FREE_MEMORY(cString);
    for (index=0; index<pEntry->ValuesLen; index++)
    {
        LwRtlCStringAllocateFromWC16String(
            &cString, 
            pEntry->Values[index]->Name);
            printf("    %s\n", cString);
        LWREG_SAFE_FREE_MEMORY(cString);
    }

    return NULL;
}

#endif

static void *pfDeleteNodeCallback(
    MEM_REG_STORE_HANDLE pEntry, 
    PVOID userContext,
    PWSTR subStringPrefix)
{
    MemRegStoreDeleteNode(pEntry);

    return NULL;
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

    regDbConn.pMemReg = pKeyHandle->pKey->hKey;

    status = MemDbRecurseDepthFirstRegistry(
                 Handle,
                 &regDbConn,
                 pwszSubKey,
                 pfDeleteNodeCallback,
                 NULL);
    return status;
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
    return 0;
}
