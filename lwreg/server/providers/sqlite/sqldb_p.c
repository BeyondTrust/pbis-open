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
 *        sqldb_p.c
 *
 * Abstract:
 *
 *        Sqlite3 backend for Registry Database Interface
 *        APIs inlock
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */
#include "includes.h"

NTSTATUS
RegDbOpenKey_inlock(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyPath,
    OUT OPTIONAL PREG_DB_KEY* ppRegKey
    )
{
    NTSTATUS status = 0;
	wchar16_t wch = '\\';
    PWSTR pwszKeyToken = NULL;
    // Do not free
    PCWSTR pwszCurrentKeyPath = pwszFullKeyPath;
    int64_t qwParentId = 0;
    PREG_DB_KEY pRegKey = NULL;
    PREG_DB_KEY pRegKeyDup = NULL;
    BOOLEAN bInLock = FALSE;

    status = SqliteCacheGetDbKeyInfo(pwszFullKeyPath, &pRegKey);
    if (!status)
    {
    	goto cleanup;
    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
    {
    	status = 0;
    }
    BAIL_ON_NT_STATUS(status);

    while (pwszCurrentKeyPath)
    {
        LWREG_SAFE_FREE_MEMORY(pwszKeyToken);
        RegDbSafeFreeEntryKey(&pRegKey);

    	status = SqliteGetKeyToken(pwszCurrentKeyPath, wch, &pwszKeyToken);
        BAIL_ON_NT_STATUS(status);

        status = RegDbOpenKeyName_inlock(hDb,
        		                  !pwszKeyToken ? pwszCurrentKeyPath : pwszKeyToken,
        		                  &qwParentId,
        		                  &pRegKey);
        BAIL_ON_NT_STATUS(status);

        pwszCurrentKeyPath = RegStrchr(pwszCurrentKeyPath, wch);

        if (pwszCurrentKeyPath)
        {
        	pwszCurrentKeyPath++;
        }
    }

    status = LwRtlWC16StringDuplicate(&pRegKey->pwszFullKeyName, pwszFullKeyPath);
    BAIL_ON_NT_STATUS(status);

    if (pRegKey->qwAclIndex != -1)
    {
    	status = RegDbGetKeyAclByAclIndex_inlock(hDb,
    	    		                             pRegKey->qwAclIndex,
    	    		                             &pRegKey->pSecDescRel,
    	    		                             &pRegKey->ulSecDescLength);
    	BAIL_ON_NT_STATUS(status);
    }

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbOpenKey_inlock() finished");

    status = RegDbDuplicateDbKeyEntry(pRegKey, &pRegKeyDup);
    BAIL_ON_NT_STATUS(status);

    LWREG_LOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

	status = SqliteCacheInsertDbKeyInfo_inlock(pRegKeyDup);
	BAIL_ON_NT_STATUS(status);
	pRegKeyDup = NULL;

cleanup:

    SqliteReleaseDbKeyInfo_inlock(pRegKeyDup);

    LWREG_UNLOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    if (!status && ppRegKey)
    {
    	*ppRegKey = pRegKey;
    }
    else
    {
    	RegDbSafeFreeEntryKey(&pRegKey);
    }

    LWREG_SAFE_FREE_MEMORY(pwszKeyToken);

    return status;

error:

    if(ppRegKey)
    {
    	*ppRegKey = NULL;
    }

    RegDbSafeFreeEntryKey(&pRegKey);

    goto cleanup;
}

NTSTATUS
RegDbOpenKeyName_inlock(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN OUT int64_t* pqwParentId,
    OUT PREG_DB_KEY* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 5;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_DB_KEY pRegEntry = NULL;

    pstQuery = pConn->pstOpenKeyEx;
    status = RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 2, *pqwParentId);
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
            //Duplicate keys are found
        	status = STATUS_DUPLICATE_NAME;
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_KEY, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_NT_STATUS(status);

        status = RegDbUnpackRegKeyInfo(pstQuery,
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

    status = (NTSTATUS)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    if (!sResultCount)
    {
    	status = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    *ppRegEntry = pRegEntry;
    *pqwParentId = pRegEntry->version.qwDbId;

cleanup:

    return status;

error:
    *ppRegEntry = NULL;

    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }
    RegDbSafeFreeEntryKey(&pRegEntry);

    goto cleanup;
}

NTSTATUS
RegDbDeleteKeyWithNoSubKeys_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN int64_t qwAclId,
    IN PCWSTR pwszFullKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PCWSTR pwszKeyName = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    size_t sCount = 0;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKey;


    // Delete the key
    pwszKeyName = RegStrrchr(pwszFullKeyName, '\\');
    pwszKeyName = pwszKeyName ? pwszKeyName + 1 : pwszFullKeyName;

    status = RegSqliteBindInt64(pstQuery, 1, qwId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindStringW(pstQuery, 2, pwszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_step(pstQuery);
    if (status == SQLITE_DONE)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    // Delete the key's values
    pstQuery = pConn->pstDeleteAllKeyValues;

    status = RegSqliteBindInt64(pstQuery, 1, qwId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_step(pstQuery);
    if (status == SQLITE_DONE)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    // Delete the key's values associated attributes
    pstQuery = pConn->pstDeleteAllValueAttributes;

    status = RegSqliteBindInt64(pstQuery, 1, qwId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_step(pstQuery);
    if (status == SQLITE_DONE)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    // Delete the keys' ACL if there is no more reference to it
    if (qwAclId != -1)
    {
        status = RegDbQueryAclRefCountWOCurrKey_inlock(hDb, qwAclId, qwId, &sCount);
        BAIL_ON_NT_STATUS(status);

        if (!sCount)
        {
    	    status = RegDbDeleteAcl_inlock(hDb, qwAclId);
    	    BAIL_ON_NT_STATUS(status);
        }
    }

    status = sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    SqliteCacheDeleteDbKeyInfo(pwszFullKeyName);

cleanup:

    return status;

 error:
    goto cleanup;
}

NTSTATUS
RegDbDeleteAcl_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwSdCacheId
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteAcl;

    status = RegSqliteBindInt64(pstQuery, 1, qwSdCacheId);
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

    return status;

error:

    if (pstQuery)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

NTSTATUS
RegDbQueryInfoKeyCount_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN QueryKeyInfoOption queryType,
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

    if (qwId <= 0)
    {
    	status = STATUS_INTERNAL_ERROR;
    	BAIL_ON_NT_STATUS(status);
   }

    switch (queryType)
    {
        case QuerySubKeys:
            pstQuery = pConn->pstQuerySubKeysCount;
            break;

        case QueryValues:
            pstQuery = pConn->pstQueryValuesCount;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

    status = RegSqliteBindInt64(pstQuery, 1, qwId);
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

        switch (queryType)
        {
            case QuerySubKeys:
                status = RegDbUnpackSubKeysCountInfo(pstQuery,
                                                      &iColumnPos,
                                                      &dwCount);
                BAIL_ON_NT_STATUS(status);

                break;

            case QueryValues:
                status = RegDbUnpackKeyValuesCountInfo(pstQuery,
                                                      &iColumnPos,
                                                      &dwCount);
                BAIL_ON_NT_STATUS(status);

                break;
        }

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
RegDbQueryInfoKey_inlock(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_KEY** pppRegEntries
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    size_t sResultCapacity = 0;
    const int nExpectedCols = 5;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_DB_KEY pRegEntry = NULL;
    PREG_DB_KEY* ppRegEntries = NULL;

    if (qwId <= 0)
    {
    	status = STATUS_INTERNAL_ERROR;
    	BAIL_ON_NT_STATUS(status);
    }

    pstQuery = pConn->pstQuerySubKeys;

    status = RegSqliteBindInt64(pstQuery, 1, qwId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 2, dwLimit);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 3, dwOffset);
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

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_KEY, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_NT_STATUS(status);

        status = RegDbUnpackRegKeyInfo(pstQuery,
                                         &iColumnPos,
                                         pRegEntry);
        BAIL_ON_NT_STATUS(status);

        status = LwRtlWC16StringAllocatePrintfW(
                        &pRegEntry->pwszFullKeyName,
                        L"%ws\\%ws",
                        pwszKeyName,
                        pRegEntry->pwszKeyName);
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
        RegDbSafeFreeEntryKey(&pRegEntry);
        RegDbSafeFreeEntryKeyList(sResultCount, &ppRegEntries);
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

NTSTATUS
RegDbUpdateKeyAclIndex_inlock(
	IN REG_DB_HANDLE hDb,
	IN int64_t qwKeyDbId,
	IN int64_t qwKeySdId
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
	// Do not free
	sqlite3_stmt *pstQuery = pConn->pstUpdateKeyAclIndexByKeyId;

	status = RegSqliteBindInt64(pstQuery, 1, qwKeySdId);
	BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

	status = RegSqliteBindInt64(pstQuery, 2, qwKeyDbId);
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

	return status;

error:

	if (pstQuery)
	{
		sqlite3_reset(pstQuery);
	}

	goto cleanup;
}

NTSTATUS
RegDbQueryAclRefCountWOCurrKey_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwSdId,
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

    if (qwSdId <= 0)
    {
    	status = STATUS_INTERNAL_ERROR;
    	BAIL_ON_NT_STATUS(status);
    }

    pstQuery = pConn->pstQueryAclRefCount;

    status = RegSqliteBindInt64(pstQuery, 1, qwSdId);
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

        status = RegDbUnpackAclrefCountInfo(pstQuery,
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
RegDbGetKeyAclIndexByKeyId_inlock(
	IN REG_DB_HANDLE hDb,
	IN int64_t qwKeyDbId,
	OUT int64_t* pqwAclIndex
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
	size_t sResultCount = 0;
	const int nExpectedCols = 1;
	int iColumnPos = 0;
	int nGotColumns = 0;
	// Do not free
	sqlite3_stmt *pstQuery = pConn->pstQueryKeyAclIndexByKeyId;

	status = RegSqliteBindInt64(
			   pstQuery,
			   1,
			   qwKeyDbId);
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
			//Duplicate ACL records are found
			status = STATUS_DUPLICATE_NAME;
			BAIL_ON_NT_STATUS(status);
		}

		iColumnPos = 0;

		status = RegDbUnpackAclIndexInfoInKeys(pstQuery,
										 &iColumnPos,
										 pqwAclIndex);
		sResultCount++;
	}

	if (status == SQLITE_DONE)
	{
		status = STATUS_SUCCESS;
	}
	BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

	status = (DWORD)sqlite3_reset(pstQuery);
	BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

cleanup:

	return status;

error:

	if (pstQuery)
	{
		sqlite3_reset(pstQuery);
	}

	*pqwAclIndex = -1;

	goto cleanup;
}

NTSTATUS
RegDbGetKeyAclByAclIndex_inlock(
    IN REG_DB_HANDLE hDb,
    // qwAclIndex should not be -1 (pre-check is done before call this function)
    IN int64_t qwAclIndex,
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    OUT PULONG pulSecDescLength
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    int nExpectedCols = 1;
    int iColumnPos = 0;
    int nGotColumns = 0;
    size_t sResultCount = 0;
	PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;

	// Do not free
	sqlite3_stmt *pstQuery = pConn->pstQueryKeyAcl;


	status = RegSqliteBindInt64(pstQuery, 1, qwAclIndex);
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
			//Duplicate ACLs are found
			status = STATUS_DUPLICATE_NAME;
			BAIL_ON_NT_STATUS(status);
		}

		iColumnPos = 0;

		status = RegDbUnpackAclInfo(pstQuery,
				                     &iColumnPos,
				                     ppSecDescRel,
		                             pulSecDescLength);
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
		// This should never happen, we only store valid SD in ACLs
		status = STATUS_INTERNAL_ERROR;
		BAIL_ON_NT_STATUS(status);
	}

cleanup:
    return status;

error:
    *ppSecDescRel = NULL;
    *pulSecDescLength = 0;

    goto cleanup;
}

NTSTATUS
RegDbGetKeyAclIndexByKeyAcl_inlock(
	IN REG_DB_HANDLE hDb,
	IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescLength,
	OUT int64_t* pqwAclIndex
	)
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    size_t sResultCount = 0;
    const int nExpectedCols = 1;
    int iColumnPos = 0;
    int nGotColumns = 0;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstQueryKeyAclIndex;

	status = RegSqliteBindBlob(
			   pstQuery,
			   1,
			   (BYTE*)pSecurityDescriptor,
			   ulSecDescLength);
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
			//Duplicate ACL records are found
			status = STATUS_DUPLICATE_NAME;
			BAIL_ON_NT_STATUS(status);
		}

		iColumnPos = 0;

		status = RegDbUnpackAclIndexInfoInAcls(pstQuery,
				                         &iColumnPos,
				                         pqwAclIndex);
		sResultCount++;
	}

    if (status == SQLITE_DONE)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

