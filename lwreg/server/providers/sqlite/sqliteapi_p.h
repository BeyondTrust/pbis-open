/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        sqliteapi_p.h
 *
 * Abstract:
 *
 *        Registry sqlite provider (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef SQLITEAPI_P_H_
#define SQLITEAPI_P_H_

struct __REG_KEY_CONTEXT
{
    LONG refCount;

    pthread_rwlock_t mutex;
    pthread_rwlock_t* pMutex;

    int64_t qwId;
    PWSTR pwszKeyName;

    int64_t qwSdId;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor;
    ULONG ulSecDescLength;
    BOOLEAN bHasSdInfo;


    PWSTR pwszParentKeyName;

    DWORD dwNumSubKeys;
    DWORD dwNumCacheSubKeys;
    size_t sMaxSubKeyLen;
    PWSTR* ppwszSubKeyNames;
    BOOLEAN bHasSubKeyInfo;

    size_t sMaxValueNameLen;
    size_t sMaxValueLen;

    // keep track of the number of values that have been set by user
    DWORD dwNumValues;
    DWORD dwNumCacheValues;

    PREG_DATA_TYPE pTypes;
    PWSTR* ppwszValueNames;
    PBYTE* ppValues;
    PDWORD pdwValueLen;

    BOOLEAN bHasValueInfo;

    // keep track of the number of values that have not been set by user
    // but are defined in schema as value attributes
    DWORD dwNumDefaultValues;
    DWORD dwNumCacheDefaultValues;

    PREG_DATA_TYPE pDefaultTypes;
    PWSTR* ppwszDefaultValueNames;
    PBYTE* ppDefaultValues;
    PDWORD pdwDefaultValueLen;

    BOOLEAN bHasDefaultValueInfo;



};

typedef struct __REG_KEY_CONTEXT REG_KEY_CONTEXT;

NTSTATUS
SqliteGetKeyToken(
    PCWSTR pwszInputString,
    wchar16_t c,
    PWSTR *ppwszOutputString
    );

NTSTATUS
SqliteGetParentKeyName(
    PCWSTR pwszInputString,
    wchar16_t c,
    PWSTR *ppwszOutputString
    );

NTSTATUS
SqliteCreateKeyHandle(
    IN PACCESS_TOKEN pToken,
    IN ACCESS_MASK AccessDesired,
    IN PREG_KEY_CONTEXT pKey,
    OUT PREG_KEY_HANDLE* ppKeyHandle
    );

NTSTATUS
SqliteCreateKeyContext(
    IN PREG_DB_KEY pRegEntry,
    OUT PREG_KEY_CONTEXT* ppKeyResult
    );

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
    );

NTSTATUS
SqliteOpenKeyInternal(
	IN HANDLE handle,
    IN PCWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
    );

NTSTATUS
SqliteOpenKeyInternal_inlock(
	IN OPTIONAL HANDLE handle,
    IN PCWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
    );

NTSTATUS
SqliteOpenKeyInternal_inlock_inDblock(
	IN OPTIONAL HANDLE handle,
	IN PCWSTR pwszFullKeyName, // Full Key Path
	IN ACCESS_MASK AccessDesired,
	OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
	);

VOID
SqliteCloseKey_inlock(
    IN HKEY hKey
    );

NTSTATUS
SqliteDeleteKeyInternal_inlock(
	IN HANDLE handle,
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteDeleteKeyInternal_inlock_inDblock(
	IN HANDLE handle,
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteDeleteActiveKey(
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteDeleteActiveKey_inlock(
    IN PCWSTR pwszKeyName
    );

REG_DATA_TYPE
GetRegDataType(
    REG_DATA_TYPE_FLAGS Flags
    );

#endif /* SQLITEAPI_P_H_ */
