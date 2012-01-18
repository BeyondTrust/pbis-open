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
 *        regsqlite.c
 *
 * Abstract:
 *
 *        Wrapper functions for sqlite
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */
#include "includes.h"

NTSTATUS
RegSqliteBindString(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN PCSTR pszValue
    )
{
    return (NTSTATUS)sqlite3_bind_text(pstQuery, Index, pszValue,
                             -1, SQLITE_TRANSIENT);
}

NTSTATUS
RegSqliteBindStringW(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN PCWSTR pwszValue
    )
{
    return sqlite3_bind_text16(pstQuery, Index, pwszValue,
                             -1, SQLITE_STATIC);
}

NTSTATUS
RegSqliteBindInt64(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int64_t Value
    )
{
    return (NTSTATUS)sqlite3_bind_int64(pstQuery, Index, (sqlite_int64)Value);
}

NTSTATUS
RegSqliteBindInt32(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int Value
    )
{
    return (NTSTATUS)sqlite3_bind_int(pstQuery, Index, Value);
}

NTSTATUS
RegSqliteBindBlob(
	IN OUT sqlite3_stmt* pstQuery,
	IN int Index,
	IN BYTE* Value,
	IN DWORD dwValueLen
	)
{
    return (NTSTATUS)sqlite3_bind_blob(pstQuery, Index,
    		                           (const void *)Value, (int)dwValueLen, SQLITE_STATIC);
}

NTSTATUS
RegSqliteBindBoolean(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN BOOLEAN bValue
    )
{
    return (NTSTATUS)sqlite3_bind_int(pstQuery, Index, bValue ? 1 : 0);
}

NTSTATUS
RegSqliteReadInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int64_t *pqwResult)
{
	NTSTATUS status = STATUS_SUCCESS;
    //Do not free
    PSTR pszEndPtr = NULL;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }
#endif

    *pqwResult = strtoll(pszColumnValue, &pszEndPtr, 10);
    if (pszEndPtr == NULL || pszEndPtr == pszColumnValue || *pszEndPtr != '\0')
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    (*piColumnPos)++;

error:
    return status;
}

NTSTATUS
RegSqliteReadString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PSTR *ppszResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }
#endif

    status = RegStrDupOrNull(
            pszColumnValue,
            ppszResult);
    BAIL_ON_NT_STATUS(status);

    (*piColumnPos)++;

cleanup:
    return status;

error:
    *ppszResult = NULL;

    goto cleanup;
}

NTSTATUS
RegSqliteReadWC16String(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PWSTR *ppwszResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    //Do not free
    PCWSTR pwszColumnValue = (PCWSTR)sqlite3_column_text16(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }
#endif

    status = RegWcStrDupOrNull(
    		pwszColumnValue,
    		ppwszResult);
    BAIL_ON_NT_STATUS(status);

    (*piColumnPos)++;

cleanup:
    return status;

error:
    *ppwszResult = NULL;

    goto cleanup;
}

NTSTATUS
RegSqliteReadTimeT(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    time_t *pResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint64_t qwTemp;

    status = RegSqliteReadUInt64(
        pstQuery,
        piColumnPos,
        name,
        &qwTemp);
    BAIL_ON_NT_STATUS(status);

    *pResult = qwTemp;

error:
    return status;
}

NTSTATUS
RegSqliteReadUInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    uint64_t *pqwResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    //Do not free
    PSTR pszEndPtr = NULL;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }
#endif

    BAIL_ON_NT_INVALID_STRING(pszColumnValue);
    *pqwResult = strtoull(pszColumnValue, &pszEndPtr, 10);
    if (pszEndPtr == NULL || pszEndPtr == pszColumnValue || *pszEndPtr != '\0')
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    (*piColumnPos)++;

error:
    return status;
}

NTSTATUS
RegSqliteReadInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int *piResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int64_t qwTemp;
    int iColumnPos = *piColumnPos;

    status = RegSqliteReadInt64(
        pstQuery,
        &iColumnPos,
        name,
        &qwTemp);
    BAIL_ON_NT_STATUS(status);

    if (qwTemp > INT_MAX || qwTemp < INT_MIN)
    {
        status = STATUS_RANGE_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    *piResult = (int)qwTemp;
    *piColumnPos = iColumnPos;

error:
    return status;
}

NTSTATUS
RegSqliteReadUInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    DWORD *pdwResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint64_t qwTemp;
    int iColumnPos = *piColumnPos;

    status = RegSqliteReadUInt64(
        pstQuery,
        &iColumnPos,
        name,
        &qwTemp);
    BAIL_ON_NT_STATUS(status);

    if (qwTemp > UINT_MAX)
    {
        status = STATUS_RANGE_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    *pdwResult = qwTemp;
    *piColumnPos = iColumnPos;

error:
    return status;
}