cleanup:

    return status;

error:

    if (pstQuery)
    {
        sqlite3_reset(pstQuery);
    }

    *pqwAclIndex = -1;

    goto cleanup;
}

NTSTATUS
RegDbQueryTotalAclCount_inlock(
    IN REG_DB_HANDLE hDb,
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


    pstQuery = pConn->pstQueryTotalAclCount;

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

        status = RegDbUnpackTotalAclCountInfo(pstQuery,
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
RegDbGetKeyAclByAclOffset_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwOffset,
    OUT int64_t* pqwCacheId,
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    OUT PULONG pulSecDescLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int nExpectedCols = 2;
    int iColumnPos = 0;
    int nGotColumns = 0;
    size_t sResultCount = 0;

    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;

    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstQueryAclByOffset;


    status = RegSqliteBindInt64(pstQuery, 1, qwOffset);
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
            //Duplicate ACLs are found
            status = STATUS_DUPLICATE_NAME;
            BAIL_ON_NT_STATUS(status);
        }

        iColumnPos = 0;

        status = RegDbUnpackAclIndexInfoInAcls(pstQuery,
                                               &iColumnPos,
                                               pqwCacheId);
        BAIL_ON_NT_STATUS(status);

        status = RegDbUnpackAclInfo(pstQuery,
                                    &iColumnPos,
                                    ppSecDescRel,
                                    pulSecDescLength);
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
        // This should never happen, we only store valid SD in ACLs
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    return status;

error:
    *pqwCacheId = -1;
    *ppSecDescRel = NULL;
    *pulSecDescLength = 0;

    goto cleanup;
}

