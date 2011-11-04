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
 *        samdbmisc.c
 *
 * Abstract:
 *
 *        Likewise SAM DB
 *
 *        Misc Functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
SamDbComputeLMHash(
    PCSTR pszPassword,
    PBYTE pHash,
    DWORD dwHashByteLen
    )
{
    DWORD dwError = 0;

    if (!pHash || (dwHashByteLen != 16))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    memset(pHash, 0, dwHashByteLen);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbComputeNTHash(
    PCWSTR pwszPassword,
    PBYTE pHash,
    DWORD dwHashByteLen
    )
{
    DWORD dwError = 0;

    if (!pHash || (dwHashByteLen != 16))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    memset(pHash, 0, dwHashByteLen);

    if (pwszPassword)
    {
        MD4((PBYTE)pwszPassword, 
            wc16slen(pwszPassword)*sizeof(WCHAR),
            pHash);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbGetObjectClass(
    DIRECTORY_MOD       Modifications[],
    SAMDB_OBJECT_CLASS* pObjectClass
    )
{
    DWORD dwError = 0;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    wchar16_t pwszObjectClassAttr[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    DWORD dwNumMods = 0;

    while (Modifications[dwNumMods].pwszAttrName &&
           Modifications[dwNumMods].pAttrValues)
    {
        if (!wc16scasecmp(&pwszObjectClassAttr[0],
                          Modifications[dwNumMods].pwszAttrName))
        {
            PATTRIBUTE_VALUE pAttrValue = NULL;

            if (Modifications[dwNumMods].ulNumValues != 1)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            pAttrValue = &Modifications[dwNumMods].pAttrValues[0];

            if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            switch (pAttrValue->data.ulValue)
            {
                case SAMDB_OBJECT_CLASS_DOMAIN:
                case SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN:
                case SAMDB_OBJECT_CLASS_CONTAINER:
                case SAMDB_OBJECT_CLASS_LOCAL_GROUP:
                case SAMDB_OBJECT_CLASS_LOCALGRP_MEMBER:
                case SAMDB_OBJECT_CLASS_USER:

                    objectClass = pAttrValue->data.ulValue;

                    break;

                default:

                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);

                    break;
            }

            break;
        }

        dwNumMods++;
    }

    *pObjectClass = objectClass;

cleanup:

    return dwError;

error:

    *pObjectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    goto cleanup;
}

DWORD
SamDbFindObjectClassMapInfo(
    SAMDB_OBJECT_CLASS                   objectClass,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO  pMapInfos,
    DWORD                                dwNumMapInfos,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO* ppMapInfo
    )
{
    DWORD dwError = 0;
    DWORD iMap = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pMapInfo = NULL;

    for (; iMap < dwNumMapInfos; iMap++)
    {
        PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pIterMapInfo = &pMapInfos[iMap];

        if (pIterMapInfo->objectClass == objectClass)
        {
            pMapInfo = pIterMapInfo;
            break;
        }
    }

    if (!pMapInfo)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppMapInfo = pMapInfo;

cleanup:

    return dwError;

error:

    *ppMapInfo = NULL;

    goto cleanup;
}

PSAM_DB_COLUMN_VALUE
SamDbReverseColumnValueList(
    PSAM_DB_COLUMN_VALUE pColumnValueList
    )
{
    PSAM_DB_COLUMN_VALUE pP = NULL;
    PSAM_DB_COLUMN_VALUE pQ = pColumnValueList;
    PSAM_DB_COLUMN_VALUE pR = NULL;

    while( pQ )
    {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

VOID
SamDbFreeColumnValueList(
    PSAM_DB_COLUMN_VALUE pColValueList
    )
{
    while (pColValueList)
    {
        PSAM_DB_COLUMN_VALUE pTmp = pColValueList;

        pColValueList = pColValueList->pNext;

        if (pTmp->pAttrValues)
        {
            DirectoryFreeAttributeValues(
                    pTmp->pAttrValues,
                    pTmp->ulNumValues);
        }

        DirectoryFreeMemory(pTmp);
    }
}

DWORD
SamDbGetNumberOfDependents_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PDWORD                 pdwNumDependents
    )
{
    DWORD dwError = 0;

    // TODO:

    *pdwNumDependents = 0;

    return dwError;
}

DWORD
SamDbGetObjectCount(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    SAMDB_OBJECT_CLASS     objectClass,
    PDWORD                 pdwNumObjects
    )
{
    DWORD dwError = 0;
    sqlite3_stmt* pSqlStatement = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD   dwNumObjects = 0;

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &gSamGlobals.rwLock);

    if (!pDirectoryContext->pDbContext->pQueryObjectCountStmt)
    {
        PCSTR pszQueryTemplate = "SELECT count(*) FROM " SAM_DB_OBJECTS_TABLE \
                                     " WHERE " SAM_DB_COL_OBJECT_CLASS " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDirectoryContext->pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDirectoryContext->pDbContext->pQueryObjectCountStmt,
                        NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(
                        dwError,
                        pDirectoryContext->pDbContext->pDbHandle);
    }

    pSqlStatement = pDirectoryContext->pDbContext->pQueryObjectCountStmt;

    dwError = sqlite3_bind_int(
                    pSqlStatement,
                    1,
                    objectClass);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwNumObjects = sqlite3_column_int(
                            pSqlStatement,
                            0);

        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pdwNumObjects = dwNumObjects;

cleanup:

    if (pDirectoryContext->pDbContext->pQueryObjectCountStmt)
    {
        sqlite3_reset(pDirectoryContext->pDbContext->pQueryObjectCountStmt);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbGetObjectRecordInfo(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;
    LONG64  llObjectRecordId = 0;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetObjectRecordInfo_inlock(
                    pDirectoryContext,
                    pszObjectDN,
                    &llObjectRecordId,
                    &objectClass);


    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    *pllObjectRecordId = llObjectRecordId;
    *pObjectClass = objectClass;

    return dwError;
}

DWORD
SamDbGetObjectRecordInfo_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    )
{
    DWORD dwError = 0;
    sqlite3_stmt* pSqlStatement = NULL;
    LONG64   llObjectRecordId = 0;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    BAIL_ON_INVALID_POINTER(pszObjectDN);

    if (!pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt)
    {
        PCSTR pszQueryTemplate = "SELECT " SAM_DB_COL_RECORD_ID "," \
                                           SAM_DB_COL_OBJECT_CLASS  \
                                 "  FROM " SAM_DB_OBJECTS_TABLE     \
                                 " WHERE " SAM_DB_COL_DISTINGUISHED_NAME " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDirectoryContext->pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt,
                        NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(
                        dwError,
                        pDirectoryContext->pDbContext->pDbHandle);
    }

    pSqlStatement = pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt;

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    1,
                    pszObjectDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 2)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        llObjectRecordId = sqlite3_column_int64(
                                pSqlStatement,
                                0);

        objectClass = sqlite3_column_int(
                                pSqlStatement,
                                1);

        dwError = LW_ERROR_SUCCESS;
    }
    else if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pllObjectRecordId = llObjectRecordId;
    *pObjectClass = objectClass;

cleanup:

    if (pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt)
    {
        sqlite3_reset(pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt);
    }

    return dwError;

error:

    *pllObjectRecordId = 0;
    *pObjectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    goto cleanup;
}

DWORD
SamDbGetObjectRecordInfoBySID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectSID,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;
    LONG64  llObjectRecordId = 0;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetObjectRecordInfoBySID_inlock(
                    pDirectoryContext,
                    pszObjectSID,
                    &llObjectRecordId,
                    &objectClass);


    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    *pllObjectRecordId = llObjectRecordId;
    *pObjectClass = objectClass;

    return dwError;
}

DWORD
SamDbGetObjectRecordInfoBySID_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectSID,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    )
{
    DWORD dwError = 0;
    sqlite3_stmt* pSqlStatement = NULL;
    LONG64   llObjectRecordId = 0;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    BAIL_ON_INVALID_POINTER(pszObjectSID);

    if (!pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt)
    {
        PCSTR pszQueryTemplate = "SELECT " SAM_DB_COL_RECORD_ID "," \
                                           SAM_DB_COL_OBJECT_CLASS  \
                                 "  FROM " SAM_DB_OBJECTS_TABLE     \
                                 " WHERE " SAM_DB_COL_OBJECT_SID " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDirectoryContext->pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt,
                        NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(
                        dwError,
                        pDirectoryContext->pDbContext->pDbHandle);
    }

    pSqlStatement = pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt;

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    1,
                    pszObjectSID,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 2)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        llObjectRecordId = sqlite3_column_int64(
                                pSqlStatement,
                                0);

        objectClass = sqlite3_column_int(
                                pSqlStatement,
                                1);

        dwError = LW_ERROR_SUCCESS;
    }
    else if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pllObjectRecordId = llObjectRecordId;
    *pObjectClass = objectClass;

