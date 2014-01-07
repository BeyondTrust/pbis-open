/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samdbuser.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM User Specific Management Methods
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

static
DWORD
SamDbSetPassword_inlock(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

static
DWORD
SamDbVerifyPassword_inlock(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
SamDbSetPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    BOOLEAN bInLock = FALSE;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbSetPassword_inlock(
                    hBindHandle,
                    pwszUserDN,
                    pwszPassword);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbIncrementSequenceNumber_inlock(
                    pDirectoryContext);
    BAIL_ON_SAMDB_ERROR(dwError);

error:
    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    return dwError;
}

static
DWORD
SamDbSetPassword_inlock(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;
    BYTE lmHash[16];
    BYTE ntHash[16];
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    LONG64 llCurTime = 0;
    PCSTR pszQueryTemplate = "UPDATE " SAM_DB_OBJECTS_TABLE \
                             "   SET " SAM_DB_COL_LM_HASH " = ?1," \
                                       SAM_DB_COL_NT_HASH " = ?2," \
                                       SAM_DB_COL_PASSWORD_LAST_SET " = ?3"  \
                             " WHERE " SAM_DB_COL_DISTINGUISHED_NAME " = ?4" \
                             "   AND " SAM_DB_COL_OBJECT_CLASS " = ?5";
    sqlite3_stmt* pSqlStatement = NULL;
    PSTR pszPassword = NULL;
    PSTR pszUserDN = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_USER;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    dwError = LwWc16sToMbs(
                    pwszPassword,
                    &pszPassword);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pwszUserDN,
                    &pszUserDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQueryTemplate,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    memset(&lmHash[0], 0, sizeof(lmHash));
    memset(&ntHash[0], 0, sizeof(ntHash));

    dwError = SamDbComputeLMHash(
                pszPassword,
                &lmHash[0],
                sizeof(lmHash));
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbComputeNTHash(
                pwszPassword,
                &ntHash[0],
                sizeof(ntHash));
    BAIL_ON_SAMDB_ERROR(dwError);

    llCurTime = SamDbGetNTTime(time(NULL));

    dwError = sqlite3_bind_blob(
                    pSqlStatement,
                    1,
                    &lmHash[0],
                    sizeof(lmHash),
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    dwError = sqlite3_bind_blob(
                    pSqlStatement,
                    2,
                    &ntHash[0],
                    sizeof(ntHash),
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    dwError = sqlite3_bind_int64(
                    pSqlStatement,
                    3,
                    llCurTime);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    4,
                    pszUserDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    dwError = sqlite3_bind_int(
                    pSqlStatement,
                    5,
                    objectClass);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if (!sqlite3_changes(pDirectoryContext->pDbContext->pDbHandle))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

cleanup:

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    LW_SECURE_FREE_STRING(pszPassword);
    DIRECTORY_FREE_STRING(pszUserDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbChangePassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbVerifyPassword_inlock(
                    hBindHandle,
                    pwszUserDN,
                    pwszOldPassword);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSetPassword_inlock(
                    hBindHandle,
                    pwszUserDN,
                    pwszNewPassword);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbVerifyPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbVerifyPassword_inlock(
                    hBindHandle,
                    pwszUserDN,
                    pwszPassword);

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    return dwError;
}

static
DWORD
SamDbVerifyPassword_inlock(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PSTR pszPassword = NULL;
    PSTR pszUserDN = NULL;
    BYTE lmHash[16];
    BYTE ntHash[16];
    BYTE lmHashDbValue[16];
    BYTE ntHashDbValue[16];
    sqlite3_stmt* pSqlStatement = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_USER;
    PCSTR pszQueryTemplate = "SELECT " SAM_DB_COL_LM_HASH "," \
                                       SAM_DB_COL_NT_HASH     \
                             "  FROM " SAM_DB_OBJECTS_TABLE   \
                             " WHERE " SAM_DB_COL_DISTINGUISHED_NAME " = ?1" \
                             "   AND " SAM_DB_COL_OBJECT_CLASS " = ?2";

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    dwError = LwWc16sToMbs(
                    pwszPassword,
                    &pszPassword);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pwszUserDN,
                    &pszUserDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    memset(&lmHash[0], 0, sizeof(lmHash));
    memset(&ntHash[0], 0, sizeof(ntHash));

    memset(&lmHashDbValue[0], 0, sizeof(lmHashDbValue));
    memset(&ntHashDbValue[0], 0, sizeof(ntHashDbValue));

    dwError = SamDbComputeLMHash(
                pszPassword,
                &lmHash[0],
                sizeof(lmHash));
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbComputeNTHash(
                pwszPassword,
                &ntHash[0],
                sizeof(ntHash));
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQueryTemplate,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    1,
                    pszUserDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    dwError = sqlite3_bind_int(
                    pSqlStatement,
                    2,
                    objectClass);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if ((dwError = sqlite3_step(pSqlStatement)) == SQLITE_DONE)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }
    else
    if (dwError == SQLITE_ROW)
    {
        DWORD dwNumBytes = 0;

        if (sqlite3_column_count(pSqlStatement) != 2)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwNumBytes = sqlite3_column_bytes(pSqlStatement, 0);
        if (dwNumBytes)
        {
            if (dwNumBytes != sizeof(lmHash))
            {
                dwError = LW_ERROR_DATA_ERROR;
                BAIL_ON_SAMDB_ERROR(dwError);
            }
            else
            {
                PCVOID pData = sqlite3_column_blob(pSqlStatement, 0);

                memcpy(&lmHashDbValue[0], pData, dwNumBytes);
            }
        }

        dwNumBytes = sqlite3_column_bytes(pSqlStatement, 1);
        if (dwNumBytes)
        {
            if (dwNumBytes != sizeof(ntHash))
            {
                dwError = LW_ERROR_DATA_ERROR;
                BAIL_ON_SAMDB_ERROR(dwError);
            }
            else
            {
                PCVOID pData = sqlite3_column_blob(pSqlStatement, 1);

                memcpy(&ntHashDbValue[0], pData, dwNumBytes);
            }
        }

        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if (memcmp(&lmHash[0], &lmHashDbValue[0], sizeof(lmHash)) ||
        memcmp(&ntHash[0], &ntHashDbValue[0], sizeof(ntHash)))
    {
        dwError = LW_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

cleanup:

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    LW_SECURE_FREE_STRING(pszPassword);
    DIRECTORY_FREE_STRING(pszUserDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbGetUserCount(
    HANDLE hBindHandle,
    PDWORD pdwNumUsers
    )
{
    return SamDbGetObjectCount(
                hBindHandle,
                SAMDB_OBJECT_CLASS_USER,
                pdwNumUsers);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