NTSTATUS
RegDbGetKeyValue_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 5;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_DB_VALUE pRegEntry = NULL;


    if (valueType == REG_NONE)
    {
        pstQuery = pConn->pstQueryKeyValue;

        status = (NTSTATUS)RegSqliteBindStringW(pstQuery, 1, pwszValueName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = (NTSTATUS)RegSqliteBindInt64(pstQuery, 2, qwParentKeyId);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);
    }
    else
    {
        if (pbIsWrongType && !*pbIsWrongType)
        {
            pstQuery = pConn->pstQueryKeyValueWithType;
        }
        else
        {
            pstQuery = pConn->pstQueryKeyValueWithWrongType;
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

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_VALUE, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackRegValueInfo(pstQuery,
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
        RegDbSafeFreeEntryValue(&pRegEntry);
        if (ppRegEntry)
            *ppRegEntry = NULL;
    }

    return status;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

NTSTATUS
RegDbUpdateRegValues_inlock(
    IN HANDLE hDb,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    sqlite3_stmt *pstQueryEntry = NULL;
    int iColumnPos = 1;
    PREG_DB_VALUE pEntry = NULL;
    DWORD dwIndex = 0;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
        pEntry = ppValues[dwIndex];

        if (pEntry == NULL)
        {
            continue;
        }

        pstQueryEntry = pConn->pstUpdateRegValue;

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
                   pEntry->pValue,
                   pEntry->dwValueLen);
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

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbUpdateRegValues_inlock() finished");

    status = sqlite3_reset(pstQueryEntry);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    return status;

 error:

    goto cleanup;
}

NTSTATUS
RegDbStoreRegKeys_inlock(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_KEY* ppKeys
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDB;
    sqlite3_stmt *pstCreateKey = NULL;
    sqlite3_stmt *pstCreateKeyAcl = NULL;
    int iColumnPos = 1;
    // Do not free pEntry
    PREG_DB_KEY pEntry = NULL;
    DWORD dwIndex = 0;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;

    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
        pEntry = ppKeys[dwIndex];

        if (pEntry == NULL)
        {
            continue;
        }

        // Creating a new key qwDbId should be -1
        if (pEntry->version.qwDbId != -1)
        {
            REG_LOG_DEBUG("Registry::sqldb.c RegDbStoreRegKeys() qwDbId is -1");
            continue;
        }

        status = RegDbGetKeyAclIndexByKeyAcl_inlock(
                                            hDB,
                                            pEntry->pSecDescRel,
                                            pEntry->ulSecDescLength,
                                            &pEntry->qwAclIndex);
        BAIL_ON_NT_STATUS(status);

        if (pEntry->qwAclIndex == -1)
        {
            pstCreateKeyAcl = pConn->pstCreateRegAcl;

            status = RegSqliteBindBlob(
                       pstCreateKeyAcl,
                       1,
                       (BYTE*)pEntry->pSecDescRel,
                       pEntry->ulSecDescLength);
            BAIL_ON_SQLITE3_ERROR_STMT(status, pstCreateKeyAcl);

            status = (DWORD)sqlite3_step(pstCreateKeyAcl);
            if (status == SQLITE_DONE)
            {
                status = 0;
            }
            BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

            pEntry->qwAclIndex = sqlite3_last_insert_rowid(pConn->pDb);

            status = sqlite3_reset(pstCreateKeyAcl);
            BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
        }

        pstCreateKey = pConn->pstCreateRegKey;

        if (!bGotNow)
        {
            status = RegGetCurrentTimeSeconds(&now);
            BAIL_ON_NT_STATUS(status);

            bGotNow = TRUE;
        }

        iColumnPos = 1;

        status = RegSqliteBindInt64(
                    pstCreateKey,
                    iColumnPos,
                    pEntry->qwParentId);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindStringW(
                    pstCreateKey,
                    iColumnPos,
                    pEntry->pwszKeyName);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt64(
                    pstCreateKey,
                    iColumnPos,
                    pEntry->qwAclIndex);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindInt64(
                    pstCreateKey,
                    iColumnPos,
                    now);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = (DWORD)sqlite3_step(pstCreateKey);
        if (status == SQLITE_DONE)
        {
            status = 0;
        }
        BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

        status = sqlite3_reset(pstCreateKey);
        BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    }

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreRegKeys_inlock() finished");

cleanup:

    return status;

 error:

    goto cleanup;
}

