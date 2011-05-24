/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        sqliteapi.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SqliteOpenKeyEx_inlock_inDblock(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

static
NTSTATUS
SqliteDeleteKey_inlock_inDblock(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    );

static
NTSTATUS
SqliteDeleteTreeInternal_inlock_inDblock(
    IN HANDLE Handle,
    IN HKEY hKey
    );

DWORD
SqliteProvider_Initialize(
    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable,
    const PWSTR* ppwszRootKeyNames
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int iCount = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescLength = 0;


    dwError = RegDbOpen(REG_CACHE,
                        &ghCacheConnection);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegNtStatusToWin32Error(
                           RegDbFixAcls(ghCacheConnection));
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegHashCreate(
                    2 * 1024,
                    RegHashCaselessWC16StringCompare,
                    RegHashCaselessWc16String,
                    SqliteCacheFreeKeyCtxHashEntry,
                    NULL,
                    &gActiveKeyList.pKeyList);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegHashCreate(
                    2 * 1024,
                    RegHashCaselessWC16StringCompare,
                    RegHashCaselessWc16String,
                    SqliteCacheFreeDbKeyHashEntry,
                    NULL,
                    &gRegDbKeyList.pKeyList);
    BAIL_ON_REG_ERROR(dwError);

    // Creating default SD to create registry root key(s)
    dwError = RegSrvCreateDefaultSecDescRel(&pSecDescRel, &ulSecDescLength);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < NUM_ROOTKEY; iCount++)
    {
    	dwError = RegNtStatusToWin32Error(
    			                   SqliteCreateKeyInternal(NULL,
    			                		                   NULL,
    			                		                   ppwszRootKeyNames[iCount],
    			                		                   0,
    			                		                   pSecDescRel,
    			                		                   ulSecDescLength,
    			                		                   NULL,
    			                		                   NULL));
    	if (LWREG_ERROR_KEYNAME_EXIST == dwError)
    	{
    		dwError = 0;
    	}
        BAIL_ON_REG_ERROR(dwError);
    }

    *ppFnTable = &gRegSqliteProviderAPITable;

cleanup:
    LWREG_SAFE_FREE_MEMORY(pSecDescRel);

    return dwError;

error:
    *ppFnTable = NULL;
    goto cleanup;
}

VOID
SqliteProvider_Shutdown(
    PREGPROV_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    RegDbSafeClose(&ghCacheConnection);

    if (gActiveKeyList.pKeyList)
    {
        RegHashSafeFree(&gActiveKeyList.pKeyList);
    }

    if (gRegDbKeyList.pKeyList)
    {
        RegHashSafeFree(&gRegDbKeyList.pKeyList);
    }
}

NTSTATUS
SqliteCreateKeyEx(
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
	NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PWSTR pwszKeyNameWithSubKey = NULL;
    PREG_KEY_CONTEXT pKeyCtx = NULL;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_NT_INVALID_POINTER(pSubKey);
    BAIL_ON_INVALID_RESERVED_VALUE(dwReserved);

    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    // Check key length
    if (wc16slen(pSubKey) > MAX_KEY_LENGTH)
    {
    	status = STATUS_INVALID_BLOCK_LENGTH;
    	BAIL_ON_NT_STATUS(status);
    }

    // check whether key is valid
	if (!RegSrvIsValidKeyName(pSubKey))
	{
		status = STATUS_OBJECT_NAME_INVALID;
		BAIL_ON_NT_STATUS(status);
	}

	// Check whether create subkey is allowed on parentKey
	status = RegSrvAccessCheckKeyHandle(pKeyHandle,
 			                            KEY_CREATE_SUB_KEY);
	BAIL_ON_NT_STATUS(status);

	status = LwRtlWC16StringAllocatePrintfW(
					&pwszKeyNameWithSubKey,
					L"%ws\\%ws",
					pKeyCtx->pwszKeyName,
					pSubKey);
	BAIL_ON_NT_STATUS(status);

    status =  SqliteCreateKeyInternal(
    		   Handle,
    		   pKeyCtx,
    		   pwszKeyNameWithSubKey,
    		   AccessDesired,
    		   pSecDescRel,
    		   ulSecDescLength,
               (PREG_KEY_HANDLE*)phkResult,
               pdwDisposition);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteOpenKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PWSTR pwszKeyNameWithSubKey = NULL;
    PREG_KEY_HANDLE pOpenKeyHandle = NULL;
    PREG_KEY_CONTEXT pKey = NULL;
    PCWSTR pwszFullKeyName = NULL;

    if (pKeyHandle)
    {
    	pKey = pKeyHandle->pKey;

    	if (LW_IS_NULL_OR_EMPTY_STR(pKey->pwszKeyName))
        {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

        if (pwszSubKey)
        {
            status = LwRtlWC16StringAllocatePrintfW(
                            &pwszKeyNameWithSubKey,
                            L"%ws\\%ws",
                            pKey->pwszKeyName,
                            pwszSubKey);
            BAIL_ON_NT_STATUS(status);
        }

        pwszFullKeyName = pwszSubKey ? pwszKeyNameWithSubKey :  pKey->pwszKeyName;
    }
    else
    {
    	pwszFullKeyName =  pwszSubKey;
    }

	status = SqliteOpenKeyInternal(Handle,
			                       pwszFullKeyName,
								   AccessDesired,
								   &pOpenKeyHandle);
	BAIL_ON_NT_STATUS(status);

    *phkResult = (HKEY)pOpenKeyHandle;

cleanup:

    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);

    return status;

error:

    SqliteSafeFreeKeyHandle(pOpenKeyHandle);
    *phkResult = NULL;

    goto cleanup;
}

