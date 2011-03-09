#include "includes.h"
// sqlite provider specific helpers

// Key Context (key handle) helper utility functions
void
RegSrvSafeFreeKeyContext(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    if (pKeyResult != NULL)
    {
        if (pKeyResult->pMutex)
        {
            pthread_rwlock_destroy(&pKeyResult->mutex);
        }

        LWREG_SAFE_FREE_MEMORY(pKeyResult->pwszKeyName);
        LWREG_SAFE_FREE_MEMORY(pKeyResult->pwszParentKeyName);
        RegFreeWC16StringArray(pKeyResult->ppwszSubKeyNames, pKeyResult->dwNumCacheSubKeys);

        RegFreeWC16StringArray(pKeyResult->ppwszValueNames, pKeyResult->dwNumCacheValues);
        RegFreeValueByteArray(pKeyResult->ppValues, pKeyResult->dwNumCacheValues);
        LWREG_SAFE_FREE_MEMORY(pKeyResult->pdwValueLen);
        LWREG_SAFE_FREE_MEMORY(pKeyResult->pTypes);

        RegFreeWC16StringArray(pKeyResult->ppwszDefaultValueNames, pKeyResult->dwNumCacheDefaultValues);
        RegFreeValueByteArray(pKeyResult->ppDefaultValues, pKeyResult->dwNumCacheDefaultValues);
        LWREG_SAFE_FREE_MEMORY(pKeyResult->pdwDefaultValueLen);
        LWREG_SAFE_FREE_MEMORY(pKeyResult->pDefaultTypes);

        LWREG_SAFE_FREE_MEMORY(pKeyResult->pSecurityDescriptor);

        memset(pKeyResult, 0, sizeof(*pKeyResult));
        LWREG_SAFE_FREE_MEMORY(pKeyResult);
    }
}

DWORD
RegSrvGetKeyRefCount(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD refCount = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    refCount = pKeyResult->refCount;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return refCount;
}

void
RegSrvResetSubKeyInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    pKeyResult->bHasSubKeyInfo = FALSE;

    RegFreeWC16StringArray(pKeyResult->ppwszSubKeyNames, pKeyResult->dwNumCacheSubKeys);
    pKeyResult->ppwszSubKeyNames = NULL;

    pKeyResult->dwNumCacheSubKeys = 0;
    pKeyResult->dwNumSubKeys = 0;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return;
}

BOOLEAN
RegSrvHasSubKeyInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bHasSubKeyInfo = FALSE;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    bHasSubKeyInfo = pKeyResult->bHasSubKeyInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return bHasSubKeyInfo;
}

DWORD
RegSrvSubKeyNum(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD dwSubKeyCount = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    dwSubKeyCount = pKeyResult->dwNumSubKeys;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwSubKeyCount;
}

size_t
RegSrvSubKeyNameMaxLen(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    size_t sSubKeyNameMaxLen = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    sSubKeyNameMaxLen = pKeyResult->sMaxSubKeyLen;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return sSubKeyNameMaxLen;
}

PCWSTR
RegSrvSubKeyName(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN DWORD dwIndex
    )
{
    BOOLEAN bInLock = FALSE;
    PCWSTR pwszSubKeyName = NULL;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    pwszSubKeyName = pKeyResult->ppwszSubKeyNames[dwIndex];

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return pwszSubKeyName;
}

BOOLEAN
RegSrvHasSecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bHasSdInfo = FALSE;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    bHasSdInfo = pKeyResult->bHasSdInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return bHasSdInfo;
}

ULONG
RegSrvGetKeySecurityDescriptorSize(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    ULONG ulSecDescRelLen = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    ulSecDescRelLen = pKeyResult->ulSecDescLength;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return ulSecDescRelLen;
}