NTSTATUS
RegDbStoreRegValues_inlock(
    IN HANDLE hDb,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    sqlite3_stmt *pstQueryEntry = NULL;
    int iColumnPos = 1;
    PREG_DB_VALUE pEntry = NULL;
    DWORD dwIndex = 0;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
        pEntry = ppValues[dwIndex];

        if (pEntry == NULL)
        {
            continue;
        }

        pstQueryEntry = pConn->pstCreateRegValue;

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
                    pEntry->type);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

        status = RegSqliteBindBlob(
                   pstQueryEntry,
                   iColumnPos,
                   pEntry->pValue,
                   pEntry->dwValueLen);
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
    }

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreRegValues_inlock() finished");

    status = sqlite3_reset(pstQueryEntry);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    return status;

 error:

    goto cleanup;
}

NTSTATUS
RegDbCreateKeyValue_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_VALUE pRegEntry = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_VALUE, sizeof(*pRegEntry));
    BAIL_ON_NT_STATUS(status);

    memset(pRegEntry, 0, sizeof(*pRegEntry));

    status = LwRtlWC16StringDuplicate(&pRegEntry->pwszValueName, pwszValueName);
    BAIL_ON_NT_STATUS(status);

    if (dwValueLen)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry->pValue, BYTE, sizeof(*pRegEntry->pValue)*dwValueLen);
        BAIL_ON_NT_STATUS(status);

        memset(pRegEntry->pValue, 0, sizeof(*pRegEntry->pValue)*dwValueLen);

        memcpy(pRegEntry->pValue, pValue, dwValueLen);
    }

    pRegEntry->dwValueLen = dwValueLen;
    pRegEntry->type = valueType;
    pRegEntry->qwParentId = qwParentKeyId;

    status = RegDbStoreRegValues_inlock(
                 hDb,
                 1,
                 &pRegEntry);
    BAIL_ON_NT_STATUS(status);

    if (ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }

