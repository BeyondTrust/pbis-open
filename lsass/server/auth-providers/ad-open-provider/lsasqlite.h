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
 *        lsasqlite.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Sqlite wrapper methods used by the cache API
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __LSASQLITE_H__
#define __LSASQLITE_H__

#include <lw/security-types.h>

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
           LSA_LOG_DEBUG("Sqlite3 error '%s' (code = %u)", \
                         LSA_SAFE_LOG_STRING(pszError), dwError); \
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

typedef DWORD (*PFN_LSA_SQLITE_EXEC_CALLBACK)(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );

DWORD
LsaSqliteReadUInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    uint64_t *pqwResult);

DWORD
LsaSqliteReadInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int64_t *pqwResult);

DWORD
LsaSqliteReadUInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    DWORD *pdwResult);

DWORD
LsaSqliteReadBoolean(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    BOOLEAN *pbResult);

DWORD
LsaSqliteReadString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PSTR *ppszResult);

DWORD
LsaSqliteReadStringInPlace(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSTR pszResult,
    //Includes NULL
    IN size_t sMaxSize);

DWORD
LsaSqliteReadSid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSID* ppSid);

DWORD
LsaSqliteReadGuid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT uuid_t** ppGuid);

DWORD
LsaSqliteBindInt64(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int64_t Value
    );

DWORD
LsaSqliteReadTimeT(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    time_t *pResult);

DWORD
LsaSqliteBindString(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN PCSTR pszValue
    );

DWORD
LsaSqliteBindBoolean(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN BOOLEAN bValue
    );

DWORD
LsaSqliteAllocPrintf(
    OUT PSTR* ppszSqlCommand,
    IN PCSTR pszSqlFormat,
    IN ...
    );

DWORD
LsaSqliteExec(
    IN sqlite3* pSqlDatabase,
    IN PCSTR pszSqlCommand,
    OUT PSTR* ppszSqlError
    );

DWORD
LsaSqliteExecCallbackWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PFN_LSA_SQLITE_EXEC_CALLBACK pfnCallback,
    IN PVOID pContext
    );

DWORD
LsaSqliteExecWithRetry(
    IN sqlite3* pDb,
    IN pthread_rwlock_t* pLock,
    IN PCSTR pszTransaction
    );

#endif /* __LSASQLITE_H__ */