NTSTATUS
RegSrvGetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    )
{
    NTSTATUS status = 0;

    if (ulSecDescRelLen < pKeyResult->ulSecDescLength)
    {
    	status = STATUS_BUFFER_TOO_SMALL;
    	BAIL_ON_NT_STATUS(status);
    }

    memcpy(pSecurityDescriptor, pKeyResult->pSecurityDescriptor, pKeyResult->ulSecDescLength);

cleanup:

    return status;

error:
    goto cleanup;
}


NTSTATUS
RegSrvGetKeySecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    )
{
    BOOLEAN bInLock = FALSE;
    NTSTATUS status = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    status = RegSrvGetKeySecurityDescriptor_inlock(pKeyResult, pSecurityDescriptor, ulSecDescRelLen);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegSrvSetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    )
{
    NTSTATUS status = 0;

    LWREG_SAFE_FREE_MEMORY(pKeyResult->pSecurityDescriptor);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pSecurityDescriptor, VOID, ulSecDescRelLen);
    BAIL_ON_NT_STATUS(status);

    memcpy(pKeyResult->pSecurityDescriptor, pSecurityDescriptor, ulSecDescRelLen);

    pKeyResult->ulSecDescLength = ulSecDescRelLen;

    pKeyResult->bHasSdInfo = TRUE;

cleanup:

    return status;

error:
    pKeyResult->bHasSdInfo = FALSE;

    goto cleanup;
}


NTSTATUS
RegSrvSetKeySecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    )
{
    NTSTATUS status = 0;
	BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegSrvSetKeySecurityDescriptor_inlock(pKeyResult, pSecurityDescriptor,ulSecDescRelLen);
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    pKeyResult->bHasSdInfo = FALSE;

    goto cleanup;
}

void
RegSrvResetValueInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    pKeyResult->bHasValueInfo = FALSE;

    RegFreeWC16StringArray(pKeyResult->ppwszValueNames, pKeyResult->dwNumCacheValues);
    RegFreeValueByteArray(pKeyResult->ppValues, pKeyResult->dwNumCacheValues);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pdwValueLen);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pTypes);

    pKeyResult->ppwszValueNames = NULL;
    pKeyResult->ppValues = NULL;
    pKeyResult->dwNumCacheValues = 0;
    pKeyResult->dwNumValues = 0;

    pKeyResult->bHasDefaultValueInfo = FALSE;

    RegFreeWC16StringArray(pKeyResult->ppwszDefaultValueNames, pKeyResult->dwNumCacheDefaultValues);
    RegFreeValueByteArray(pKeyResult->ppDefaultValues, pKeyResult->dwNumCacheDefaultValues);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pdwDefaultValueLen);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pDefaultTypes);

    pKeyResult->ppwszDefaultValueNames = NULL;
    pKeyResult->ppDefaultValues = NULL;
    pKeyResult->dwNumCacheDefaultValues = 0;
    pKeyResult->dwNumCacheValues = 0;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);
}

BOOLEAN
RegSrvHasValueInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bHasValueInfo = FALSE;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    bHasValueInfo = pKeyResult->bHasValueInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return bHasValueInfo;
}

BOOLEAN
RegSrvHasDefaultValueInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bHasDefaultValueInfo = FALSE;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    bHasDefaultValueInfo = pKeyResult->bHasDefaultValueInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return bHasDefaultValueInfo;
}


DWORD
RegSrvValueNum(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD dwValueCount = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    dwValueCount = pKeyResult->dwNumValues;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwValueCount;
}

DWORD
RegSrvDefaultValueNum(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD dwValueCount = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    dwValueCount = pKeyResult->dwNumDefaultValues;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwValueCount;
}

size_t
RegSrvMaxValueNameLen(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    size_t sMaxValueNameLen = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    sMaxValueNameLen = pKeyResult->sMaxValueNameLen;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return sMaxValueNameLen;
}

size_t
RegSrvMaxValueLen(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    size_t sMaxValueLen = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    sMaxValueLen = pKeyResult->sMaxValueLen;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return sMaxValueLen;
}