cleanup:
    if (!ppRegEntry)
    {
        RegDbSafeFreeEntryValue(&pRegEntry);
    }

    return status;

error:
    RegDbSafeFreeEntryValue(&pRegEntry);
    *ppRegEntry = NULL;

    goto cleanup;
}



void
RegDbSafeFreeEntryKeyList(
    size_t sCount,
    PREG_DB_KEY** pppEntries
    )
{
    if (*pppEntries != NULL)
    {
        size_t iEntry;
        for (iEntry = 0; iEntry < sCount; iEntry++)
        {
            RegDbSafeFreeEntryKey(&(*pppEntries)[iEntry]);
        }
        LWREG_SAFE_FREE_MEMORY(*pppEntries);
    }
}

void
RegDbSafeFreeEntryValueList(
    size_t sCount,
    PREG_DB_VALUE** pppEntries
    )
{
    if (*pppEntries != NULL)
    {
        size_t iEntry;
        for (iEntry = 0; iEntry < sCount; iEntry++)
        {
            RegDbSafeFreeEntryValue(&(*pppEntries)[iEntry]);
        }
        LWREG_SAFE_FREE_MEMORY(*pppEntries);
    }
}

NTSTATUS
RegDbDuplicateDbKeyEntry(
    PREG_DB_KEY pRegKey,
    PREG_DB_KEY* ppRegKey
    )
{
	NTSTATUS status = 0;
	PREG_DB_KEY pRegDupKey = NULL;

	if (!pRegKey)
	{
		goto done;
	}

    status = LW_RTL_ALLOCATE((PVOID*)&pRegDupKey, REG_DB_KEY, sizeof(*pRegDupKey));
    BAIL_ON_NT_STATUS(status);

    memcpy(pRegDupKey, pRegKey, sizeof(*pRegKey));

    pRegDupKey->pwszFullKeyName = NULL;
    pRegDupKey->pwszKeyName = NULL;
    pRegDupKey->pSecDescRel = NULL;

    status = LwRtlWC16StringDuplicate(&pRegDupKey->pwszKeyName,
    		                          pRegKey->pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pRegDupKey->pwszFullKeyName,
    		                          pRegKey->pwszFullKeyName);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pRegDupKey->pSecDescRel, VOID, pRegKey->ulSecDescLength);
    BAIL_ON_NT_STATUS(status);

    memcpy(pRegDupKey->pSecDescRel, pRegKey->pSecDescRel, pRegKey->ulSecDescLength);