VOID
SqliteCloseKey(
    IN HKEY hKey
    )
{
	SqliteSafeFreeKeyHandle((PREG_KEY_HANDLE)hKey);
}

NTSTATUS
SqliteDeleteKey(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;
    PWSTR pwszKeyName = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKey = NULL;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_NT_INVALID_POINTER(pServerState);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, DELETE);
    BAIL_ON_NT_STATUS(status);

    pKey = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKey);

    BAIL_ON_NT_INVALID_POINTER(pSubKey);

    status = LwRtlWC16StringAllocatePrintfW(
                    &pwszKeyName,
                    L"%ws\\%ws",
                    pKey->pwszKeyName,
                    pSubKey);
    BAIL_ON_NT_STATUS(status);

    if (!pServerState->pToken)
    {
        status = RegSrvCreateAccessToken(pServerState->peerUID,
                                         pServerState->peerGID,
                                         &pServerState->pToken);
        BAIL_ON_NT_STATUS(status);
    }

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    status = SqliteDeleteKeyInternal_inlock(Handle, pwszKeyName);
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    LWREG_SAFE_FREE_MEMORY(pwszKeyName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteQueryInfoKey(
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
    NTSTATUS status = STATUS_SUCCESS;
    size_t sNumSubKeys = 0;
    size_t sNumValues = 0;
    DWORD dwOffset = 0;
    size_t sNumDefaultValues = 0;
    DWORD dwDefaultOffset = 0;
    BOOLEAN bInLock = FALSE;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_INVALID_RESERVED_POINTER(pdwReserved);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;

    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyCtx->mutex);

    status = SqliteCacheSubKeysInfo_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);

    status = SqliteCacheKeyValuesInfo_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);

    status = SqliteCacheKeyDefaultValuesInfo_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);

    if (pKeyCtx->dwNumSubKeys > pKeyCtx->dwNumCacheSubKeys)
    {
        dwOffset = pKeyCtx->dwNumCacheSubKeys;
        do
        {
            status = SqliteCacheUpdateSubKeysInfo_inlock(
                              dwOffset,
                              pKeyCtx,
                              &sNumSubKeys);
            BAIL_ON_NT_STATUS(status);

            dwOffset+= (DWORD)sNumSubKeys;
        } while (sNumSubKeys);
    }

    if (pKeyCtx->dwNumValues > pKeyCtx->dwNumCacheValues)
    {
        dwOffset = pKeyCtx->dwNumCacheValues;
        do
        {
            status = SqliteCacheUpdateValuesInfo_inlock(
                              dwOffset,
                              pKeyCtx,
                              &sNumValues);
            BAIL_ON_NT_STATUS(status);

            dwOffset+= (DWORD)sNumValues;
        } while (sNumValues);
    }

    if (pKeyCtx->dwNumDefaultValues > pKeyCtx->dwNumCacheDefaultValues)
    {
        dwDefaultOffset = pKeyCtx->dwNumCacheDefaultValues;
        do
        {
            status = SqliteCacheUpdateDefaultValuesInfo_inlock(
                              dwDefaultOffset,
                              pKeyCtx,
                              &sNumDefaultValues);
            BAIL_ON_NT_STATUS(status);

            dwDefaultOffset+= (DWORD)sNumDefaultValues;
        } while (sNumDefaultValues);
    }

    status = SqliteCacheKeySecurityDescriptor_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);


    if (pcSubKeys)
    {
        *pcSubKeys = pKeyCtx->dwNumSubKeys;
    }
    if (pcMaxSubKeyLen)
    {
        *pcMaxSubKeyLen = pKeyCtx->sMaxSubKeyLen;
    }
    if (pcValues)
    {
        *pcValues = pKeyCtx->dwNumValues + pKeyCtx->dwNumDefaultValues;
    }
    if (pcMaxValueNameLen)
    {
        *pcMaxValueNameLen = pKeyCtx->sMaxValueNameLen;
    }
    if (pcMaxValueLen)
    {
        *pcMaxValueLen = pKeyCtx->sMaxValueLen;
    }
    if (pcbSecurityDescriptor)
    {
        *pcbSecurityDescriptor = (DWORD)pKeyCtx->ulSecDescLength;
    }

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyCtx->mutex);

    return status;

