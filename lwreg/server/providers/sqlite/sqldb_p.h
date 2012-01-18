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
 *        sqldb_p.h
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Private functions in sqlite3 Caching backend
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#ifndef __SQLCACHE_P_H__
#define __SQLCACHE_P_H__


#define REG_DB_FREE_UNUSED_CACHEIDS   \
    "delete from " REG_DB_TABLE_NAME_CACHE_TAGS " where CacheId NOT IN " \
        "CacheId NOT IN ( select CacheId from " REG_DB_TABLE_NAME_ENTRIES " );\n"

typedef struct _REG_DB_CONNECTION
{
    sqlite3 *pDb;
    pthread_rwlock_t lock;

    // registry user view related sql statement

    sqlite3_stmt *pstCreateRegKey;
    sqlite3_stmt *pstCreateRegValue;
    sqlite3_stmt *pstCreateRegAcl;
	sqlite3_stmt *pstUpdateRegValue;
	sqlite3_stmt *pstQueryKeyAclIndex;
	sqlite3_stmt *pstQueryKeyAcl;
	sqlite3_stmt *pstQueryKeyAclIndexByKeyId;
	sqlite3_stmt *pstUpdateKeyAclIndexByKeyId;
    sqlite3_stmt *pstOpenKeyEx;
    sqlite3_stmt *pstDeleteKey;
    sqlite3_stmt *pstDeleteAllKeyValues;
    sqlite3_stmt *pstDeleteKeyValue;
    sqlite3_stmt *pstDeleteAcl;
    sqlite3_stmt *pstQuerySubKeys;
    sqlite3_stmt *pstQuerySubKeysCount;
    sqlite3_stmt *pstQueryValues;
    sqlite3_stmt *pstQueryValuesCount;
    sqlite3_stmt *pstQueryKeyValue;
    sqlite3_stmt *pstQueryKeyValueWithType;
    sqlite3_stmt *pstQueryKeyValueWithWrongType;
    sqlite3_stmt *pstQueryMultiKeyValues;
    sqlite3_stmt *pstQueryAclRefCount;
    sqlite3_stmt *pstQueryTotalAclCount;
    sqlite3_stmt *pstQueryAclByOffset;
    sqlite3_stmt *pstUpdateRegAclByCacheId;

    // registry schema view related sql statement
    sqlite3_stmt *pstCreateRegValueAttributes;
    sqlite3_stmt *pstQueryValueAttributes;
    sqlite3_stmt *pstQueryValueAttributesWithType;
    sqlite3_stmt *pstQueryValueAttributesWithWrongType;
    sqlite3_stmt *pstUpdateValueAttributes;
    sqlite3_stmt *pstDeleteValueAttributes;
    sqlite3_stmt *pstDeleteAllValueAttributes;
    sqlite3_stmt *pstQueryDefaultValues;
    sqlite3_stmt *pstQueryDefaultValuesCount;


} REG_DB_CONNECTION, *PREG_DB_CONNECTION;


NTSTATUS
RegDbUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PREG_ENTRY_VERSION_INFO pResult
    );

NTSTATUS
RegDbUnpackRegKeyInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_DB_KEY pResult
    );

NTSTATUS
RegDbUnpackRegValueInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_DB_VALUE pResult
    );

NTSTATUS
RegDbUnpackSubKeysCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

NTSTATUS
RegDbUnpackAclrefCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

NTSTATUS
RegDbUnpackTotalAclCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

NTSTATUS
RegDbUnpackKeyValuesCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

NTSTATUS
RegDbUnpackAclIndexInfoInAcls(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    int64_t* pqwAclIndex
    );

NTSTATUS
RegDbUnpackAclIndexInfoInKeys(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    int64_t* pqwAclIndex
    );

NTSTATUS
RegDbUnpackAclInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    PULONG pSecDescLen
    );

NTSTATUS
RegDbDuplicateDbKeyEntry(
    PREG_DB_KEY pRegKey,
    PREG_DB_KEY* ppRegKey
    );

void
RegDbSafeFreeEntryKey(
    PREG_DB_KEY* ppEntry
    );

void
RegDbSafeFreeEntryValue(
    PREG_DB_VALUE* ppEntry
    );

void
RegDbSafeFreeEntryKeyList(
    size_t sCount,
    PREG_DB_KEY** pppEntries
    );

void
RegDbSafeFreeEntryValueList(
    size_t sCount,
    PREG_DB_VALUE** pppEntries
    );

NTSTATUS
RegDbSafeRecordSubKeysInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_KEY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordSubKeysInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_KEY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordValuesInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordDefaultValuesInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordDefaultValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbOpen(
    IN PCSTR pszDbPath,
    OUT PREG_DB_HANDLE phDb
    );

