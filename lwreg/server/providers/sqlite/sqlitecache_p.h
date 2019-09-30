/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        sqlitecache_p.h
 *
 * Abstract:
 *
 *        Registry Sqlite Cache private header
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#ifndef __SQLITECACHE_P_H_
#define __SQLITECACHE_P_H_


PREG_KEY_CONTEXT
SqliteCacheLocateActiveKey(
    IN PCWSTR pwszKeyName
    );

PREG_KEY_CONTEXT
SqliteCacheLocateActiveKey_inlock(
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteCacheInsertActiveKey(
    IN PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheInsertActiveKey_inlock(
    IN PREG_KEY_CONTEXT pKeyResult
    );

VOID
SqliteCacheDeleteActiveKey(
    IN PWSTR pszKeyName
    );

VOID
SqliteCacheDeleteActiveKey_inlock(
    IN PWSTR pwszKeyName
    );

void
SqliteCacheResetParentKeySubKeyInfo(
    IN PCWSTR pwszParentKeyName
    );

void
SqliteCacheResetParentKeySubKeyInfo_inlock(
    IN PCWSTR pwszParentKeyName
    );

void
SqliteCacheResetKeyValueInfo(
    IN PCWSTR pwszKeyName
    );

void
SqliteCacheResetKeyValueInfo_inlock(
    IN PCWSTR pwszKeyName
    );

void
SqliteSafeFreeKeyHandle(
	PREG_KEY_HANDLE pKeyHandle
	);

void
SqliteSafeFreeKeyHandle_inlock(
	PREG_KEY_HANDLE pKeyHandle
	);

VOID
SqliteReleaseKeyContext(
    PREG_KEY_CONTEXT pKeyResult
    );

VOID
SqliteReleaseKeyContext_inlock(
    PREG_KEY_CONTEXT pKeyResult
    );

void
SqliteCacheFreeKeyCtxHashEntry(
    IN const REG_HASH_ENTRY* pEntry
    );

NTSTATUS
SqliteCacheSubKeysInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheSubKeysInfo_inlock_inDblock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheKeySecurityDescriptor(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheKeySecurityDescriptor_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheSubKeysInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheUpdateSubKeysInfo_inlock(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumSubKeys
    );

NTSTATUS
SqliteCacheUpdateSubKeysInfo(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumSubKeys
    );

NTSTATUS
SqliteCacheKeyValuesInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheKeyValuesInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheUpdateValuesInfo_inlock(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    );

NTSTATUS
SqliteCacheUpdateValuesInfo(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    );

NTSTATUS
SqliteCacheUpdateSubKeysInfo(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumSubKeys
    );

NTSTATUS
SqliteCacheKeyDefaultValuesInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheKeyDefaultValuesInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteCacheUpdateDefaultValuesInfo_inlock(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    );

NTSTATUS
SqliteCacheUpdateDefaultValuesInfo(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    OUT size_t* psNumValues
    );

// Sqlite DB Key Index and ACL Index mapping cache
void
SqliteCacheFreeDbKeyHashEntry(
    IN const REG_HASH_ENTRY* pEntry
    );

NTSTATUS
SqliteCacheGetDbKeyInfo(
    IN PCWSTR pwszKeyName,
    OUT PREG_DB_KEY* ppRegKey
    );

VOID
SqliteCacheDeleteDbKeyInfo(
    IN PCWSTR pwszKeyName
    );

VOID
SqliteCacheDeleteDbKeyInfo_inlock(
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteCacheInsertDbKeyInfo(
    IN PREG_DB_KEY pRegKey
    );

NTSTATUS
SqliteCacheInsertDbKeyInfo_inlock(
    IN PREG_DB_KEY pRegKey
    );

VOID
SqliteReleaseDbKeyInfo(
    PREG_DB_KEY pRegKey
    );

VOID
SqliteReleaseDbKeyInfo_inlock(
    PREG_DB_KEY pRegKey
    );


#endif // __SQLITECACHE__P_H_