error:
    if (pcSubKeys)
    {
        *pcSubKeys = 0;
    }
    if (pcMaxSubKeyLen)
    {
        *pcMaxSubKeyLen = 0;
    }
    if (pcValues)
    {
        *pcValues = 0;
    }
    if (pcMaxValueNameLen)
    {
        *pcMaxValueNameLen = 0;
    }
    if (pcMaxValueLen)
    {
        *pcMaxValueLen = 0;
    }

    goto cleanup;
}

NTSTATUS
SqliteEnumKeyEx(
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
	NTSTATUS status = STATUS_SUCCESS;
    size_t sSubKeyLen = 0;
    // Do not free
    PWSTR pSubKeyName = NULL;
    BOOLEAN bInLock = FALSE;
    size_t sNumSubKeys = 0;
    PREG_DB_KEY* ppRegEntries = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_INVALID_RESERVED_POINTER(pdwReserved);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_ENUMERATE_SUB_KEYS);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;

    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);
    // the size of pName is *pcName
    BAIL_ON_NT_INVALID_POINTER(pName);
    BAIL_ON_NT_INVALID_POINTER(pcName);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyCtx->mutex);

    // Try to grab information from pKeyResults:
    // if subkey information is not yet available in pKeyResult, do it here
    // Otherwise, use this information
    status = SqliteCacheSubKeysInfo_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);

    if (!pKeyCtx->dwNumSubKeys)
    {
        goto cleanup;
    }

    if (dwIndex >= pKeyCtx->dwNumSubKeys)
    {
    	status = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NT_STATUS(status);
    }

    if (dwIndex < pKeyCtx->dwNumCacheSubKeys)
    {
    	pSubKeyName = pKeyCtx->ppwszSubKeyNames[dwIndex];
    }
    else
    {
    	status = RegDbQueryInfoKey(ghCacheConnection,
    			                   pKeyCtx->pwszKeyName,
    			                   pKeyCtx->qwId,
                                   1,
                                   dwIndex,
                                   &sNumSubKeys,
                                   &ppRegEntries);
        BAIL_ON_NT_STATUS(status);

        if (sNumSubKeys > 1)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        pSubKeyName = ppRegEntries[0]->pwszKeyName;
    }

    if (pSubKeyName)
    {
    	sSubKeyLen = RtlWC16StringNumChars(pSubKeyName);
    }

    if (*pcName < sSubKeyLen+1)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pName, pSubKeyName, sSubKeyLen*sizeof(*pName));
    pName[sSubKeyLen] = (LW_WCHAR)'\0';
    *pcName = (DWORD)sSubKeyLen;

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyCtx->mutex);

    RegDbSafeFreeEntryKeyList(sNumSubKeys,&ppRegEntries);

    return status;

error:
    *pcName = 0;

    goto cleanup;
}

NTSTATUS
SqliteEnumKeyEx_inDblock(
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
	NTSTATUS status = STATUS_SUCCESS;
    size_t sSubKeyLen = 0;
    // Do not free
    PWSTR pSubKeyName = NULL;
    BOOLEAN bInLock = FALSE;
    size_t sNumSubKeys = 0;
    PREG_DB_KEY* ppRegEntries = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_INVALID_RESERVED_POINTER(pdwReserved);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_ENUMERATE_SUB_KEYS);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;

    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);
    BAIL_ON_NT_INVALID_POINTER(pName); // the size of pName is *pcName
    BAIL_ON_NT_INVALID_POINTER(pcName);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyCtx->mutex);

    // Try to grab information from pKeyResults:
    // if subkey information is not yet available in pKeyResult, do it here
    // Otherwise, use this information
    status = SqliteCacheSubKeysInfo_inlock_inDblock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);

    if (!pKeyCtx->dwNumSubKeys)
    {
        goto cleanup;
    }

    if (dwIndex >= pKeyCtx->dwNumSubKeys)
    {
    	status = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NT_STATUS(status);
    }

    if (dwIndex < pKeyCtx->dwNumCacheSubKeys)
    {
    	pSubKeyName = pKeyCtx->ppwszSubKeyNames[dwIndex];
    }
    else
    {
    	status = RegDbQueryInfoKey_inlock(ghCacheConnection,
    			                   pKeyCtx->pwszKeyName,
    			                   pKeyCtx->qwId,
                                   1,
                                   dwIndex,
                                   &sNumSubKeys,
                                   &ppRegEntries);
        BAIL_ON_NT_STATUS(status);

        if (sNumSubKeys > 1)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        pSubKeyName = ppRegEntries[0]->pwszKeyName;
    }

    if (pSubKeyName)
    {
    	sSubKeyLen = RtlWC16StringNumChars(pSubKeyName);
    }

    if (*pcName < sSubKeyLen+1)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pName, pSubKeyName, sSubKeyLen*sizeof(*pName));
    pName[sSubKeyLen] = (LW_WCHAR)'\0';
    *pcName = (DWORD)sSubKeyLen;

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyCtx->mutex);

    RegDbSafeFreeEntryKeyList(sNumSubKeys,&ppRegEntries);

    return status;

