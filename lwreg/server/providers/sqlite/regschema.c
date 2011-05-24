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
 *        regschema.c
 *
 * Abstract:
 *
 *        Registry Schema Implementation (Value Attributes APIs etc)
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors:
 *          Wei Fu (wfu@likewise.com)
 */
#include "includes.h"

NTSTATUS
SqliteSetValueAttributes(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszValueName = NULL;
    BOOLEAN bIsWrongType = TRUE;
    wchar16_t wszEmptyValueName[] = REG_EMPTY_VALUE_NAME_W;
    PWSTR pwszKeyNameWithSubKey = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    PREG_KEY_HANDLE pKeyHandleInUse = NULL;
    PREG_KEY_CONTEXT pKeyCtxInUse = NULL;
    // Do not free
    PBYTE pData = NULL;
    DWORD cbData = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    if (!RegValidValueAttributes(pValueAttributes))
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    if (pwszSubKey)
    {
        status = LwRtlWC16StringAllocatePrintfW(
                        &pwszKeyNameWithSubKey,
                        L"%ws\\%ws",
                        pKeyCtx->pwszKeyName,
                        pwszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    status = SqliteOpenKeyInternal(hRegConnection,
                                   pwszSubKey ? pwszKeyNameWithSubKey : pKeyCtx->pwszKeyName,
                                   KEY_SET_VALUE,
                                   &pKeyHandleInUse);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvAccessCheckKeyHandle(pKeyHandleInUse, KEY_SET_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtxInUse = pKeyHandleInUse->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtxInUse);

    status = LwRtlWC16StringDuplicate(&pwszValueName, !pValueName ? wszEmptyValueName : pValueName);
    BAIL_ON_NT_STATUS(status);

    status = RegDbGetValueAttributes(
                              ghCacheConnection,
                              pKeyCtxInUse->qwId,
                              (PCWSTR)pwszValueName,
                              (REG_DATA_TYPE)pValueAttributes->ValueType,
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

    pData = pValueAttributes->pDefaultValue;
    cbData = pValueAttributes->DefaultValueLen;

    if (cbData == 0)
    {
        goto done;
    }

    switch (pValueAttributes->ValueType)
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

            if (!pData || pData[cbData-1] != '\0' || pData[cbData-2] != '\0' )
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
    status = RegDbSetValueAttributes(ghCacheConnection,
                                     pKeyCtxInUse->qwId,
                                     pwszValueName,
                                     pValueAttributes);
    BAIL_ON_NT_STATUS(status);

    SqliteCacheResetKeyValueInfo(pKeyCtxInUse->pwszKeyName);

cleanup:

    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);
    SqliteSafeFreeKeyHandle(pKeyHandleInUse);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteGetValueAttributes(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    return SqliteGetValueAttributes_Internal(hRegConnection,
                                             hKey,
                                             pwszSubKey,
                                             pValueName,
                                             REG_NONE,
                                             TRUE,
                                             ppCurrentValue,
                                             ppValueAttributes);
}

NTSTATUS
SqliteGetValueAttributes_Internal(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pValueName,
    IN OPTIONAL REG_DATA_TYPE dwType,
    // Whether bail or not in case there is no value attributes
    IN BOOLEAN bDoBail,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszValueName = NULL;
    PREG_DB_VALUE_ATTRIBUTES pRegEntry = NULL;
    PREG_DB_VALUE pRegValueEntry = NULL;
    BOOLEAN bIsWrongType = FALSE;
    wchar16_t wszEmptyValueName[] = REG_EMPTY_VALUE_NAME_W;
    PWSTR pwszKeyNameWithSubKey = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    PREG_KEY_HANDLE pKeyHandleInUse = NULL;
    PREG_KEY_CONTEXT pKeyCtxInUse = NULL;
    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;
    BOOLEAN bInDbLock = FALSE;
    BOOLEAN bInLock = FALSE;
    PSTR pszError = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)ghCacheConnection;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)hRegConnection;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    if (!ppCurrentValue && !ppValueAttributes)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    if (pwszSubKey)
    {
        status = LwRtlWC16StringAllocatePrintfW(
                        &pwszKeyNameWithSubKey,
                        L"%ws\\%ws",
                        pKeyCtx->pwszKeyName,
                        pwszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

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

    // pServerState->pToken should be created at this point
    status = SqliteOpenKeyInternal_inlock_inDblock(
                              hRegConnection,
                              pwszSubKey ? pwszKeyNameWithSubKey : pKeyCtx->pwszKeyName,
                              KEY_QUERY_VALUE,
                              &pKeyHandleInUse);
    BAIL_ON_NT_STATUS(status);

    // ACL check
    status = RegSrvAccessCheckKeyHandle(pKeyHandleInUse, KEY_QUERY_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtxInUse = pKeyHandleInUse->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtxInUse);

    status = LwRtlWC16StringDuplicate(&pwszValueName, !pValueName ? wszEmptyValueName : pValueName);
    BAIL_ON_NT_STATUS(status);

    // Optionally get value
    if (ppCurrentValue)
    {
        status = RegDbGetKeyValue_inlock(
                                  ghCacheConnection,
                                  pKeyCtxInUse->qwId,
                                  pwszValueName,
                                  dwType,
                                  &bIsWrongType,
                                  &pRegValueEntry);
        if (!status)
        {
            status = LW_RTL_ALLOCATE((PVOID*)&pCurrentValue,
                                      LWREG_CURRENT_VALUEINFO,
                                      sizeof(*pCurrentValue));
            BAIL_ON_NT_STATUS(status);

            pCurrentValue->cbData = pRegValueEntry->dwValueLen;
            pCurrentValue->dwType = pRegValueEntry->type;

            if (pCurrentValue->cbData)
            {
                status = LW_RTL_ALLOCATE((PVOID*)&pCurrentValue->pvData,
                                         VOID,
                                         pCurrentValue->cbData);
                BAIL_ON_NT_STATUS(status);

                memcpy(pCurrentValue->pvData, pRegValueEntry->pValue, pCurrentValue->cbData);
            }
        }
        else if (LW_STATUS_OBJECT_NAME_NOT_FOUND ==  status)
        {
            status = 0;
        }
        BAIL_ON_NT_STATUS(status);
    }

    // Get value attributes
    if (ppValueAttributes)
    {
        status = RegDbGetValueAttributes_inlock(
                                         ghCacheConnection,
                                         pKeyCtxInUse->qwId,
                                         pwszValueName,
                                         REG_NONE,
                                         &bIsWrongType,
                                         &pRegEntry);
        if (!bDoBail && LW_STATUS_OBJECT_NAME_NOT_FOUND == status)
        {
            status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry,
                                     REG_DB_VALUE_ATTRIBUTES,
                                     sizeof(*pRegEntry));
            BAIL_ON_NT_STATUS(status);
        }
        BAIL_ON_NT_STATUS(status);
    }

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c SqliteGetValueAttributes_Internal() finished");

    if (ppCurrentValue)
    {
        *ppCurrentValue = pCurrentValue;
        pCurrentValue = NULL;
    }

    if (ppValueAttributes)
    {
        *ppValueAttributes = pRegEntry->pValueAttributes;
        pRegEntry->pValueAttributes = NULL;
    }

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInDbLock);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    SqliteSafeFreeKeyHandle(pKeyHandleInUse);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);
    RegDbSafeFreeEntryValue(&pRegValueEntry);

    RegSafeFreeCurrentValueInfo(&pCurrentValue);
    RegDbSafeFreeEntryValueAttributes(&pRegEntry);

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

    if (ppCurrentValue)
    {
        *ppCurrentValue = NULL;
    }

    if (ppValueAttributes)
    {
        *ppValueAttributes  = NULL;
    }

    goto cleanup;
}

