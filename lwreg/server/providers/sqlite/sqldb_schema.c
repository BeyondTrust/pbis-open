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
 *        sqldb_schema.c
 *
 * Abstract:
 *
 *        Sqlite3 backend for Registry Database Interface
 *        with value attributes schema information
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#include "includes.h"

#define INTEGER_RANGE_DELIMITOR_S "|"
#define INTEGER_RANGE_DELIMITOR_C '|'


static
NTSTATUS
RegDbConvertValueAttributesRangeToBinary(
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes,
    OUT PBYTE* ppRange,
    OUT PDWORD pdwRangeLength
    );

static
NTSTATUS
RegDbConvertBinaryToValueAttributesRange(
    IN PBYTE pRange,
    IN DWORD dwRangeLength,
    IN OUT PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );



NTSTATUS
RegDbUnpackRegValueAttributesInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_DB_VALUE_ATTRIBUTES pResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pRange = NULL;
    DWORD dwRangeLength = 0;

    status = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "ParentId",
        &pResult->qwParentId);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadWC16String(
        pstQuery,
        piColumnPos,
        "ValueName",
        &pResult->pwszValueName);
    BAIL_ON_NT_STATUS(status);

    if (!pResult->pwszValueName)
    {
        status = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Type",
        &pResult->pValueAttributes->ValueType);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadBlob(
        pstQuery,
        piColumnPos,
        "DefaultValue",
        (PBYTE*)&pResult->pValueAttributes->pDefaultValue,
        &pResult->pValueAttributes->DefaultValueLen);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadWC16String(
        pstQuery,
        piColumnPos,
        "Document",
        &pResult->pValueAttributes->pwszDocString);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "RangeType",
        &pResult->pValueAttributes->RangeType);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Hint",
        &pResult->pValueAttributes->Hint);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadBlob(
        pstQuery,
        piColumnPos,
        "Range",
        &pRange,
        &dwRangeLength);
    BAIL_ON_NT_STATUS(status);

    // Convert pRange to pResult->pValueAttributes->Range union
    status = RegDbConvertBinaryToValueAttributesRange(
                  pRange,
                  dwRangeLength,
                  pResult->pValueAttributes);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadTimeT(
        pstQuery,
        piColumnPos,
        "LastUpdated",
        &pResult->tLastUpdated);
    BAIL_ON_NT_STATUS(status);

cleanup:
    RTL_FREE(&pRange);

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbUnpackDefaultValuesCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "DefaultvalueCount",
        pdwCount);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbStoreRegValueAttributes(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    NTSTATUS status = 0;
    sqlite3_stmt *pstQueryEntry = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDB;
    int iColumnPos = 1;
    PREG_DB_VALUE_ATTRIBUTES pEntry = NULL;
    DWORD dwIndex = 0;
    PSTR pszError = NULL;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;
    BOOLEAN bInLock = FALSE;
    PBYTE pRange = NULL;
    DWORD dwRangeLength = 0;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
                    pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
        pEntry = ppValueAttributes[dwIndex];

        if (pEntry == NULL)
        {
            continue;
        }

        pstQueryEntry = pConn->pstCreateRegValueAttributes;

        if (!bGotNow)
        {
            status = RegGetCurrentTimeSeconds(&now);
            BAIL_ON_NT_STATUS(status);

            bGotNow = TRUE;
        }

        iColumnPos = 1;

        status = sqlite3_reset(pstQueryEntry);
        BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

        status = RegSqliteBindInt64(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->qwParentId);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindStringW(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pwszValueName);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt32(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pValueAttributes->ValueType);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindBlob(
                   pstQueryEntry,
                   iColumnPos,
                   pEntry->pValueAttributes->pDefaultValue,
                   pEntry->pValueAttributes->DefaultValueLen);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindStringW(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pValueAttributes->pwszDocString);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt32(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pValueAttributes->RangeType);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt32(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pValueAttributes->Hint);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        // Convert pEntry->pValueAttributes->Range to pRange
        status = RegDbConvertValueAttributesRangeToBinary(
                        pEntry->pValueAttributes,
                        &pRange,
                        &dwRangeLength);
        BAIL_ON_NT_STATUS(status);

        status = RegSqliteBindBlob(
                   pstQueryEntry,
                   iColumnPos,
                   pRange,
                   dwRangeLength);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt64(
                    pstQueryEntry,
                    iColumnPos,
                    now);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = (DWORD)sqlite3_step(pstQueryEntry);
        if (status == SQLITE_DONE)
        {
            status = 0;
        }
        BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

        RTL_FREE(&pRange);
    }

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreERegValues() finished");

    status = sqlite3_reset(pstQueryEntry);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    RTL_FREE(&pRange);

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
RegDbCreateValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_VALUE_ATTRIBUTES pRegEntry = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_VALUE_ATTRIBUTES, sizeof(*pRegEntry));
    BAIL_ON_NT_STATUS(status);

    memset(pRegEntry, 0, sizeof(*pRegEntry));

    status = LwRtlWC16StringDuplicate(&pRegEntry->pwszValueName, pwszValueName);
    BAIL_ON_NT_STATUS(status);

    pRegEntry->pValueAttributes = pValueAttributes;

    pRegEntry->qwParentId = qwParentKeyId;

    status = RegDbStoreRegValueAttributes(
                 hDb,
                 1,
                 &pRegEntry);
    BAIL_ON_NT_STATUS(status);
    pRegEntry->pValueAttributes = NULL;

