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
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "includes.h"

NTSTATUS
SqliteGetKeyToken(
    PCWSTR pwszInputString,
    wchar16_t c,
    PWSTR *ppwszOutputString
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    // Do not free
    PCWSTR pwszFound = NULL;
    PWSTR pwszOutputString = NULL;

    BAIL_ON_NT_INVALID_STRING(pwszInputString);

    pwszFound = RegStrchr(pwszInputString, c);
    if (pwszFound)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pwszOutputString, wchar16_t,
        		                  sizeof(*pwszOutputString)* (pwszFound - pwszInputString +1));
        BAIL_ON_NT_STATUS(status);

        memcpy(pwszOutputString, pwszInputString,(pwszFound - pwszInputString) * sizeof(*pwszOutputString));
    }

    *ppwszOutputString = pwszOutputString;

cleanup:

    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pwszOutputString);

    goto cleanup;
}

NTSTATUS
SqliteGetParentKeyName(
    PCWSTR pwszInputString,
    wchar16_t c,
    PWSTR *ppwszOutputString
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    // Do not free
    PCWSTR pwszFound = NULL;
    PWSTR pwszOutputString = NULL;

    BAIL_ON_NT_INVALID_STRING(pwszInputString);

    pwszFound = RegStrrchr(pwszInputString, c);
    if (pwszFound)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pwszOutputString, wchar16_t,
        		                  sizeof(*pwszOutputString)* (pwszFound - pwszInputString +1));
        BAIL_ON_NT_STATUS(status);

        memcpy(pwszOutputString, pwszInputString,(pwszFound - pwszInputString) * sizeof(*pwszOutputString));
    }

    *ppwszOutputString = pwszOutputString;

cleanup:

    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pwszOutputString);

    goto cleanup;
}

NTSTATUS
SqliteCreateKeyHandle(
    IN PACCESS_TOKEN pToken,
    IN ACCESS_MASK AccessDesired,
    IN PREG_KEY_CONTEXT pKey,
    OUT PREG_KEY_HANDLE* ppKeyHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_HANDLE pKeyHandle = NULL;
    ACCESS_MASK AccessGranted = 0;

    BAIL_ON_INVALID_KEY_CONTEXT(pKey);

    status = RegSrvAccessCheckKey(pToken,
                                  pKey->pSecurityDescriptor,
                                  pKey->ulSecDescLength,
                                  AccessDesired,
                                  &AccessGranted);
    if (STATUS_NO_TOKEN == status)
    {
        status = 0;
        AccessGranted = 0;
    }
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyHandle, REG_KEY_HANDLE, sizeof(*pKeyHandle));
    BAIL_ON_NT_STATUS(status);

    pKeyHandle->AccessGranted = AccessGranted;
    pKeyHandle->pKey = pKey;

    *ppKeyHandle = pKeyHandle;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pKeyHandle);

    goto cleanup;
}

NTSTATUS
SqliteCreateKeyContext(
    IN PREG_DB_KEY pRegEntry,
    OUT PREG_KEY_CONTEXT* ppKeyResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKeyResult = NULL;

    BAIL_ON_INVALID_REG_ENTRY(pRegEntry);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult, REG_KEY_CONTEXT, sizeof(*pKeyResult));
    BAIL_ON_NT_STATUS(status);

    pKeyResult->refCount = 1;

    pthread_rwlock_init(&pKeyResult->mutex, NULL);
    pKeyResult->pMutex = &pKeyResult->mutex;

    status = LwRtlWC16StringDuplicate(&pKeyResult->pwszKeyName, pRegEntry->pwszFullKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteGetParentKeyName(pKeyResult->pwszKeyName, (wchar16_t)'\\',&pKeyResult->pwszParentKeyName);
    BAIL_ON_NT_STATUS(status);

    pKeyResult->qwId = pRegEntry->version.qwDbId;

    pKeyResult->qwSdId = pRegEntry->qwAclIndex;

    // Cache ACL
    if (pRegEntry->ulSecDescLength)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pSecurityDescriptor, VOID, pRegEntry->ulSecDescLength);
        BAIL_ON_NT_STATUS(status);

        memcpy(pKeyResult->pSecurityDescriptor, pRegEntry->pSecDescRel, pRegEntry->ulSecDescLength);
        pKeyResult->ulSecDescLength = pRegEntry->ulSecDescLength;
        pKeyResult->bHasSdInfo = TRUE;
    }

    *ppKeyResult = pKeyResult;