error:
    *pcName = 0;

    goto cleanup;
}

NTSTATUS
SqliteSetValueEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR   pwszValueName = NULL;
    PWSTR*  ppwszOutMultiSz = NULL;
    BOOLEAN bIsWrongType = TRUE;
    wchar16_t wszEmptyValueName[] = REG_EMPTY_VALUE_NAME_W;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_INVALID_RESERVED_VALUE(dwReserved);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_SET_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    status = LwRtlWC16StringDuplicate(&pwszValueName, !pValueName ? wszEmptyValueName : pValueName);
    BAIL_ON_NT_STATUS(status);

    status = RegDbGetKeyValue(ghCacheConnection,
    		                  pKeyCtx->qwId,
    		                  (PCWSTR)pwszValueName,
                              (REG_DATA_TYPE)dwType,
                              &bIsWrongType,
                              NULL);
    if (!status)
    {
        status = STATUS_DUPLICATE_NAME;
        BAIL_ON_NT_STATUS(status);
    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
    {
        status = 0;
    }
    BAIL_ON_NT_STATUS(status);

    if (!cbData)
    {
    	goto done;
    }

    switch (dwType)
    {
        case REG_BINARY:
        case REG_DWORD:
            break;

        case REG_MULTI_SZ:
        case REG_SZ:
        	if (cbData == 1)
        	{
                status = STATUS_INTERNAL_ERROR;
                BAIL_ON_NT_STATUS(status);
        	}

            if (pData[cbData-1] != '\0' || pData[cbData-2] != '\0' )
            {
                status = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(status);
            }

            break;

        default:
        	status = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(status);
    }

done:
    status = RegDbSetKeyValue(ghCacheConnection,
    		                  pKeyCtx->qwId,
							  (PCWSTR)pwszValueName,
						       (const PBYTE)pData,
						       cbData,
							  (REG_DATA_TYPE)dwType,
							  NULL);
    BAIL_ON_NT_STATUS(status);

    SqliteCacheResetKeyValueInfo(pKeyCtx->pwszKeyName);

cleanup:

    LWREG_SAFE_FREE_MEMORY(pwszValueName);

    if (ppwszOutMultiSz)
    {
        RegFreeMultiStrsW(ppwszOutMultiSz);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteGetValue(
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
    NTSTATUS status = STATUS_SUCCESS;
    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;
    // Do not free
    PBYTE pValue = NULL;
    DWORD cbValue = 0;
    REG_DATA_TYPE dwType = REG_NONE;

    status = SqliteGetValueAttributes_Internal(
                                      Handle,
                                      hKey,
                                      pSubKey,
                                      pValueName,
                                      GetRegDataType(Flags),
                                      FALSE,
                                      &pCurrentValue,
                                      &pValueAttributes);
    BAIL_ON_NT_STATUS(status);

    // Value does not exist
    if (!pCurrentValue && !pValueAttributes)
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    // Value is not set by client, hence using value attributes
    if (!pCurrentValue && pValueAttributes)
    {
        pValue = pValueAttributes->pDefaultValue;
        cbValue = pValueAttributes->DefaultValueLen;
        dwType = pValueAttributes->ValueType;
    }
    else if (pCurrentValue)
    {
        pValue = pCurrentValue->pvData;
        cbValue = pCurrentValue->cbData;
        dwType = pCurrentValue->dwType;
    }

    status = RegCopyValueBytes(pValue,
                               cbValue,
                               pData,
                               pcbData);
    BAIL_ON_NT_STATUS(status);

    *pdwType = dwType;

cleanup:
    RegSafeFreeCurrentValueInfo(&pCurrentValue);
    RegSafeFreeValueAttributes(&pValueAttributes);

    return status;

error:
    *pdwType = 0;

    goto cleanup;
}

NTSTATUS
SqliteDeleteKeyValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszValueName = NULL;
    wchar16_t wszEmptyValueName[] = REG_EMPTY_VALUE_NAME_W;
    PCWSTR pwszFullKeyName = NULL;
    PWSTR pwszKeyNameWithSubKey = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    PREG_KEY_HANDLE pKeyHandleInUse = NULL;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    if (pSubKey)
    {
        status = LwRtlWC16StringAllocatePrintfW(
                        &pwszKeyNameWithSubKey,
                        L"%ws\\%ws",
                        pKeyCtx->pwszKeyName,
                        pSubKey);
        BAIL_ON_NT_STATUS(status);

    }

    pwszFullKeyName = pSubKey ? pwszKeyNameWithSubKey : pKeyCtx->pwszKeyName;

	status = SqliteOpenKeyInternal(Handle,
			                       pwszFullKeyName,
			                       KEY_SET_VALUE,
								   &pKeyHandleInUse);
	BAIL_ON_NT_STATUS(status);

	// ACL check
    status = RegSrvAccessCheckKeyHandle(pKeyHandleInUse, KEY_SET_VALUE);
    BAIL_ON_NT_STATUS(status);;

    status = LwRtlWC16StringDuplicate(&pwszValueName, !pValueName ? wszEmptyValueName : pValueName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteDeleteValue(Handle,
                               (HKEY)pKeyHandleInUse,
                               pwszValueName);
    BAIL_ON_NT_STATUS(status);

cleanup:
    SqliteSafeFreeKeyHandle(pKeyHandleInUse);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteDeleteValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszValueName = NULL;
    wchar16_t wszEmptyValueName[] = REG_EMPTY_VALUE_NAME_W;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)ghCacheConnection;
    PSTR pszError = NULL;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
	// ACL check
    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_SET_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    status = LwRtlWC16StringDuplicate(&pwszValueName, !pValueName ? wszEmptyValueName : pValueName);
    BAIL_ON_NT_STATUS(status);

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
                    pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    status = RegDbGetKeyValue_inlock((REG_DB_HANDLE)pConn,
    		                         pKeyCtx->qwId,
    		                         pwszValueName,
                                     REG_NONE,
                                     NULL,
                                     NULL);
    // (1) if value is user-set delete the user set value
    if (!status)
    {
        status = RegDbDeleteKeyValue_inlock((REG_DB_HANDLE)pConn,
                                            pKeyCtx->qwId,
                                            pwszValueName);
        BAIL_ON_NT_STATUS(status);
    }
    // (2) if value is not user-set
    else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
    {
        status = RegDbGetValueAttributes_inlock((REG_DB_HANDLE)pConn,
                                                pKeyCtx->qwId,
                                                pwszValueName,
                                                REG_NONE,
                                                NULL,
                                                NULL);
        // (2).1 value is not user-set but has schema info
        if (!status)
        {
            status = STATUS_CANNOT_DELETE;
        }
        // (2).2 value is not user-set and has no schema info
        // bail with status that is generated
        BAIL_ON_NT_STATUS(status);
    }
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c SqliteDeleteValue() finished");

    // To persist the locking order. On success reset key value info
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    SqliteCacheResetKeyValueInfo(pKeyCtx->pwszKeyName);

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);

    return status;

error:
    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
                 "rollback",
                 NULL,
                 NULL,
                 NULL);

    goto cleanup;
}