NTSTATUS
RegDbUpdateRegValues(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    );

NTSTATUS
RegDbStoreRegValues(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    );

NTSTATUS
RegDbCreateKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyName,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT PREG_DB_KEY* ppRegKey
    );

NTSTATUS
RegDbOpenKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    OUT OPTIONAL PREG_DB_KEY* ppRegEntry
    );

NTSTATUS
RegDbOpenKey_inlock(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyPath,
    OUT OPTIONAL PREG_DB_KEY* ppRegKey
    );

NTSTATUS
RegDbDeleteKey(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN int64_t qwAclId,
    IN PCWSTR pwszFullKeyName
    );

NTSTATUS
RegDbDeleteKey_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN int64_t qwAclId,
    IN PCWSTR pwszFullKeyName
    );

NTSTATUS
RegDbDeleteKeyWithNoSubKeys_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN int64_t qwAclId,
    IN PCWSTR pwszFullKeyName
    );

NTSTATUS
RegDbQueryInfoKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_KEY** pppRegEntries
    );

NTSTATUS
RegDbQueryInfoKey_inlock(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_KEY** pppRegEntries
    );

NTSTATUS
RegDbQueryInfoKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_VALUE** pppRegEntries
    );

NTSTATUS
RegDbQueryInfoKeyCount(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN QueryKeyInfoOption queryType,
    OUT size_t* psCount
    );

NTSTATUS
RegDbQueryInfoKeyCount_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN QueryKeyInfoOption queryType,
    OUT size_t* psCount
    );

NTSTATUS
RegDbCreateKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    );

NTSTATUS
RegDbGetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    );

NTSTATUS
RegDbSetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN const PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    );

NTSTATUS
RegDbDeleteKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName
    );

NTSTATUS
RegDbUpdateKeyAcl(
	IN REG_DB_HANDLE hDb,
	IN PCWSTR pwszFullKeyPath,
	IN int64_t qwKeyDbId,
	IN int64_t qwKeyCurrSdId,
	IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelToSet,
	IN ULONG ulSecDescToSetLen
	);

NTSTATUS
RegDbGetKeyAclByKeyId(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwKeyDbId,
    OUT int64_t *pqwKeyAclId,
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    OUT PULONG pSecDescLen
    );

NTSTATUS
RegDbFixAcls(
    IN REG_DB_HANDLE hDb
    );

void
RegDbSafeClose(
    PREG_DB_HANDLE phDb
    );

NTSTATUS
RegDbEmptyCache(
    IN REG_DB_HANDLE hDb
    );

NTSTATUS
RegDbFlushNOP(
    REG_DB_HANDLE hDb
    );


//Inlock db utility functions
NTSTATUS
RegDbOpenKeyName_inlock(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN OUT int64_t* pqwParentId,
    OUT PREG_DB_KEY* ppRegEntry
    );

NTSTATUS
RegDbGetKeyAclIndexByKeyAcl_inlock(
	IN REG_DB_HANDLE hDb,
	IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescLen,
	OUT int64_t* pqwAclIndex
	);

NTSTATUS
RegDbGetKeyAclByAclIndex_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwAclIndex,
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    OUT PULONG pulSecDescLen
    );

NTSTATUS
RegDbQueryTotalAclCount_inlock(
    IN REG_DB_HANDLE hDb,
    OUT size_t* psCount
    );

NTSTATUS
RegDbGetKeyAclByAclOffset_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwOffset,
    OUT int64_t* pqwCacheId,
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    OUT PULONG pulSecDescLength
    );

NTSTATUS
RegDbQueryAclRefCountWOCurrKey_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwSdId,
    IN int64_t qwKeyId,
    OUT size_t* psCount
    );

NTSTATUS
RegDbGetKeyAclIndexByKeyId_inlock(
	IN REG_DB_HANDLE hDb,
	IN int64_t qwKeyDbId,
	OUT int64_t* pqwAclIndex
	);

NTSTATUS
RegDbDeleteAcl_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwSdCacheId
    );

NTSTATUS
RegDbUpdateKeyAclIndex_inlock(
	IN REG_DB_HANDLE hDb,
	IN int64_t qwKeyDbId,
	IN int64_t qwKeySdId
	);

NTSTATUS
RegDbGetKeyValue_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    );

NTSTATUS
RegDbDeleteKeyValue_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName
    );

NTSTATUS
RegDbUpdateRegValues_inlock(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    );

NTSTATUS
RegDbStoreRegKeys_inlock(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_KEY* ppKeys
    );

NTSTATUS
RegDbStoreRegValues_inlock(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    );

NTSTATUS
RegDbCreateKeyValue_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    );

#endif /* __SQLCACHE_P_H__ */
