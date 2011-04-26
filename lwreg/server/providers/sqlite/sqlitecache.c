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
 *        sqlitecache.c
 *
 * Abstract:
 *
 *        Registry Sqlite backend caching layer APIs
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */
#include "includes.h"


/*Find whether this key has already been in active Key list,
 * If not, create key context and add the active key to the list
 * Otherwise, increment the existing active key reference count by 1
 */
PREG_KEY_CONTEXT
SqliteCacheLocateActiveKey(
    IN PCWSTR pwszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = NULL;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = SqliteCacheLocateActiveKey_inlock(pwszKeyName);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return pKeyResult;
}

PREG_KEY_CONTEXT
SqliteCacheLocateActiveKey_inlock(
    IN PCWSTR pwszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = NULL;
    DWORD status = 0;

    status = RegHashGetValue(gActiveKeyList.pKeyList,
    		                 pwszKeyName,
                              (PVOID*)&pKeyResult);
    if (!status && pKeyResult)
    {
        LwInterlockedIncrement(&pKeyResult->refCount);
    }

    return pKeyResult;
}

NTSTATUS
SqliteCacheInsertActiveKey(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    status = SqliteCacheInsertActiveKey_inlock(pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheInsertActiveKey_inlock(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegHashSetValue(gActiveKeyList.pKeyList,
                              (PVOID)pKeyResult->pwszKeyName,
                              (PVOID)pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:
    goto cleanup;
}

VOID
SqliteCacheDeleteActiveKey(
    IN PWSTR pwszKeyName
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    SqliteCacheDeleteActiveKey_inlock(pwszKeyName);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return;
}

VOID
SqliteCacheDeleteActiveKey_inlock(
    IN PWSTR pwszKeyName
    )
{
    REG_HASH_ITERATOR hashIterator;
    REG_HASH_ENTRY* pHashEntry = NULL;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    int iCount = 0;
    NTSTATUS status = 0;

    status = RegHashGetValue(gActiveKeyList.pKeyList,
    		                 pwszKeyName,
    		                 (PVOID*)&pKeyResult);
    if (STATUS_OBJECT_NAME_NOT_FOUND == status)
    {
        return;
    }

    RegHashGetIterator(gActiveKeyList.pKeyList, &hashIterator);

    for (iCount = 0; (pHashEntry = RegHashNext(&hashIterator)) != NULL; iCount++)
    {
    	if (LwRtlWC16StringIsEqual((PCWSTR)pHashEntry->pKey, pwszKeyName, FALSE))
    	{
    		RegHashRemoveKey(gActiveKeyList.pKeyList, pHashEntry->pKey);

            break;
        }
    }

    return;
}

void
SqliteCacheResetParentKeySubKeyInfo(
    IN PCWSTR pwszParentKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey(pwszParentKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        SqliteResetSubKeyInfo(pKeyResult);
    }

    SqliteReleaseKeyContext(pKeyResult);

    return;
}

void
SqliteCacheResetParentKeySubKeyInfo_inlock(
    IN PCWSTR pwszParentKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey_inlock(pwszParentKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        SqliteResetSubKeyInfo(pKeyResult);
    }

    SqliteReleaseKeyContext_inlock(pKeyResult);

    return;
}

void
SqliteCacheResetKeyValueInfo(
    IN PCWSTR pwszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey(pwszKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        SqliteResetValueInfo(pKeyResult);
    }

    SqliteReleaseKeyContext(pKeyResult);

    return;
}

void
SqliteCacheResetKeyValueInfo_inlock(
    IN PCWSTR pwszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey_inlock(pwszKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        SqliteResetValueInfo(pKeyResult);
    }

    SqliteReleaseKeyContext_inlock(pKeyResult);

    return;
}

void
SqliteSafeFreeKeyHandle(
	PREG_KEY_HANDLE pKeyHandle
	)
{
	if (!pKeyHandle)
	{
		return;
	}

	SqliteReleaseKeyContext(pKeyHandle->pKey);
	memset(pKeyHandle, 0, sizeof(*pKeyHandle));

	LWREG_SAFE_FREE_MEMORY(pKeyHandle);
}

void
SqliteSafeFreeKeyHandle_inlock(
	PREG_KEY_HANDLE pKeyHandle
	)
{
	if (!pKeyHandle)
	{
		return;
	}

	SqliteReleaseKeyContext_inlock(pKeyHandle->pKey);
	memset(pKeyHandle, 0, sizeof(*pKeyHandle));

	LWREG_SAFE_FREE_MEMORY(pKeyHandle);
}


VOID
SqliteReleaseKeyContext(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    SqliteReleaseKeyContext_inlock(pKeyResult);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);
}

VOID
SqliteReleaseKeyContext_inlock(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    if (pKeyResult && InterlockedDecrement(&pKeyResult->refCount) == 0)
    {
        SqliteCacheDeleteActiveKey_inlock(pKeyResult->pwszKeyName);
    }
}

void
SqliteCacheFreeKeyCtxHashEntry(
    IN const REG_HASH_ENTRY* pEntry
    )
{
	PREG_KEY_CONTEXT pKeyResult = (PREG_KEY_CONTEXT)pEntry->pValue;

    if (pKeyResult)
    {
    	SqliteSafeFreeKeyContext(pKeyResult);
    }
}

// sqlite caching helper functions supporting paging
NTSTATUS
SqliteCacheSubKeysInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumSubKeys = 0;
    size_t sNumCacheSubKeys = 0;
    PREG_DB_KEY* ppRegEntries = NULL;

    if (pKeyResult->bHasSubKeyInfo)
    {
        goto cleanup;
    }

    status = RegDbQueryInfoKeyCount(ghCacheConnection,
    		                        pKeyResult->qwId,
                                    QuerySubKeys,
                                    &sNumSubKeys);
    BAIL_ON_NT_STATUS(status);

    sNumCacheSubKeys = (sNumSubKeys > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumSubKeys;

    status = RegDbQueryInfoKey(ghCacheConnection,
    		                   pKeyResult->pwszKeyName,
    		                   pKeyResult->qwId,
                               sNumCacheSubKeys,
                               0,
                               &sNumCacheSubKeys,
                               &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    status = RegDbSafeRecordSubKeysInfo_inlock(
                            sNumSubKeys,
                            sNumCacheSubKeys,
                            ppRegEntries,
                            pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    RegDbSafeFreeEntryKeyList(sNumCacheSubKeys,&ppRegEntries);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheSubKeysInfo_inlock_inDblock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumSubKeys = 0;
    size_t sNumCacheSubKeys = 0;
    PREG_DB_KEY* ppRegEntries = NULL;

    if (pKeyResult->bHasSubKeyInfo)
    {
        goto cleanup;
    }

    status = RegDbQueryInfoKeyCount_inlock(ghCacheConnection,
    		                        pKeyResult->qwId,
                                    QuerySubKeys,
                                    &sNumSubKeys);
    BAIL_ON_NT_STATUS(status);

    sNumCacheSubKeys = (sNumSubKeys > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumSubKeys;

    status = RegDbQueryInfoKey_inlock(ghCacheConnection,
    		                   pKeyResult->pwszKeyName,
    		                   pKeyResult->qwId,
                               sNumCacheSubKeys,
                               0,
                               &sNumCacheSubKeys,
                               &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    status = RegDbSafeRecordSubKeysInfo_inlock(
                            sNumSubKeys,
                            sNumCacheSubKeys,
                            ppRegEntries,
                            pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    RegDbSafeFreeEntryKeyList(sNumCacheSubKeys,&ppRegEntries);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheSubKeysInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheSubKeysInfo_inlock(pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    NTSTATUS status = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor = NULL;
    ULONG ulSecDescRelLen = 0;

    if (pKeyResult->bHasSdInfo)
    	goto cleanup;

    status = RegDbGetKeyAclByKeyId(ghCacheConnection,
			                       pKeyResult->qwId,
			                       &pKeyResult->qwSdId,
			                       &pSecurityDescriptor,
	   	                           &ulSecDescRelLen);
	BAIL_ON_NT_STATUS(status);

    status = SqliteSetKeySecurityDescriptor_inlock(pKeyResult, 
                                                   pSecurityDescriptor,
                                                   ulSecDescRelLen);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pSecurityDescriptor);

    return status;

error:
    pKeyResult->bHasSdInfo = FALSE;

    goto cleanup;
}

NTSTATUS
SqliteCacheKeySecurityDescriptor(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheKeySecurityDescriptor_inlock(pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateSubKeysInfo_inlock(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumSubKeys
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumSubKeys = 0;
    PREG_DB_KEY* ppRegEntries = NULL;
    int iCount = 0;
    size_t sSubKeyLen = 0;

    status = RegDbQueryInfoKey(ghCacheConnection,
    		                   pKeyResult->pwszKeyName,
    		                   pKeyResult->qwId,
                               dwDefaultCacheSize,
                               dwOffSet,
                               &sNumSubKeys,
                               &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (int)sNumSubKeys; iCount++)
    {
		if (ppRegEntries[iCount]->pwszKeyName)
		{
			sSubKeyLen = RtlWC16StringNumChars(ppRegEntries[iCount]->pwszKeyName);
		}

		if (pKeyResult->sMaxSubKeyLen < sSubKeyLen)
		{
			pKeyResult->sMaxSubKeyLen = sSubKeyLen;
		}

        sSubKeyLen = 0;
    }

cleanup:
    *psNumSubKeys = sNumSubKeys;

    RegDbSafeFreeEntryKeyList(sNumSubKeys, &ppRegEntries);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateSubKeysInfo(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumSubKeys
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheUpdateSubKeysInfo_inlock(dwOffSet,
                                             pKeyResult,
                                             psNumSubKeys);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheKeyValuesInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumValues = 0;
    size_t sNumCacheValues = 0;
    PREG_DB_VALUE* ppRegEntries = NULL;

    if (pKeyResult->bHasValueInfo)
    {
        goto cleanup;
    }

    status = RegDbQueryInfoKeyCount(ghCacheConnection,
    		                        pKeyResult->qwId,
                                    QueryValues,
                                    &sNumValues);
    BAIL_ON_NT_STATUS(status);

    sNumCacheValues = (sNumValues > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumValues;

    status = RegDbQueryInfoKeyValue(ghCacheConnection,
    		                        pKeyResult->qwId,
                                    sNumCacheValues,
                                    0,
                                    &sNumCacheValues,
                                    &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    status = RegDbSafeRecordValuesInfo_inlock(
                            sNumValues,
                            sNumCacheValues,
                            ppRegEntries,
                            pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    RegDbSafeFreeEntryValueList(sNumCacheValues, &ppRegEntries);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheKeyValuesInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheKeyValuesInfo_inlock(pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateValuesInfo_inlock(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumValues = 0;
    PREG_DB_VALUE* ppRegEntries = NULL;
    int iCount = 0;
    size_t sValueNameLen = 0;
    DWORD dwValueLen = 0;


    status = RegDbQueryInfoKeyValue(ghCacheConnection,
    		                        pKeyResult->qwId,
                                    dwDefaultCacheSize,
                                    dwOffSet,
                                    &sNumValues,
                                    &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (int)sNumValues; iCount++)
    {

		if (ppRegEntries[iCount]->pwszValueName)
		{
			sValueNameLen = RtlWC16StringNumChars(ppRegEntries[iCount]->pwszValueName);
		}

		if (pKeyResult->sMaxValueNameLen < sValueNameLen)
			pKeyResult->sMaxValueNameLen = sValueNameLen;

        status = RegCopyValueBytes(ppRegEntries[iCount]->pValue,
        		                   ppRegEntries[iCount]->dwValueLen,
                                   NULL,
                                   &dwValueLen);
        BAIL_ON_NT_STATUS(status);

		if (pKeyResult->sMaxValueLen < (size_t)dwValueLen)
			pKeyResult->sMaxValueLen = (size_t)dwValueLen;

        sValueNameLen = 0;
        dwValueLen = 0;
    }

cleanup:
    *psNumValues = sNumValues;
    RegDbSafeFreeEntryValueList(sNumValues, &ppRegEntries);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateValuesInfo(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheUpdateValuesInfo_inlock(dwOffSet,
                                                pKeyResult,
                                                psNumValues);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

// Sqlite DB Key Index and ACL Index mapping cache
void
SqliteCacheFreeDbKeyHashEntry(
    IN const REG_HASH_ENTRY* pEntry
    )
{
	PREG_DB_KEY pRegKey = (PREG_DB_KEY)pEntry->pValue;

    if (pRegKey)
    {
    	RegDbSafeFreeEntryKey(&pRegKey);
    }
}

NTSTATUS
SqliteCacheGetDbKeyInfo(
    IN PCWSTR pwszKeyName,
    OUT PREG_DB_KEY* ppRegKey
    )
{
	NTSTATUS status = 0;
    BOOLEAN bInLock = FALSE;
    //Do not free
    PREG_DB_KEY pRegKeyRef = NULL;
    PREG_DB_KEY pRegKey = NULL;

    LWREG_LOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    status = RegHashGetValue(gRegDbKeyList.pKeyList,
    		                 pwszKeyName,
                             (PVOID*)&pRegKeyRef);
    BAIL_ON_NT_STATUS(status);

    status = RegDbDuplicateDbKeyEntry(pRegKeyRef, &pRegKey);
    BAIL_ON_NT_STATUS(status);

    *ppRegKey = pRegKey;

cleanup:

    LWREG_UNLOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    return status;

error:

    *ppRegKey = NULL;
    RegDbSafeFreeEntryKey(&pRegKey);

    goto cleanup;
}

NTSTATUS
SqliteCacheInsertDbKeyInfo(
    IN PREG_DB_KEY pRegKey
    )
{
    NTSTATUS status = 0;
	BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    status = SqliteCacheInsertDbKeyInfo_inlock(pRegKey);
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWREG_UNLOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    return status;

error:

    goto cleanup;
}

NTSTATUS
SqliteCacheInsertDbKeyInfo_inlock(
    IN PREG_DB_KEY pRegKey
    )
{
	NTSTATUS status = STATUS_SUCCESS;

	BAIL_ON_NT_INVALID_POINTER(pRegKey);

    status = RegHashSetValue(gRegDbKeyList.pKeyList,
                             (PVOID)pRegKey->pwszFullKeyName,
                             (PVOID)pRegKey);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:
    goto cleanup;
}

VOID
SqliteCacheDeleteDbKeyInfo(
    IN PCWSTR pwszKeyName
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    SqliteCacheDeleteDbKeyInfo_inlock(pwszKeyName);

    LWREG_UNLOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    return;
}

VOID
SqliteCacheDeleteDbKeyInfo_inlock(
    IN PCWSTR pwszKeyName
    )
{
	NTSTATUS status = 0;
	REG_HASH_ITERATOR hashIterator;
    REG_HASH_ENTRY* pHashEntry = NULL;
    PREG_DB_KEY pRegKey = NULL;
    int iCount = 0;

    status = RegHashGetValue(gRegDbKeyList.pKeyList,
    		                 (PCVOID)pwszKeyName,
    		                 (PVOID*)&pRegKey);
    if (STATUS_OBJECT_NAME_NOT_FOUND == status)
    {
        return;
    }

    RegHashGetIterator(gRegDbKeyList.pKeyList, &hashIterator);

    for (iCount = 0; (pHashEntry = RegHashNext(&hashIterator)) != NULL; iCount++)
    {
    	if (LwRtlWC16StringIsEqual((PCWSTR)pHashEntry->pKey, pwszKeyName, FALSE))
    	{
    		RegHashRemoveKey(gRegDbKeyList.pKeyList, pHashEntry->pKey);

            break;
        }
    }

    return;
}

VOID
SqliteReleaseDbKeyInfo(
    PREG_DB_KEY pRegKey
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    SqliteReleaseDbKeyInfo_inlock(pRegKey);

    LWREG_UNLOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);
}

VOID
SqliteReleaseDbKeyInfo_inlock(
    PREG_DB_KEY pRegKey
    )
{
    if (pRegKey)
    {
    	SqliteCacheDeleteDbKeyInfo_inlock(pRegKey->pwszFullKeyName);
    }
}


NTSTATUS
SqliteCacheKeyDefaultValuesInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t sNumDefaultValues = 0;
    size_t sNumCacheDefaultValues = 0;
    PREG_DB_VALUE_ATTRIBUTES* ppRegEntries = NULL;

    if (pKeyResult->bHasDefaultValueInfo)
    {
        goto cleanup;
    }

    status = RegDbQueryDefaultValuesCount(ghCacheConnection,
                                    pKeyResult->qwId,
                                    &sNumDefaultValues);
    BAIL_ON_NT_STATUS(status);

    sNumCacheDefaultValues = (sNumDefaultValues > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumDefaultValues;

    status = RegDbQueryDefaultValues(ghCacheConnection,
                                    pKeyResult->qwId,
                                    sNumCacheDefaultValues,
                                    0,
                                    &sNumCacheDefaultValues,
                                    &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    status = RegDbSafeRecordDefaultValuesInfo_inlock(
                            sNumDefaultValues,
                            sNumCacheDefaultValues,
                            ppRegEntries,
                            pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    RegDbSafeFreeEntryValueAttributesList(sNumCacheDefaultValues,
                                          &ppRegEntries);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheKeyDefaultValuesInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheKeyDefaultValuesInfo_inlock(pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateDefaultValuesInfo_inlock(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t sNumValues = 0;
    PREG_DB_VALUE_ATTRIBUTES* ppRegEntries = NULL;
    int iCount = 0;
    size_t sValueNameLen = 0;
    DWORD dwValueLen = 0;


    status = RegDbQueryDefaultValues(ghCacheConnection,
                                    pKeyResult->qwId,
                                    dwDefaultCacheSize,
                                    dwOffSet,
                                    &sNumValues,
                                    &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (int)sNumValues; iCount++)
    {
        if (!ppRegEntries[iCount]->pValueAttributes)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        if (ppRegEntries[iCount]->pwszValueName)
        {
            sValueNameLen = RtlWC16StringNumChars(ppRegEntries[iCount]->pwszValueName);
        }

        if (pKeyResult->sMaxValueNameLen < sValueNameLen)
            pKeyResult->sMaxValueNameLen = sValueNameLen;

        status = RegCopyValueBytes(ppRegEntries[iCount]->pValueAttributes->pDefaultValue,
                                   ppRegEntries[iCount]->pValueAttributes->DefaultValueLen,
                                   NULL,
                                   &dwValueLen);
        BAIL_ON_NT_STATUS(status);

        if (pKeyResult->sMaxValueLen < (size_t)dwValueLen)
            pKeyResult->sMaxValueLen = (size_t)dwValueLen;

        sValueNameLen = 0;
        dwValueLen = 0;
    }

cleanup:
    *psNumValues = sNumValues;
    RegDbSafeFreeEntryValueAttributesList(sNumValues,
                                          &ppRegEntries);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateDefaultValuesInfo(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheUpdateDefaultValuesInfo_inlock(dwOffSet,
                                                       pKeyResult,
                                                       psNumValues);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}
