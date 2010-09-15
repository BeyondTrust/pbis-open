/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        regsqlite.h
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Sqlite wrapper methods used by the cache API
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __REGSQLITE_H__
#define __REGSQLITE_H__

#include <lw/security-types.h>
#include "regserver.h"

typedef enum
{
    QuerySubKeys = 0,
    QueryValues = 1
} QueryKeyInfoOption;

#define SQLITE3_SAFE_FREE_STRING(x) \
    if ((x) != NULL) \
    { \
       sqlite3_free(x); \
       (x) = NULL; \
    }

#define BAIL_ON_SQLITE3_ERROR(dwError, pszError) \
    do { \
        if (dwError) \
        { \
           REG_LOG_DEBUG("Sqlite3 error '%s' (code = %d)", \
                         REG_SAFE_LOG_STRING(pszError), dwError); \
           goto error;                               \
        } \
    } while (0)

#define BAIL_ON_SQLITE3_ERROR_DB(dwError, pDb) \
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb))

#define BAIL_ON_SQLITE3_ERROR_STMT(dwError, pStatement) \
    BAIL_ON_SQLITE3_ERROR_DB(dwError, sqlite3_db_handle(pStatement))

#define ENTER_SQLITE_LOCK(pLock, bInLock)                 \
        if (!bInLock) {                                    \
           pthread_rwlock_wrlock(pLock);            \
           bInLock = TRUE;                                 \
        }

#define LEAVE_SQLITE_LOCK(pLock, bInLock)                 \
        if (bInLock) {                                     \
           pthread_rwlock_unlock(pLock);            \
           bInLock = FALSE;                                \
        }

typedef NTSTATUS (*PFN_REG_SQLITE_EXEC_CALLBACK)(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );

struct _REG_DB_CONNECTION;
typedef struct _REG_DB_CONNECTION *REG_DB_HANDLE;
typedef REG_DB_HANDLE *PREG_DB_HANDLE;

typedef struct __REG_ENTRY_VERSION_INFO
{
    // This value is set to -1 if the value is not stored in the
    // database (it only exists in memory). Otherwise, this is an index into
    // the database.
    int64_t qwDbId;
    time_t tLastUpdated;
} REG_ENTRY_VERSION_INFO, *PREG_ENTRY_VERSION_INFO;

typedef struct __REG_DB_KEY
{
   REG_ENTRY_VERSION_INFO version;
   int64_t qwParentId;
   PWSTR pwszFullKeyName; //Full Key Path
   PWSTR pwszKeyName;
   int64_t qwAclIndex;
   PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel;
   ULONG ulSecDescLength;
} REG_DB_KEY, *PREG_DB_KEY;

typedef struct __REG_DB_VALUE
{
   time_t tLastUpdated;
   int64_t qwParentId;
   PWSTR pwszValueName;
   REG_DATA_TYPE type;
   PBYTE pValue;
   DWORD dwValueLen;
} REG_DB_VALUE, *PREG_DB_VALUE;

typedef struct __REG_DB_VALUE_ATTRIBUTES
{
   time_t tLastUpdated;
   int64_t qwParentId;
   PWSTR pwszValueName;
   PLWREG_VALUE_ATTRIBUTES pValueAttributes;
} REG_DB_VALUE_ATTRIBUTES, *PREG_DB_VALUE_ATTRIBUTES;

NTSTATUS
RegSqliteReadUInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    uint64_t *pqwResult);

NTSTATUS
RegSqliteReadInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int64_t *pqwResult);

NTSTATUS
RegSqliteReadInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int *piResult
    );

NTSTATUS
RegSqliteReadUInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    DWORD *pdwResult
    );

NTSTATUS
RegSqliteReadBlob(
	sqlite3_stmt *pstQuery,
	int *piColumnPos,
	PCSTR name,
	PBYTE* ppValue,
	PDWORD pdwValueLen
	);

NTSTATUS
RegSqliteReadString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PSTR *ppszResult
    );

NTSTATUS
RegSqliteReadWC16String(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PWSTR *ppwszResult
    );

NTSTATUS
RegSqliteReadWC16String(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PWSTR *ppszResult
    );

NTSTATUS
RegSqliteBindInt64(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int64_t Value
    );

NTSTATUS
RegSqliteBindInt32(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int Value
    );

NTSTATUS
RegSqliteBindBlob(
	IN OUT sqlite3_stmt* pstQuery,
	IN int Index,
	IN BYTE* Value,
	IN DWORD dwValueLen
	);

NTSTATUS
RegSqliteReadTimeT(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    time_t *pResult);

NTSTATUS
RegSqliteBindString(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN PCSTR pszValue
    );

NTSTATUS
RegSqliteBindStringW(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN PCWSTR pwszValue
    );

NTSTATUS
RegSqliteBindBoolean(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN BOOLEAN bValue
    );

NTSTATUS
RegSqliteExec(
    IN sqlite3* pSqlDatabase,
    IN PCSTR pszSqlCommand,
    OUT PSTR* ppszSqlError
    );

NTSTATUS
RegSqliteExecCallbackWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PFN_REG_SQLITE_EXEC_CALLBACK pfnCallback,
    IN PVOID pContext
    );

NTSTATUS
RegSqliteExecWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PCSTR pszTransaction
    );

#endif /* __REGSQLITE_H__ */
