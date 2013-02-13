#include "includes.h"
// sqlite provider specific helpers

// Key Context (key handle) helper utility functions
void
SqliteSafeFreeKeyContext(
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

void
SqliteResetSubKeyInfo(
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

NTSTATUS
SqliteGetKeySecurityDescriptor_inlock(
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
SqliteSetKeySecurityDescriptor_inlock(
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

void
SqliteResetValueInfo(
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