NTSTATUS
SqliteEnumValue(
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
	NTSTATUS status = STATUS_SUCCESS;
    size_t sValueNameLen = 0;
    REG_DATA_TYPE valueType = REG_NONE;
    BOOLEAN bInLock = FALSE;
    size_t sNumValues = 0;
    PREG_DB_VALUE* ppRegEntries = NULL;
    // Do not free
    PBYTE pValueContent = NULL;
    // Do not free
    PWSTR pValName = NULL;
    DWORD dwValueLen = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    DWORD dwTotalValue = 0;
    size_t sNumDefaultValues = 0;
    PREG_DB_VALUE_ATTRIBUTES* ppRegDefaultEntries = NULL;
    DWORD dwDefaultIndex = 0;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_INVALID_RESERVED_POINTER(pdwReserved);
	// ACL check
    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_QUERY_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);
    BAIL_ON_NT_INVALID_POINTER(pValueName); // the size of pName is *pcName
    BAIL_ON_NT_INVALID_POINTER(pcchValueName);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyCtx->mutex);

    // Try to grab information from pKeyResults:
    // if subkey information is not yet available in pKeyResult, do it here
    // Otherwise, use this information
    status = SqliteCacheKeyValuesInfo_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);

    status = SqliteCacheKeyDefaultValuesInfo_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);


    dwTotalValue = pKeyCtx->dwNumValues + pKeyCtx->dwNumDefaultValues;
    if (dwTotalValue == 0)
    {
        goto cleanup;
    }

    if (dwIndex >= dwTotalValue)
    {
    	status = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NT_STATUS(status);
    }

    // (1) Enumrate user specified values that has been cached
    if (dwIndex < pKeyCtx->dwNumCacheValues)
    {
        pValName = pKeyCtx->ppwszValueNames[dwIndex];
        valueType = pKeyCtx->pTypes[dwIndex];
        dwValueLen = pKeyCtx->pdwValueLen[dwIndex];
        pValueContent = pKeyCtx->ppValues[dwIndex];

        goto done;
    }

    // (2) Enumrate user specified values that has been not been cached
    // until there are no more user specified values.
    if (dwIndex >= pKeyCtx->dwNumCacheValues)
    {
    	status = RegDbQueryInfoKeyValue(ghCacheConnection,
    			                        pKeyCtx->qwId,
                                        1,
                                        dwIndex,
                                        &sNumValues,
                                        &ppRegEntries);
    	// sNumValues == 0 means no more entry
    	if (!status  && !sNumValues)
    	{
    	    // (3) Enumrate default values that has been cached
    	    dwDefaultIndex = dwIndex-pKeyCtx->dwNumValues;

    	    if (dwDefaultIndex < pKeyCtx->dwNumCacheDefaultValues)
    	    {
    	        pValName = pKeyCtx->ppwszDefaultValueNames[dwDefaultIndex];
    	        valueType = pKeyCtx->pDefaultTypes[dwDefaultIndex];
    	        dwValueLen = pKeyCtx->pdwDefaultValueLen[dwDefaultIndex];
    	        pValueContent = pKeyCtx->ppDefaultValues[dwDefaultIndex];

    	        goto done;
    	    }
    	    // (4) Enumrate default values that has not been cached
    	    else if (dwDefaultIndex >= pKeyCtx->dwNumCacheDefaultValues)
    	    {
    	        status = RegDbQueryDefaultValues(ghCacheConnection,
    	                                         pKeyCtx->qwId,
    	                                         1,
    	                                         dwDefaultIndex,
    	                                         &sNumDefaultValues,
    	                                         &ppRegDefaultEntries);
    	        BAIL_ON_NT_STATUS(status);

    	        if (sNumDefaultValues > 1)
    	        {
    	            status = STATUS_INTERNAL_ERROR;
    	            BAIL_ON_NT_STATUS(status);
    	        }

    	        pValName = ppRegDefaultEntries[0]->pwszValueName;
    	        valueType = ppRegDefaultEntries[0]->pValueAttributes->ValueType;
    	        dwValueLen = ppRegDefaultEntries[0]->pValueAttributes->DefaultValueLen;
    	        pValueContent = ppRegDefaultEntries[0]->pValueAttributes->pDefaultValue;

    	        goto done;
    	    }
    	}
        BAIL_ON_NT_STATUS(status);

        if (sNumValues > 1)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

    	pValName = ppRegEntries[0]->pwszValueName;
        valueType = ppRegEntries[0]->type;
        dwValueLen = ppRegEntries[0]->dwValueLen;
        pValueContent = ppRegEntries[0]->pValue;
    }