cleanup:
    return status;

error:
    SqliteSafeFreeKeyContext(pKeyResult);
    *ppKeyResult = NULL;

    goto cleanup;
}

/* Create a new key, if the key exists already,
 * open the existing key
 */
NTSTATUS
SqliteCreateKeyInternal(
    IN OPTIONAL HANDLE handle,
    IN PREG_KEY_CONTEXT pParentKeyCtx,
    IN PWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_KEY pRegEntry = NULL;
    PREG_KEY_HANDLE pKeyHandle = NULL;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    BOOLEAN bInLock = FALSE;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)handle;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelToSet = NULL;
    ULONG ulSecDescLengthToSet = 0;
    DWORD dwDisposition = 0;

    // Full key path
    BAIL_ON_NT_INVALID_STRING(pwszFullKeyName);

    // when starting up lwregd pServerState is NULL and
    // creating root key can skip ACL check
    if (pServerState && !pServerState->pToken)
    {
        status = RegSrvCreateAccessToken(pServerState->peerUID,
                                         pServerState->peerGID,
                                         &pServerState->pToken);
        BAIL_ON_NT_STATUS(status);
    }

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

	status = SqliteOpenKeyInternal_inlock(
			        handle,
			        pwszFullKeyName, // Full Key Path
	                AccessDesired,
	                &pKeyHandle);
	if (!status)
	{
		dwDisposition = REG_OPENED_EXISTING_KEY;

		goto done;
	}
	else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
	{
		status = 0;
	}
	BAIL_ON_NT_STATUS(status);

	// Root Key has to be created with a given SD
	if (!pParentKeyCtx && !pSecDescRel)
	{
		status = STATUS_INTERNAL_ERROR;
		BAIL_ON_NT_STATUS(status);
	}

	// ACL check
	// Get key's security descriptor
	// Inherit from its direct parent or given by caller
	if (!pSecDescRel || !ulSecDescLength)
	{
		BAIL_ON_INVALID_KEY_CONTEXT(pParentKeyCtx);

		status = SqliteCacheKeySecurityDescriptor(pParentKeyCtx);
		BAIL_ON_NT_STATUS(status);

		pSecDescRelToSet = pParentKeyCtx->pSecurityDescriptor;
		ulSecDescLengthToSet = pParentKeyCtx->ulSecDescLength;
	}
	else
	{
		pSecDescRelToSet = pSecDescRel;
		ulSecDescLengthToSet = ulSecDescLength;
	}

	// Make sure SD has at least owner information
	if (!RtlValidRelativeSecurityDescriptor(pSecDescRelToSet,
			                                ulSecDescLengthToSet,
			                                OWNER_SECURITY_INFORMATION))
	{
		status = STATUS_INVALID_SECURITY_DESCR;
		BAIL_ON_NT_STATUS(status);
	}

	// Create key with SD
	status = RegDbCreateKey(ghCacheConnection,
							pwszFullKeyName,
							pSecDescRelToSet,
							ulSecDescLengthToSet,
							&pRegEntry);
	BAIL_ON_NT_STATUS(status);

    if (pParentKeyCtx)
	{
	    SqliteCacheResetParentKeySubKeyInfo_inlock(pParentKeyCtx->pwszKeyName);
	}

	status = SqliteCreateKeyContext(pRegEntry, &pKeyCtx);
	BAIL_ON_NT_STATUS(status);

	// Cache this new key in gActiveKeyList
	status = SqliteCacheInsertActiveKey_inlock(pKeyCtx);
	BAIL_ON_NT_STATUS(status);

	status = SqliteCreateKeyHandle(pServerState ? pServerState->pToken : NULL,
	                               AccessDesired,
	                               pKeyCtx,
	                               &pKeyHandle);
	BAIL_ON_NT_STATUS(status);
	pKeyCtx = NULL;

	dwDisposition = REG_CREATED_NEW_KEY;