cleanup:

    if (pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt)
    {
        sqlite3_reset(pDirectoryContext->pDbContext->pQueryObjectRecordInfoStmt);
    }

    return dwError;

error:

    *pllObjectRecordId = 0;
    *pObjectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    goto cleanup;
}

LONG64
SamDbGetNTTime(
    time_t timeVal
    )
{
    return (timeVal + 11644473600LL) * 10000000LL;
}


DWORD
SamDbIncrementSequenceNumber(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbIncrementSequenceNumber_inlock(pDirectoryContext);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:
    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    return dwError;

error:
    goto cleanup;
}


DWORD
SamDbIncrementSequenceNumber_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PCSTR pszQueryTemplate = "UPDATE " SAM_DB_OBJECTS_TABLE \
                     " SET " SAM_DB_COL_SEQUENCE_NUMBER \
                     " = " SAM_DB_COL_SEQUENCE_NUMBER " + 1 " \
                     " WHERE " SAM_DB_COL_OBJECT_CLASS " = %u";
    PSTR pszQuery = NULL;
    sqlite3_stmt *pSqlStatement = NULL;

    dwError = LwAllocateStringPrintf(&pszQuery,
                                     pszQueryTemplate,
                                     SAMDB_OBJECT_CLASS_DOMAIN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(
                    dwError,
                    pDirectoryContext->pDbContext->pDbHandle);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

cleanup:
    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    DIRECTORY_FREE_MEMORY(pszQuery);

    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