done: 
    *ppRegKey = pRegDupKey;

cleanup:

    return status;

error:

    *ppRegKey = NULL;

    RegDbSafeFreeEntryKey(&pRegDupKey);

    goto cleanup;
}

void
RegDbSafeFreeEntryKey(
    PREG_DB_KEY* ppEntry
    )
{
	PREG_DB_KEY pEntry = NULL;
    if (ppEntry != NULL && *ppEntry != NULL)
    {
        pEntry = *ppEntry;

        LWREG_SAFE_FREE_MEMORY(pEntry->pwszKeyName);
        LWREG_SAFE_FREE_MEMORY(pEntry->pwszFullKeyName);
        LWREG_SAFE_FREE_MEMORY(pEntry->pSecDescRel);
        memset(pEntry, 0, sizeof(*pEntry));

        LWREG_SAFE_FREE_MEMORY(pEntry);

        *ppEntry = NULL;
    }
}

void
RegDbSafeFreeEntryValue(
    PREG_DB_VALUE* ppEntry
    )
{
    PREG_DB_VALUE pEntry = NULL;
    if (ppEntry != NULL && *ppEntry != NULL)
    {
        pEntry = *ppEntry;

        LWREG_SAFE_FREE_MEMORY(pEntry->pwszValueName);
        LWREG_SAFE_FREE_MEMORY(pEntry->pValue);
        memset(pEntry, 0, sizeof(*pEntry));

        LWREG_SAFE_FREE_MEMORY(pEntry);
        *ppEntry = NULL;
    }
}