NTSTATUS
SqliteDeleteValueAttributes(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pValueName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszValueName = NULL;
    wchar16_t wszEmptyValueName[] = REG_EMPTY_VALUE_NAME_W;
    PWSTR pwszKeyNameWithSubKey = NULL;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    PREG_KEY_HANDLE pKeyHandleInUse = NULL;
    PREG_KEY_CONTEXT pKeyCtxInUse = NULL;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);


    if (pwszSubKey)
    {
        status = LwRtlWC16StringAllocatePrintfW(
                        &pwszKeyNameWithSubKey,
                        L"%ws\\%ws",
                        pKeyCtx->pwszKeyName,
                        pwszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    status = SqliteOpenKeyInternal(hRegConnection,
                                   pwszSubKey ? pwszKeyNameWithSubKey : pKeyCtx->pwszKeyName,
                                   KEY_SET_VALUE | DELETE,
                                   &pKeyHandleInUse);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvAccessCheckKeyHandle(pKeyHandleInUse, KEY_SET_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtxInUse = pKeyHandleInUse->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtxInUse);

    status = LwRtlWC16StringDuplicate(&pwszValueName, !pValueName ? wszEmptyValueName : pValueName);
    BAIL_ON_NT_STATUS(status);

    status = RegDbGetValueAttributes(
                              ghCacheConnection,
                              pKeyCtxInUse->qwId,
                              (PCWSTR)pwszValueName,
                              REG_NONE,
                              NULL,
                              NULL);
    BAIL_ON_NT_STATUS(status);

    status = RegDbDeleteValueAttributes(ghCacheConnection,
                                        pKeyCtxInUse->qwId,
                                        (PCWSTR)pwszValueName);
    BAIL_ON_NT_STATUS(status);

    SqliteCacheResetKeyValueInfo(pKeyCtxInUse->pwszKeyName);

cleanup:
    SqliteSafeFreeKeyHandle(pKeyHandleInUse);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);

    return status;

error:
    goto cleanup;

}