cleanup:
    RegDbSafeFreeEntryValueAttributes(&pRegEntry);

    return status;

error:

    goto cleanup;
}

NTSTATUS
RegDbUpdateRegValueAttributes(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    NTSTATUS status = 0;
    sqlite3_stmt *pstQueryEntry = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDB;
    int iColumnPos = 1;
    PREG_DB_VALUE_ATTRIBUTES pEntry = NULL;
    DWORD dwIndex = 0;
    PSTR pszError = NULL;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;
    BOOLEAN bInLock = FALSE;
    PBYTE pRange = NULL;
    DWORD dwRangeLength = 0;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
                    pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
        pEntry = ppValueAttributes[dwIndex];

        if (pEntry == NULL)
        {
            continue;
        }

        pstQueryEntry = pConn->pstUpdateValueAttributes;

        if (!bGotNow)
        {
            status = RegGetCurrentTimeSeconds(&now);
            BAIL_ON_NT_STATUS(status);

            bGotNow = TRUE;
        }

        iColumnPos = 1;

        status = sqlite3_reset(pstQueryEntry);
        BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

        status = RegSqliteBindInt64(
                    pstQueryEntry,
                    iColumnPos,
                    now);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindBlob(
                   pstQueryEntry,
                   iColumnPos,
                   pEntry->pValueAttributes->pDefaultValue,
                   pEntry->pValueAttributes->DefaultValueLen);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindStringW(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pValueAttributes->pwszDocString);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt32(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pValueAttributes->RangeType);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt32(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pValueAttributes->Hint);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        // Convert pEntry->pValueAttributes->Range to pRange
        status = RegDbConvertValueAttributesRangeToBinary(
                                       pEntry->pValueAttributes,
                                       &pRange,
                                       &dwRangeLength);
        BAIL_ON_NT_STATUS(status);

        status = RegSqliteBindBlob(
                   pstQueryEntry,
                   iColumnPos,
                   pRange,
                   (DWORD)dwRangeLength);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt64(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->qwParentId);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindStringW(
                    pstQueryEntry,
                    iColumnPos,
                    pEntry->pwszValueName);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = (DWORD)sqlite3_step(pstQueryEntry);
        if (status == SQLITE_DONE)
        {
            status = 0;
        }
        BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);
    }

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreEntries() finished");

    status = sqlite3_reset(pstQueryEntry);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    RTL_FREE(&pRange);

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
RegDbSetValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bIsWrongType = FALSE;
    PREG_DB_VALUE_ATTRIBUTES pRegEntry = NULL;

    status = RegDbGetValueAttributes(hDb,
                                     qwParentKeyId,
                                     pwszValueName,
                                     pValueAttributes->ValueType,
                                     &bIsWrongType,
                                     &pRegEntry);
    if (!status)
    {
        RegSafeFreeValueAttributes(&pRegEntry->pValueAttributes);
        pRegEntry->pValueAttributes = pValueAttributes;

        status = RegDbUpdateRegValueAttributes(
                     hDb,
                     1,
                     &pRegEntry);
        BAIL_ON_NT_STATUS(status);
        pRegEntry->pValueAttributes = NULL;
    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND == status && !pRegEntry)
    {
        status = RegDbCreateValueAttributes(hDb,
                                            qwParentKeyId,
                                            pwszValueName,
                                            pValueAttributes);
        BAIL_ON_NT_STATUS(status);
    }
    BAIL_ON_NT_STATUS(status);

