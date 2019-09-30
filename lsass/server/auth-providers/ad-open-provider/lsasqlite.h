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
 *        lsasqlite.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