NTSTATUS
RegSqliteReadBlob(
	sqlite3_stmt *pstQuery,
	int *piColumnPos,
	PCSTR name,
	PBYTE* ppValue,
	PDWORD pdwValueLen
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	int iColumnPos = *piColumnPos;
    DWORD dwValueLen = 0;
    PBYTE pValue = NULL;
    //Do not free
    PBYTE pColumnValue = (PBYTE)sqlite3_column_blob(pstQuery, iColumnPos);

    dwValueLen = (DWORD)sqlite3_column_bytes(pstQuery, iColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }
#endif

    if (dwValueLen)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pValue, BYTE, sizeof(*pValue)*dwValueLen);
        BAIL_ON_NT_STATUS(status);

        memcpy(pValue, pColumnValue, dwValueLen);
    }

    *ppValue = pValue;
    *pdwValueLen = dwValueLen;

    (*piColumnPos)++;

cleanup:
    return status;

error:
    *ppValue = NULL;
    *pdwValueLen = 0;

    LWREG_SAFE_FREE_MEMORY(pValue);
    dwValueLen = 0;

    goto cleanup;
}

NTSTATUS
RegSqliteExecCallbackWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PFN_REG_SQLITE_EXEC_CALLBACK pfnCallback,
    IN PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszError = NULL;
    DWORD dwRetry;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(pLock, bInLock);

    for (dwRetry = 0; dwRetry < 20; dwRetry++)
    {
        status = pfnCallback(pDb, pContext, &pszError);
        if (status == SQLITE_BUSY)
        {
            SQLITE3_SAFE_FREE_STRING(pszError);
            status = 0;
            // Rollback the half completed transaction
            RegSqliteExec(pDb, "ROLLBACK", NULL);

            REG_LOG_ERROR("There is a conflict trying to access the "
                          "cache database.  This would happen if another "
                          "process is trying to access it.  Retrying...");
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(status, pszError);
            break;
        }
    }
    BAIL_ON_SQLITE3_ERROR(status, pszError);

error:
    LEAVE_SQLITE_LOCK(pLock, bInLock);
    SQLITE3_SAFE_FREE_STRING(pszError);

    return status;
}

NTSTATUS
RegSqliteBasicCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    )
{
    PCSTR pszTransaction = (PCSTR)pContext;

    return RegSqliteExec(pDb, pszTransaction, ppszError);
}

NTSTATUS
RegSqliteExecWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PCSTR pszTransaction
    )
{
    return RegSqliteExecCallbackWithRetry(
                pDb,
                pLock,
                RegSqliteBasicCallback,
                (PVOID)pszTransaction);
}

NTSTATUS
RegSqliteExec(
    IN sqlite3* pSqlDatabase,
    IN PCSTR pszSqlCommand,
    OUT PSTR* ppszSqlError
    )
{
    return (NTSTATUS)sqlite3_exec(pSqlDatabase, pszSqlCommand,
                        NULL, NULL, ppszSqlError);
}




#if 0
NTSTATUS
RegSqliteReadBoolean(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    BOOLEAN *pbResult)
{
	NTSTATUS status = STATUS_SUCCESS;
    DWORD dwTemp;

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        name,
        &dwTemp);
    BAIL_ON_NT_STATUS(status);

    *pbResult = (dwTemp != 0);

error:
    return status;
}

NTSTATUS
RegSqliteReadStringInPlace(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSTR pszResult,
    //Includes NULL
    IN size_t sMaxSize)
{
	NTSTATUS status = STATUS_SUCCESS;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);
    size_t sRequiredSize = 0;

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }
#endif

    sRequiredSize = strlen(pszColumnValue) + 1;
    if (sRequiredSize > sMaxSize)
    {
        status = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pszResult, pszColumnValue, sRequiredSize);

    (*piColumnPos)++;

cleanup:
    return status;

error:
    pszResult[0] = '\0';

    goto cleanup;
}

DWORD
RegSqliteAllocPrintf(
    OUT PSTR* ppszSqlCommand,
    IN PCSTR pszSqlFormat,
    IN ...
    )
{
    DWORD status = 0;
    va_list args;

    va_start(args, pszSqlFormat);
    *ppszSqlCommand = sqlite3_vmprintf(pszSqlFormat, args);
    va_end(args);

    if (!*ppszSqlCommand)
    {
        status = LW_ERROR_OUT_OF_MEMORY;
    }

    return status;
}
#endif