NTSTATUS
RegDbSafeRecordSubKeysInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_KEY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    int iCount = 0;
    size_t sSubKeyLen = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeWC16StringArray(pKeyResult->ppwszSubKeyNames, pKeyResult->dwNumCacheSubKeys);

    if (sCacheCount)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppwszSubKeyNames,
    	    	                 PWSTR,
    		                     sizeof(*(pKeyResult->ppwszSubKeyNames)) * sCacheCount);
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
		if (ppRegEntries[iCount]->pwszKeyName)
		{
			sSubKeyLen = RtlWC16StringNumChars(ppRegEntries[iCount]->pwszKeyName);

	        status = LwRtlWC16StringDuplicate(&pKeyResult->ppwszSubKeyNames[iCount],
	              		                      ppRegEntries[iCount]->pwszKeyName);
	        BAIL_ON_NT_STATUS(status);
	    }

		if (pKeyResult->sMaxSubKeyLen < sSubKeyLen)
			pKeyResult->sMaxSubKeyLen = sSubKeyLen;

        sSubKeyLen = 0;
    }

    pKeyResult->dwNumSubKeys = (DWORD)sCount;
    pKeyResult->dwNumCacheSubKeys = sCacheCount;
    pKeyResult->bHasSubKeyInfo = TRUE;

cleanup:
    return status;

error:
    pKeyResult->bHasSubKeyInfo = FALSE;
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordSubKeysInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_KEY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegDbSafeRecordSubKeysInfo_inlock(sCount,
											   sCacheCount,
											   ppRegEntries,
											   pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordValuesInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    int iCount = 0;
    size_t sValueNameLen = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeWC16StringArray(pKeyResult->ppwszValueNames, pKeyResult->dwNumCacheValues);
    RegFreeValueByteArray(pKeyResult->ppValues, pKeyResult->dwNumCacheValues);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pTypes);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pdwValueLen);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppwszValueNames, PWSTR,
    		                  sizeof(*(pKeyResult->ppwszValueNames))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppValues, PBYTE,
    		                  sizeof(*(pKeyResult->ppValues))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pTypes, REG_DATA_TYPE,
    		                  sizeof(*(pKeyResult->pTypes))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pdwValueLen, DWORD,
    		                  sizeof(*(pKeyResult->pdwValueLen))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        status = LwRtlWC16StringDuplicate(&pKeyResult->ppwszValueNames[iCount],
        		                          ppRegEntries[iCount]->pwszValueName);
        BAIL_ON_NT_STATUS(status);

        if (ppRegEntries[iCount]->dwValueLen)
        {
            status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppValues[iCount], BYTE,
            		                sizeof(*(pKeyResult->ppValues[iCount]))* ppRegEntries[iCount]->dwValueLen);
            BAIL_ON_NT_STATUS(status);

            memcpy(pKeyResult->ppValues[iCount], ppRegEntries[iCount]->pValue, ppRegEntries[iCount]->dwValueLen);
        }

        pKeyResult->pdwValueLen[iCount] = ppRegEntries[iCount]->dwValueLen;
        pKeyResult->pTypes[iCount] = ppRegEntries[iCount]->type;

		if (pKeyResult->sMaxValueLen < (size_t)ppRegEntries[iCount]->dwValueLen)
		{
			pKeyResult->sMaxValueLen = (size_t)ppRegEntries[iCount]->dwValueLen;
		}

		if (pKeyResult->ppwszValueNames[iCount])
		{
			sValueNameLen = RtlWC16StringNumChars(pKeyResult->ppwszValueNames[iCount]);
		}

		if (pKeyResult->sMaxValueNameLen < sValueNameLen)
			pKeyResult->sMaxValueNameLen = sValueNameLen;

        sValueNameLen = 0;
    }