done:
    if (ppKeyHandle)
	{
		*ppKeyHandle = pKeyHandle;
	}
	else
	{
		SqliteSafeFreeKeyHandle_inlock(pKeyHandle);
	}

    if (pdwDisposition)
    {
    	*pdwDisposition =  dwDisposition;
    }

cleanup:

    SqliteReleaseKeyContext_inlock(pKeyCtx);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    RegDbSafeFreeEntryKey(&pRegEntry);

    return status;

error:

    if (ppKeyHandle)
    {
	    *ppKeyHandle = NULL;
    }

    SqliteSafeFreeKeyHandle_inlock(pKeyHandle);

    goto cleanup;
}

NTSTATUS
SqliteOpenKeyInternal(
	IN HANDLE handle,
    IN PCWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)handle;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_STRING(pwszFullKeyName);

    BAIL_ON_NT_INVALID_POINTER(handle);

    if (!pServerState->pToken)
    {
        status = RegSrvCreateAccessToken(pServerState->peerUID,
                                         pServerState->peerGID,
                                         &pServerState->pToken);
        BAIL_ON_NT_STATUS(status);
    }

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    // pServerState->pToken should be created at this point
    status = SqliteOpenKeyInternal_inlock(handle,
    		                              pwszFullKeyName,
    		                              AccessDesired,
    		                              ppKeyHandle);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return status;

error:
    goto cleanup;
}

/* Open a key, if not found,
 * do not create a new key */
NTSTATUS
SqliteOpenKeyInternal_inlock(
	IN OPTIONAL HANDLE handle,
    IN PCWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)handle;
    PREG_DB_KEY pRegEntry = NULL;
    PREG_KEY_HANDLE pKeyHandle = NULL;
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    BAIL_ON_NT_INVALID_STRING(pwszFullKeyName);

    pKeyCtx = SqliteCacheLocateActiveKey_inlock(pwszFullKeyName);
    if (!pKeyCtx)
    {
        status = RegDbOpenKey(ghCacheConnection, pwszFullKeyName, &pRegEntry);
        BAIL_ON_NT_STATUS(status);

        status = SqliteCreateKeyContext(pRegEntry, &pKeyCtx);
        BAIL_ON_NT_STATUS(status);

        // Cache this new key in gActiveKeyList
        status = SqliteCacheInsertActiveKey_inlock(pKeyCtx);
        BAIL_ON_NT_STATUS(status);
    }

    status = SqliteCreateKeyHandle(pServerState ? pServerState->pToken : NULL,
                                   AccessDesired,
                                   pKeyCtx,
                                   &pKeyHandle);
	BAIL_ON_NT_STATUS(status);
	pKeyCtx = NULL;

    *ppKeyHandle = pKeyHandle;

cleanup:
    SqliteReleaseKeyContext_inlock(pKeyCtx);
    RegDbSafeFreeEntryKey(&pRegEntry);

    return status;

error:
    SqliteSafeFreeKeyHandle_inlock(pKeyHandle);
    *ppKeyHandle = NULL;

    goto cleanup;
}

NTSTATUS
SqliteOpenKeyInternal_inlock_inDblock(
    IN OPTIONAL HANDLE handle,
    IN PCWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)handle;
    PREG_DB_KEY pRegEntry = NULL;
    PREG_KEY_HANDLE pKeyHandle = NULL;
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    BAIL_ON_NT_INVALID_STRING(pwszFullKeyName);

    pKeyCtx = SqliteCacheLocateActiveKey_inlock(pwszFullKeyName);
    if (!pKeyCtx)
    {
        status = RegDbOpenKey_inlock(ghCacheConnection, pwszFullKeyName, &pRegEntry);
        BAIL_ON_NT_STATUS(status);

        status = SqliteCreateKeyContext(pRegEntry, &pKeyCtx);
        BAIL_ON_NT_STATUS(status);

        // Cache this new key in gActiveKeyList
        status = SqliteCacheInsertActiveKey_inlock(pKeyCtx);
        BAIL_ON_NT_STATUS(status);
    }

    status = SqliteCreateKeyHandle(pServerState ? pServerState->pToken : NULL,
                                   AccessDesired,
                                   pKeyCtx,
                                   &pKeyHandle);
    BAIL_ON_NT_STATUS(status);
    pKeyCtx = NULL;

    *ppKeyHandle = pKeyHandle;