// QueryInfoDeftaulValue only returns the values that are in schema and
// not yet set explicitly set by clients
// If we ever want to exclusively browse schema data,
// we need to implement a separate API just for that purpose.
NTSTATUS
SqliteQueryInfoDefaultValues(
    IN REG_DB_HANDLE hDb,
    IN PREG_KEY_CONTEXT pKey,
    OUT OPTIONAL PDWORD pcDefaultValues,
    OUT OPTIONAL PDWORD pcMaxDefaultValueNameLen,
    OUT OPTIONAL PDWORD pcMaxDefaultValueLen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t sCount = 0;
    size_t sGotCount = 0;
    PREG_DB_VALUE_ATTRIBUTES* ppRegEntries = NULL;
    BOOLEAN bInLock = FALSE;
    int iCount = 0;
    DWORD dwMaxDefaultValueNameLen = 0;
    DWORD dwMaxDefaultValueLen = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;


    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegDbQueryDefaultValuesCount_inlock(
                                          hDb,
                                          pKey->qwId,
                                          &sCount);
    BAIL_ON_NT_STATUS(status);

    status = RegDbQueryDefaultValues_inlock(
                                          hDb,
                                          pKey->qwId,
                                          sCount,
                                          0,
                                          &sGotCount,
                                          &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    if (sCount != sGotCount)
    {
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < (DWORD)sGotCount; iCount++)
    {
        DWORD dwValueNameLen = 0;

        dwValueNameLen = (DWORD)RtlWC16StringNumChars(ppRegEntries[iCount]->pwszValueName);

        if (dwMaxDefaultValueNameLen < dwValueNameLen)
            dwMaxDefaultValueNameLen = dwValueNameLen;

        if (dwMaxDefaultValueLen < ppRegEntries[iCount]->pValueAttributes->DefaultValueLen)
            dwMaxDefaultValueLen = ppRegEntries[iCount]->pValueAttributes->DefaultValueLen;
    }

    if (pcDefaultValues)
    {
        *pcDefaultValues = (DWORD)sGotCount;
    }
    if (pcMaxDefaultValueNameLen)
    {
        *pcMaxDefaultValueNameLen = dwMaxDefaultValueNameLen;
    }
    if (pcMaxDefaultValueLen)
    {
        *pcMaxDefaultValueLen = dwMaxDefaultValueLen;
    }

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    RegDbSafeFreeEntryValueAttributesList(sGotCount, &ppRegEntries);

    return status;

error:
    if (pcDefaultValues)
    {
        *pcDefaultValues = 0;
    }
    if (pcMaxDefaultValueNameLen)
    {
        *pcMaxDefaultValueNameLen = 0;
    }
    if (pcMaxDefaultValueLen)
    {
        *pcMaxDefaultValueLen = 0;
    }

    goto cleanup;
}
