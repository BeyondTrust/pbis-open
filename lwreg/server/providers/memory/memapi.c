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
    return 0;
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
    PREG_KEY_HANDLE phKeyResponse = NULL;
    MEM_REG_STORE_HANDLE pSubKey = NULL;
    PREG_KEY_CONTEXT pRetKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};

    if (!hKey)
    {
        // Search for specified root key. If NULL, return HKTM.
        status = MemDbOpenKey(
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
                     &regDbConn,
                     pwszSubKey,
                     &pSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    if (phKeyResult)
    {
        status = LW_RTL_ALLOCATE(
                     (PVOID*)&phKeyResponse,
                     PREG_KEY_HANDLE,
                     sizeof(*phKeyResponse));
        BAIL_ON_NT_STATUS(status);

        status = LW_RTL_ALLOCATE(
                     (PVOID*)&pRetKey,
                     REG_KEY_CONTEXT,
                     sizeof(*pRetKey));
        BAIL_ON_NT_STATUS(status);

        pRetKey->hKey = pSubKey;
        phKeyResponse->pKey = pRetKey;
        *phKeyResult = phKeyResponse;
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
    OUT PWSTR pClass, /*A pointer to a buffer that receives the user-defined class of the key. This parameter can be NULL.*/
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pdwReserved,/*This parameter is reserved and must be NULL.*/
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,/*implement this later*/
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime/*implement this later*/
    )
{
    return 0;
}


NTSTATUS
MemEnumKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pName, /*buffer to hold keyName*/
    IN OUT PDWORD pcName,/*When the function returns, the variable receives the number of characters stored in the buffer,not including the terminating null character.*/
    IN PDWORD pdwReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT PFILETIME pftLastWriteTime
    )
{
    return 0;
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
    return 0;
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
    return 0;
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
    return 0;
}


NTSTATUS
MemDeleteTree(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    return 0;
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