cleanup:
    SqliteReleaseKeyContext_inlock(pKeyCtx);
    RegDbSafeFreeEntryKey(&pRegEntry);

    return status;

error:
    SqliteSafeFreeKeyHandle_inlock(pKeyHandle);
    *ppKeyHandle = NULL;

    goto cleanup;
}

VOID
SqliteCloseKey_inlock(
    IN HKEY hKey
    )
{
    SqliteSafeFreeKeyHandle_inlock((PREG_KEY_HANDLE)hKey);
}

NTSTATUS
SqliteDeleteKeyInternal_inlock(
	IN HANDLE handle,
    IN PCWSTR pwszKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszParentKeyName = NULL;
    PREG_KEY_HANDLE pKeyHandle = NULL;
    // Do not free
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    status = SqliteDeleteActiveKey_inlock((PCWSTR)pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteOpenKeyInternal_inlock(handle,
    		                              pwszKeyName,
    		                              0,
                                          &pKeyHandle);
    BAIL_ON_NT_STATUS(status);

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    // Delete all the values of this key
    status = RegDbDeleteKey(ghCacheConnection, pKeyCtx->qwId, pKeyCtx->qwSdId, pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteGetParentKeyName(pwszKeyName, '\\',&pwszParentKeyName);
    BAIL_ON_NT_STATUS(status);

    if (!LW_IS_NULL_OR_EMPTY_STR(pwszParentKeyName))
    {
        SqliteCacheResetParentKeySubKeyInfo_inlock(pwszParentKeyName);
    }

cleanup:
    SqliteSafeFreeKeyHandle_inlock(pKeyHandle);

    LWREG_SAFE_FREE_MEMORY(pwszParentKeyName);

    return status;

error:
    goto cleanup;
}

// This can be called when a DB lock and active key list lock are both held
NTSTATUS
SqliteDeleteKeyInternal_inlock_inDblock(
	IN HANDLE handle,
    IN PCWSTR pwszKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszParentKeyName = NULL;
    PREG_KEY_HANDLE pKeyHandle = NULL;
    // Do not free
    PREG_KEY_CONTEXT pKeyCtx = NULL;

    status = SqliteDeleteActiveKey_inlock((PCWSTR)pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteOpenKeyInternal_inlock_inDblock(handle,
    		                                pwszKeyName,
    		                                0,
                                            &pKeyHandle);
    BAIL_ON_NT_STATUS(status);

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    // Delete all the values of this key
    status = RegDbDeleteKey_inlock(ghCacheConnection, pKeyCtx->qwId, pKeyCtx->qwSdId, pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteGetParentKeyName(pwszKeyName, '\\',&pwszParentKeyName);
    BAIL_ON_NT_STATUS(status);

    if (!LW_IS_NULL_OR_EMPTY_STR(pwszParentKeyName))
    {
        SqliteCacheResetParentKeySubKeyInfo_inlock(pwszParentKeyName);
    }

cleanup:
    SqliteSafeFreeKeyHandle_inlock(pKeyHandle);

    LWREG_SAFE_FREE_MEMORY(pwszParentKeyName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteDeleteActiveKey(
    IN PCWSTR pwszKeyName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    status = SqliteDeleteActiveKey_inlock(pwszKeyName);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteDeleteActiveKey_inlock(
    IN PCWSTR pwszKeyName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pFoundKey = NULL;

    pFoundKey = SqliteCacheLocateActiveKey_inlock(pwszKeyName);
    if (pFoundKey)
    {
        status = STATUS_RESOURCE_IN_USE;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    SqliteReleaseKeyContext_inlock(pFoundKey);

    return status;

error:
    goto cleanup;
}