done:
    if (pValName)
    {
    	sValueNameLen = RtlWC16StringNumChars(pValName);
    }

    if (*pcchValueName < sValueNameLen+1)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pValueName, pValName, (sValueNameLen+1)*sizeof(*pValueName));
    *pcchValueName = (DWORD)sValueNameLen;

    if (pcbData)
    {
        status = RegCopyValueBytes(pValueContent,
        		                   dwValueLen,
                                   pData,
                                   pcbData);
        BAIL_ON_NT_STATUS(status);
    }

    if (pType)
    {
        *pType = valueType;
    }

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyCtx->mutex);

    RegDbSafeFreeEntryValueList(sNumValues,&ppRegEntries);
    RegDbSafeFreeEntryValueAttributesList(sNumDefaultValues,
                                          &ppRegDefaultEntries);

    return status;

error:
    *pcchValueName = 0;

    if (pType)
    {
        *pType = REG_NONE;
    }

    goto cleanup;

    return status;
}

NTSTATUS
SqliteDeleteTree(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    HKEY hCurrKey = NULL;
    BOOLEAN bInDbLock = FALSE;
    BOOLEAN bInLock = FALSE;
    PSTR pszError = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)ghCacheConnection;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;

    BAIL_ON_NT_INVALID_POINTER(pServerState);

	// ACL check
    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_ALL_ACCESS | DELETE);
    BAIL_ON_NT_STATUS(status);

    if (!pServerState->pToken)
    {
        status = RegSrvCreateAccessToken(pServerState->peerUID,
                                         pServerState->peerGID,
                                         &pServerState->pToken);
        BAIL_ON_NT_STATUS(status);
    }

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    ENTER_SQLITE_LOCK(&pConn->lock, bInDbLock);

    status = sqlite3_exec(
    		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    if (pSubKey)
    {
        status = SqliteOpenKeyEx_inlock_inDblock(Handle,
                                  hKey,
                                  pSubKey,
                                  0,
                                  KEY_ALL_ACCESS | DELETE,
                                  &hCurrKey);
        BAIL_ON_NT_STATUS(status);

        status = SqliteDeleteTreeInternal_inlock_inDblock(
        		                           Handle,
                                           hCurrKey);
        BAIL_ON_NT_STATUS(status);

        if (hCurrKey)
        {
            SqliteCloseKey_inlock(hCurrKey);
            hCurrKey = NULL;
        }

        status = SqliteDeleteKey_inlock_inDblock(Handle, hKey, pSubKey);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = SqliteDeleteTreeInternal_inlock_inDblock(Handle,
                                                   hKey);
        BAIL_ON_NT_STATUS(status);
    }

    status = sqlite3_exec(
    		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c SqliteDeleteTree() finished");

cleanup:
    if (hCurrKey)
    {
        SqliteCloseKey_inlock(hCurrKey);
    }

    LEAVE_SQLITE_LOCK(&pConn->lock, bInDbLock);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return status;

error:
    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
	 			 "rollback",
	 			 NULL,
	 		     NULL,
			     NULL);

    goto cleanup;
}