cleanup:
    pKeyResult->dwNumValues = (DWORD)sCount;
    pKeyResult->dwNumCacheValues = sCacheCount;

    pKeyResult->bHasValueInfo = TRUE;

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegDbSafeRecordValuesInfo_inlock(sCount,
                                           sCacheCount,
                                           ppRegEntries,
                                           pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}


NTSTATUS
RegDbSafeRecordDefaultValuesInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int iCount = 0;
    size_t sValueNameLen = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeWC16StringArray(pKeyResult->ppwszDefaultValueNames, pKeyResult->dwNumCacheDefaultValues);
    RegFreeValueByteArray(pKeyResult->ppDefaultValues, pKeyResult->dwNumCacheDefaultValues);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pDefaultTypes);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pdwDefaultValueLen);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppwszDefaultValueNames, PWSTR,
                              sizeof(*(pKeyResult->ppwszDefaultValueNames))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppDefaultValues, PBYTE,
                              sizeof(*(pKeyResult->ppDefaultValues))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pDefaultTypes, REG_DATA_TYPE,
                              sizeof(*(pKeyResult->pDefaultTypes))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pdwDefaultValueLen, DWORD,
                              sizeof(*(pKeyResult->pdwDefaultValueLen))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        status = LwRtlWC16StringDuplicate(&pKeyResult->ppwszDefaultValueNames[iCount],
                                          ppRegEntries[iCount]->pwszValueName);
        BAIL_ON_NT_STATUS(status);

        if (!ppRegEntries[iCount]->pValueAttributes)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        if (ppRegEntries[iCount]->pValueAttributes->DefaultValueLen)
        {
            status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppDefaultValues[iCount], BYTE,
               sizeof(*(pKeyResult->ppDefaultValues[iCount])) * ppRegEntries[iCount]->pValueAttributes->DefaultValueLen);
            BAIL_ON_NT_STATUS(status);

            memcpy(pKeyResult->ppDefaultValues[iCount],
                   ppRegEntries[iCount]->pValueAttributes->pDefaultValue,
                   ppRegEntries[iCount]->pValueAttributes->DefaultValueLen);
        }

        pKeyResult->pdwDefaultValueLen[iCount] = ppRegEntries[iCount]->pValueAttributes->DefaultValueLen;
        pKeyResult->pDefaultTypes[iCount] = ppRegEntries[iCount]->pValueAttributes->ValueType;

        if (pKeyResult->sMaxValueLen < (size_t)ppRegEntries[iCount]->pValueAttributes->DefaultValueLen)
        {
            pKeyResult->sMaxValueLen = (size_t)ppRegEntries[iCount]->pValueAttributes->DefaultValueLen;
        }

        if (pKeyResult->ppwszDefaultValueNames[iCount])
        {
            sValueNameLen = RtlWC16StringNumChars(pKeyResult->ppwszDefaultValueNames[iCount]);
        }

        if (pKeyResult->sMaxValueNameLen < sValueNameLen)
            pKeyResult->sMaxValueNameLen = sValueNameLen;

        sValueNameLen = 0;
    }

cleanup:
    pKeyResult->dwNumDefaultValues = (DWORD)sCount;
    pKeyResult->dwNumCacheDefaultValues = sCacheCount;

    pKeyResult->bHasDefaultValueInfo = TRUE;

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordDefaultValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegDbSafeRecordDefaultValuesInfo_inlock(sCount,
                                                     sCacheCount,
                                                     ppRegEntries,
                                                     pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