cleanup:
    RegDbSafeFreeEntryValueAttributes(&pRegEntry);

    return status;

error:

    goto cleanup;
}

NTSTATUS
RegDbGetValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PSTR pszError = NULL;
    BOOLEAN bInLock = FALSE;


    BAIL_ON_NT_INVALID_STRING(pwszValueName);

    if (qwParentKeyId <= 0)
    {
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
                    pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    status = RegDbGetValueAttributes_inlock(
                                     hDb,
                                     qwParentKeyId,
                                     pwszValueName,
                                     valueType,
                                     pbIsWrongType,
                                     ppRegEntry);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbGetValueAttributes() finished");


cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:
    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(
             pConn->pDb,
            "rollback",
             NULL,
             NULL,
             NULL);

    goto cleanup;
}

NTSTATUS
RegDbDeleteValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteValueAttributes;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegSqliteBindInt64(pstQuery, 1, qwParentKeyId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindStringW(pstQuery, 2, pwszValueName);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_step(pstQuery);
    if (status == SQLITE_DONE)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:

    if (pstQuery)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

NTSTATUS
RegDbGetValueAttributes_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 9;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_DB_VALUE_ATTRIBUTES pRegEntry = NULL;


    BAIL_ON_NT_INVALID_STRING(pwszValueName);

    if (qwParentKeyId <= 0)
    {
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    if (valueType == REG_NONE)
    {
        pstQuery = pConn->pstQueryValueAttributes;

        status = (NTSTATUS)RegSqliteBindStringW(pstQuery, 1, pwszValueName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = (NTSTATUS)RegSqliteBindInt64(pstQuery, 2, qwParentKeyId);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);
    }
    else
    {
        if (pbIsWrongType && !*pbIsWrongType)
        {
            pstQuery = pConn->pstQueryValueAttributesWithType;
        }
        else
        {
            pstQuery = pConn->pstQueryValueAttributesWithWrongType;
        }

        status = RegSqliteBindStringW(pstQuery, 1, pwszValueName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = RegSqliteBindInt64(pstQuery, 2, qwParentKeyId);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = RegSqliteBindInt32(pstQuery, 3, (int)valueType);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);
    }

    while ((status = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            status = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        if (sResultCount >= 1)
        {
            // Duplicate key value records are found
            status = STATUS_DUPLICATE_NAME;
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry,
                                 REG_DB_VALUE_ATTRIBUTES,
                                 sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry->pValueAttributes,
                                  LWREG_VALUE_ATTRIBUTES,
                                  sizeof(*(pRegEntry->pValueAttributes)));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackRegValueAttributesInfo(pstQuery,
                                         &iColumnPos,
                                         pRegEntry);
        BAIL_ON_NT_STATUS(status);

        sResultCount++;
    }

    if (status == SQLITE_DONE)
    {
        // No more results found
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    if (!sResultCount)
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (!status && ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }
    else
    {
        RegDbSafeFreeEntryValueAttributes(&pRegEntry);
        if (ppRegEntry)
            *ppRegEntry = NULL;
    }

    return status;

error:

    goto cleanup;
}

NTSTATUS
RegDbQueryDefaultValuesCount(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwKeyId,
    OUT size_t* psCount
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PSTR pszError = NULL;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
                    pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    status = RegDbQueryDefaultValuesCount_inlock(
                                 hDb,
                                 qwKeyId,
                                 psCount);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbQueryDefaultValuesCount() finished");

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

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
RegDbQueryDefaultValuesCount_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwKeyId,
    OUT size_t* psCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 1;
    int iColumnPos = 0;
    int nGotColumns = 0;
    DWORD dwCount = 0;


    pstQuery = pConn->pstQueryDefaultValuesCount;

    status = RegSqliteBindInt64(pstQuery, 1, qwKeyId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 2, qwKeyId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    while ((status = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            status = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        if (sResultCount >= 1)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        iColumnPos = 0;

        status = RegDbUnpackDefaultValuesCountInfo(pstQuery,
                                              &iColumnPos,
                                              &dwCount);
        BAIL_ON_NT_STATUS(status);

        sResultCount++;
    }

    if (status == SQLITE_DONE)
    {
        // No more results found
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    if (!sResultCount)
    {
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    *psCount = (size_t)dwCount;

cleanup:

    return status;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    *psCount = 0;

    goto cleanup;
}

NTSTATUS
RegDbQueryDefaultValues(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES** pppRegEntries
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PSTR pszError = NULL;
    BOOLEAN bInLock = FALSE;

    if (qwId <= 0)
    {
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
                    pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    status = RegDbQueryDefaultValues_inlock(
                                     hDb,
                                     qwId,
                                     dwLimit,
                                     dwOffset,
                                     psCount,
                                     pppRegEntries);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbQueryDefaultValues() finished");

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

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
RegDbQueryDefaultValues_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES** pppRegEntries
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    size_t sResultCapacity = 0;
    const int nExpectedCols = 9;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_DB_VALUE_ATTRIBUTES pRegEntry = NULL;
    PREG_DB_VALUE_ATTRIBUTES* ppRegEntries = NULL;


    pstQuery = pConn->pstQueryDefaultValues;

    status = RegSqliteBindInt64(pstQuery, 1, qwId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 2, qwId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 3, dwLimit);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 4, dwOffset);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    while ((status = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            status = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        if (sResultCount >= sResultCapacity)
        {
            sResultCapacity *= 2;
            sResultCapacity += 10;
            status = NtRegReallocMemory(
                            ppRegEntries,
                            (PVOID*)&ppRegEntries,
                            sizeof(*ppRegEntries) * sResultCapacity);
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_VALUE_ATTRIBUTES, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry->pValueAttributes,
                                  LWREG_VALUE_ATTRIBUTES,
                                  sizeof(*(pRegEntry->pValueAttributes)));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackRegValueAttributesInfo(pstQuery,
                                         &iColumnPos,
                                         pRegEntry);
        BAIL_ON_NT_STATUS(status);

        ppRegEntries[sResultCount] = pRegEntry;
        pRegEntry = NULL;
        sResultCount++;
    }

    if (status == SQLITE_DONE)
    {
        // No more results found
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

cleanup:
    if (!status)
    {
        if (pppRegEntries)
        {
            *pppRegEntries = ppRegEntries;
        }
        *psCount = sResultCount;
    }
    else
    {
        RegDbSafeFreeEntryValueAttributes(&pRegEntry);
        RegDbSafeFreeEntryValueAttributesList(sResultCount, &ppRegEntries);
        if (pppRegEntries)
        {
            *pppRegEntries = NULL;
        }
        *psCount = 0;
    }

    return status;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

void
RegDbSafeFreeEntryValueAttributes(
    PREG_DB_VALUE_ATTRIBUTES* ppEntry
    )
{
    PREG_DB_VALUE_ATTRIBUTES pEntry = NULL;
    if (ppEntry != NULL && *ppEntry != NULL)
    {
        pEntry = *ppEntry;

        LWREG_SAFE_FREE_MEMORY(pEntry->pwszValueName);
        RegSafeFreeValueAttributes(&pEntry->pValueAttributes);

        memset(pEntry, 0, sizeof(*pEntry));

        LWREG_SAFE_FREE_MEMORY(pEntry);
        *ppEntry = NULL;
    }
}

void
RegDbSafeFreeEntryValueAttributesList(
    size_t sCount,
    PREG_DB_VALUE_ATTRIBUTES** pppEntries
    )
{
    if (*pppEntries != NULL)
    {
        size_t iEntry;
        for (iEntry = 0; iEntry < sCount; iEntry++)
        {
            RegDbSafeFreeEntryValueAttributes(&(*pppEntries)[iEntry]);
        }
        LWREG_SAFE_FREE_MEMORY(*pppEntries);
    }
}

static
NTSTATUS
RegDbConvertValueAttributesRangeToBinary(
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes,
    OUT PBYTE* ppRange,
    OUT PDWORD pdwRangeLength
    )
{
    NTSTATUS status = 0;
    PBYTE pRange = NULL;
    DWORD dwRangeLength = 0;

    if (!pValueAttributes)
    {
        goto cleanup;
    }

    if (LWREG_VALUE_RANGE_TYPE_ENUM == pValueAttributes->RangeType)
    {
        // pValueAttributes->Range.ppwszRangeEnumStrings
        if (!pValueAttributes->Range.ppwszRangeEnumStrings)
            goto cleanup;

        status = NtRegMultiStrsToByteArrayW(pValueAttributes->Range.ppwszRangeEnumStrings,
                                            &pRange,
                                            (SSIZE_T*)&dwRangeLength);
        BAIL_ON_NT_STATUS(status);
    }
    else if (LWREG_VALUE_RANGE_TYPE_INTEGER == pValueAttributes->RangeType)
    {
        // pValueAttributes->Range.RangeInteger
        status = LwRtlWC16StringAllocatePrintfW((PWSTR*)&pRange,
                                                L"%d%c%d",
                                                pValueAttributes->Range.RangeInteger.Max,
                                                INTEGER_RANGE_DELIMITOR_C,
                                                pValueAttributes->Range.RangeInteger.Min);
        BAIL_ON_NT_STATUS(status);

        dwRangeLength = (LwRtlWC16StringNumChars((PWSTR)pRange) + 1)*sizeof(wchar16_t);
    }

cleanup:
    if (status)
    {
        LWREG_SAFE_FREE_MEMORY(pRange);
        dwRangeLength = 0;
    }

    *ppRange = pRange;
    *pdwRangeLength = dwRangeLength;

    return status;

error:
    goto cleanup;

}

static
NTSTATUS
RegDbConvertBinaryToValueAttributesRange(
    IN PBYTE pRange,
    IN DWORD dwRangeLength,
    IN OUT PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    NTSTATUS status = 0;
    PSTR pszRangeInteger = NULL;
    // Do not free
    PSTR pszTmp = NULL;
    PSTR pszstrtok_rSav = NULL;

    if (!pRange || !dwRangeLength || !pValueAttributes)
    {
        goto cleanup;
    }

    if (LWREG_VALUE_RANGE_TYPE_ENUM == pValueAttributes->RangeType)
    {
        // pValueAttributes->Range.ppwszRangeEnumStrings
        status = NtRegByteArrayToMultiStrsW(pRange,
                                            (SSIZE_T)dwRangeLength,
                                            &pValueAttributes->Range.ppwszRangeEnumStrings);
        BAIL_ON_NT_STATUS(status);
    }
    else if (LWREG_VALUE_RANGE_TYPE_INTEGER == pValueAttributes->RangeType)
    {
        // pValueAttributes->Range.RangeInteger
        status = LwRtlCStringAllocateFromWC16String(&pszRangeInteger,
                                                    (PWSTR)pRange);
        BAIL_ON_NT_STATUS(status);

        // MAX|MIN
        pszTmp = strtok_r(pszRangeInteger, INTEGER_RANGE_DELIMITOR_S, &pszstrtok_rSav);
        if (pszTmp != NULL)
        {
            pValueAttributes->Range.RangeInteger.Max = pszTmp ? atoi(pszTmp) : 0;
            pszTmp = strtok_r(NULL, INTEGER_RANGE_DELIMITOR_S, &pszstrtok_rSav);
            pValueAttributes->Range.RangeInteger.Min = pszTmp ? atoi(pszTmp) : 0;
        }
    }

 cleanup:
    LWREG_SAFE_FREE_MEMORY(pszRangeInteger);

    return status;

error:
    goto cleanup;
}