NTSTATUS
SqliteQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    DWORD dwTotalSize = 0;
    PREG_DB_VALUE* ppRegEntries = NULL;
    int iCount  = 0;
    int iOffSet = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);

    pKeyCtx = pKeyHandle->pKey;

    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);
    BAIL_ON_NT_INVALID_POINTER(pVal_list);

    if (!num_vals)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&ppRegEntries, REG_DB_VALUE, sizeof(*ppRegEntries) * num_vals);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < num_vals; iCount++)
    {
    	BAIL_ON_NT_INVALID_POINTER(pVal_list[iCount].ve_valuename);

        status = RegDbGetKeyValue(ghCacheConnection,
        		                  pKeyCtx->qwId,
        		                  pVal_list[iCount].ve_valuename,
                                  REG_NONE,
                                  NULL,
                                  &ppRegEntries[iCount]);
        BAIL_ON_NT_STATUS(status);

        /* record val length */
        pVal_list[iCount].ve_valuelen = (DWORD)ppRegEntries[iCount]->dwValueLen;
        dwTotalSize += (DWORD)ppRegEntries[iCount]->dwValueLen;
    }

    if (!pVal_list)
        goto cleanup;

    if (pdwTotalsize && *pdwTotalsize < dwTotalSize)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < num_vals; iCount++)
    {
        iOffSet = iCount == 0 ? 0 : (iOffSet + pVal_list[iCount-1].ve_valuelen);

        /* value type*/
        pVal_list[iCount].ve_type = ppRegEntries[iCount]->type;

        memcpy(pValue+iOffSet, ppRegEntries[iCount]->pValue, pVal_list[iCount].ve_valuelen*sizeof(*ppRegEntries[iCount]->pValue));
    }

cleanup:

    if (pdwTotalsize)
    {
        *pdwTotalsize = dwTotalSize;
    }

    RegDbSafeFreeEntryValueList(num_vals, &ppRegEntries);

    return status;

error:
    goto cleanup;
}

REG_DATA_TYPE
GetRegDataType(
    REG_DATA_TYPE_FLAGS Flags
    )
{
    REG_DATA_TYPE dataType = 0;

    switch (Flags)
    {
        case RRF_RT_REG_SZ:
            dataType = REG_SZ;
            break;
        case RRF_RT_REG_BINARY:
            dataType = REG_BINARY;
            break;
        case RRF_RT_REG_DWORD:
            dataType = REG_DWORD;
            break;
        case RRF_RT_REG_MULTI_SZ:
            dataType = REG_MULTI_SZ;
            break;

        default:
            dataType = REG_NONE;
    }

    return dataType;
}

static
NTSTATUS
SqliteOpenKeyEx_inlock_inDblock(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PWSTR pwszKeyNameWithSubKey = NULL;
    PREG_KEY_HANDLE pOpenKeyHandle = NULL;
    PREG_KEY_CONTEXT pKey = NULL;
    PCWSTR pwszFullKeyName = NULL;

    if (pKeyHandle)
    {
        pKey = pKeyHandle->pKey;

        if (LW_IS_NULL_OR_EMPTY_STR(pKey->pwszKeyName))
        {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

        if (pwszSubKey)
        {
            status = LwRtlWC16StringAllocatePrintfW(
                            &pwszKeyNameWithSubKey,
                            L"%ws\\%ws",
                            pKey->pwszKeyName,
                            pwszSubKey);
            BAIL_ON_NT_STATUS(status);
        }

        pwszFullKeyName = pwszSubKey ? pwszKeyNameWithSubKey :  pKey->pwszKeyName;
    }
    else
    {
        pwszFullKeyName =  pwszSubKey;
    }

    status = SqliteOpenKeyInternal_inlock_inDblock(Handle,
                                                   pwszFullKeyName,
                                                   AccessDesired,
                                                   &pOpenKeyHandle);
    BAIL_ON_NT_STATUS(status);

    *phkResult = (HKEY)pOpenKeyHandle;

cleanup:

    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);

    return status;

error:

    SqliteSafeFreeKeyHandle_inlock(pOpenKeyHandle);
    *phkResult = NULL;

    goto cleanup;
}

static
NTSTATUS
SqliteDeleteKey_inlock_inDblock(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszKeyName = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKey = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_CREATE_SUB_KEY);
    BAIL_ON_NT_STATUS(status);

    pKey = pKeyHandle->pKey;

    BAIL_ON_INVALID_KEY_CONTEXT(pKey);
    BAIL_ON_NT_INVALID_POINTER(pSubKey);

    status = LwRtlWC16StringAllocatePrintfW(
                    &pwszKeyName,
                    L"%ws\\%ws",
                    pKey->pwszKeyName,
                    pSubKey);
    BAIL_ON_NT_STATUS(status);

    status = SqliteDeleteKeyInternal_inlock_inDblock(Handle, pwszKeyName);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszKeyName);

    return status;

error:
    goto cleanup;
}

/*delete all subkeys and values of hKey*/
static
NTSTATUS
SqliteDeleteTreeInternal_inlock_inDblock(
    IN HANDLE Handle,
    IN HKEY hKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HKEY hCurrKey = NULL;
    int iCount = 0;
    DWORD dwSubKeyCount = 0;
    LW_WCHAR psubKeyName[MAX_KEY_LENGTH];
    DWORD dwSubKeyLen = 0;
    PWSTR* ppwszSubKey = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    status = RegDbQueryInfoKeyCount_inlock(ghCacheConnection,
                                           pKeyCtx->qwId,
                                           QuerySubKeys,
                                           (size_t*)&dwSubKeyCount);
    BAIL_ON_NT_STATUS(status);

    if (dwSubKeyCount)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&ppwszSubKey, PWSTR, sizeof(*ppwszSubKey) * dwSubKeyCount);
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < dwSubKeyCount; iCount++)
    {
        dwSubKeyLen = MAX_KEY_LENGTH;
        memset(psubKeyName, 0, MAX_KEY_LENGTH);

        status = SqliteEnumKeyEx_inDblock(Handle,
                                  hKey,
                                  iCount,
                                  psubKeyName,
                                  &dwSubKeyLen,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);
        BAIL_ON_NT_STATUS(status);

        status = LwRtlWC16StringDuplicate(&ppwszSubKey[iCount], psubKeyName);
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < dwSubKeyCount; iCount++)
    {
        status = SqliteOpenKeyEx_inlock_inDblock(Handle,
                                  hKey,
                                  ppwszSubKey[iCount],
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hCurrKey);
        BAIL_ON_NT_STATUS(status);

        status = SqliteDeleteTreeInternal_inlock_inDblock(
                                           Handle,
                                           hCurrKey);
        BAIL_ON_NT_STATUS(status);

        if (hCurrKey)
        {
            SqliteCloseKey_inlock(hCurrKey);
            hCurrKey = NULL;
        }

        status = SqliteDeleteKey_inlock_inDblock(Handle, hKey, ppwszSubKey[iCount]);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (hCurrKey)
    {
        SqliteCloseKey_inlock(hCurrKey);
    }
    RegFreeWC16StringArray(ppwszSubKey, dwSubKeyCount);

    return status;


error:
    goto cleanup;
}

