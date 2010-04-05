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
 *        samdbmodify.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM objects modification routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
SamDbUpdateObjectInDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          modifications[]
    );

static
DWORD
SamDbUpdateBuildObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAM_DB_DN                          pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbUpdateBuildColumnValueList(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbUpdateBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    );

DWORD
SamDbModifyObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    LONG64 llObjectRecordId = 0;
    PSTR   pszObjectDN = NULL;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    dwError = LsaWc16sToMbs(
                    pwszObjectDN,
                    &pszObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamDbGetObjectRecordInfo(
                    pDirectoryContext,
                    pszObjectDN,
                    &llObjectRecordId,
                    &objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSchemaModifyValidateDirMods(
                    pDirectoryContext,
                    objectClass,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbUpdateObjectInDatabase(
                    pDirectoryContext,
                    pwszObjectDN,
                    objectClass,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);

   return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbUpdateObjectInDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          modifications[]
    )
{
    DWORD dwError = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo = NULL;
    PSTR  pszQuery = NULL;
    sqlite3_stmt* pSqlStatement = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bTxStarted = FALSE;
    PSAM_DB_DN pDN = NULL;
    PSTR       pszDN = NULL;

    if (!pDirectoryContext || !pwszObjectDN || !*pwszObjectDN)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbParseDN(
                pwszObjectDN,
                &pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                pDN->pwszDN,
                &pszDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbFindObjectClassMapInfo(
                    objectClass,
                    pDirectoryContext->pObjectClassAttrMaps,
                    pDirectoryContext->dwNumObjectClassAttrMaps,
                    &pObjectClassMapInfo);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbUpdateBuildObjectQuery(
                    pDirectoryContext,
                    pDN,
                    pObjectClassMapInfo,
                    modifications,
                    &pszQuery,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, pDirectoryContext);

    dwError = SamDbUpdateBindValues(
                    pDirectoryContext,
                    pszDN,
                    pColumnValueList,
                    pSqlStatement);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

cleanup:

    SAM_DB_END_TRANSACTION(bTxStarted, dwError, pDirectoryContext);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    DIRECTORY_FREE_STRING(pszQuery);

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    if (pDN)
    {
        SamDbFreeDN(pDN);
    }

    DIRECTORY_FREE_STRING(pszDN);

    return dwError;

error:

    goto cleanup;
}

#define SAMDB_UPDATE_OBJECT_QUERY_PREFIX    "UPDATE " SAM_DB_OBJECTS_TABLE
#define SAMDB_UPDATE_OBJECT_QUERY_SET       " SET "
#define SAMDB_UPDATE_OBJECT_QUERY_EQUALS    " = "
#define SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR ","
#define SAMDB_UPDATE_OBJECT_QUERY_WHERE     " WHERE "
#define SAMDB_UPDATE_OBJECT_QUERY_SUFFIX    ";"

static
DWORD
SamDbUpdateBuildObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAM_DB_DN                          pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszQueryCursor = NULL;
    PSTR  pszCursor = 0;
    DWORD iCol = 0;
    DWORD dwQueryLen = 0;
    DWORD dwColNamesLen = 0;
    CHAR  szBuf[32];
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pIter = NULL;

    dwError = SamDbUpdateBuildColumnValueList(
                    pDirectoryContext,
                    pObjectClassMapInfo,
                    modifications,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    // We are building a query which will look like
    // UPDATE samdbobjects
    //    SET col1 = ?1,
    //        col2 = ?2
    //  WHERE DistinguishedName = ?3;
    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (dwColNamesLen)
        {
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR) - 1;
        }
        else
        {
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_SET) - 1;
        }

        dwColNamesLen += strlen(&pIter->pAttrMap->szDbColumnName[0]);

        dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_EQUALS) - 1;

        sprintf(szBuf, "\?%d", ++iCol);

        dwColNamesLen += strlen(szBuf);
    }

    dwQueryLen = sizeof(SAMDB_UPDATE_OBJECT_QUERY_PREFIX) - 1;
    dwQueryLen += dwColNamesLen;
    dwQueryLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_WHERE) - 1;

    dwQueryLen += sizeof(SAM_DB_COL_DISTINGUISHED_NAME) - 1;
    dwQueryLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_EQUALS) - 1;
    sprintf(szBuf, "\?%d", ++iCol);
    dwQueryLen += strlen(szBuf);
    dwQueryLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_SUFFIX) - 1;
    dwQueryLen++;

    dwError = DirectoryAllocateMemory(
                    dwQueryLen,
                    (PVOID*)&pszQuery);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszQueryCursor = pszQuery;
    iCol = 0;
    dwColNamesLen = 0;

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_PREFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (dwColNamesLen)
        {
            pszCursor = SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR)  - 1;
        }
        else
        {
            pszCursor = SAMDB_UPDATE_OBJECT_QUERY_SET;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_SET)  - 1;
        }

        pszCursor = &pIter->pAttrMap->szDbColumnName[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
            dwColNamesLen++;
        }

        pszCursor = SAMDB_UPDATE_OBJECT_QUERY_EQUALS;
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
        }
        dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_EQUALS)  - 1;

        sprintf(szBuf, "\?%d", ++iCol);
        pszCursor = &szBuf[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
            dwColNamesLen++;
        }
    }

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_WHERE;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    pszCursor = SAM_DB_COL_DISTINGUISHED_NAME;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_EQUALS;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    sprintf(szBuf, "\?%d", ++iCol);
    pszCursor = &szBuf[0];
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_SUFFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    *ppszQuery = pszQuery;
    *ppColumnValueList = pColumnValueList;

cleanup:

    return dwError;

error:

    *ppszQuery = NULL;

    DIRECTORY_FREE_STRING(pszQuery);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    goto cleanup;
}

static
DWORD
SamDbUpdateBuildColumnValueList(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValue = NULL;
    DWORD dwNumMods = 0;
    DWORD iMap = 0;

    //
    // Build column values for the attributes specified by the user
    //
    while (modifications[dwNumMods].pwszAttrName &&
           modifications[dwNumMods].pAttrValues)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAM_DB_COLUMN_VALUE),
                        (PVOID*)&pColumnValue);
        BAIL_ON_SAMDB_ERROR(dwError);

        pColumnValue->pDirMod = &modifications[dwNumMods];

        dwError = SamDbAttributeLookupByName(
                        pDirectoryContext->pAttrLookup,
                        pColumnValue->pDirMod->pwszAttrName,
                        &pColumnValue->pAttrMap);
        BAIL_ON_SAMDB_ERROR(dwError);

        for (iMap = 0; iMap < pObjectClassMapInfo->dwNumMaps; iMap++)
        {
            PSAMDB_ATTRIBUTE_MAP_INFO pMapInfo = NULL;

            pMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

            if (!wc16scasecmp(&pMapInfo->wszAttributeName[0],
                              modifications[dwNumMods].pwszAttrName))
            {
                pColumnValue->pAttrMapInfo = pMapInfo;

                break;
            }
        }
        assert(pColumnValue->pAttrMapInfo != NULL);

        pColumnValue->pNext = pColumnValueList;
        pColumnValueList = pColumnValue;
        pColumnValue = NULL;

        dwNumMods++;
    }

    *ppColumnValueList = SamDbReverseColumnValueList(pColumnValueList);

cleanup:

    return dwError;

error:

    *ppColumnValueList = NULL;

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }
    if (pColumnValue)
    {
        SamDbFreeColumnValueList(pColumnValue);
    }

    goto cleanup;
}

static
DWORD
SamDbUpdateBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pIter = pColumnValueList;
    DWORD iParam = 0;
    PSTR pszValue = NULL;
    PWSTR pwszValue = NULL;

    for (; pIter; pIter = pIter->pNext)
    {
        if (pIter->pAttrValues)
        {
            if (pIter->ulNumValues > 1)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
        }
        else if (pIter->pDirMod)
        {
            if (pIter->ulNumValues > 1)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
        }
        BAIL_ON_SAMDB_ERROR(dwError);

        switch (pIter->pAttrMap->attributeType)
        {
            case SAMDB_ATTR_TYPE_TEXT:
            {
                PATTRIBUTE_VALUE pAttrValue = NULL;

                if (pIter->pAttrValues)
                {
                    pAttrValue = pIter->pAttrValues;
                }
                else if (pIter->pDirMod)
                {
                    pAttrValue = pIter->pDirMod->pAttrValues;
                }

                switch (pAttrValue->Type)
                {
                case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
                    pwszValue = pAttrValue->data.pwszStringValue;
                    break;

                case DIRECTORY_ATTR_TYPE_ANSI_STRING:
                    pszValue = pAttrValue->data.pszStringValue;
                    break;
                }

                if (pwszValue)
                {
                    dwError = LsaWc16sToMbs(pwszValue, &pszValue);
                    BAIL_ON_LSA_ERROR(dwError);
                }

                if (pszValue)
                {
                    dwError = sqlite3_bind_text(
                                    pSqlStatement,
                                    ++iParam,
                                    pszValue,
                                    -1,
                                    SQLITE_TRANSIENT);
                }
                else
                {
                    dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                /* If both pointers are set then it means pszValue was
                   allocated from pwszValue */
                if (pwszValue && pszValue)
                {
                    LW_SAFE_FREE_STRING(pszValue);
                }

                pszValue = NULL;
                pwszValue = NULL;
                break;
            }

            case SAMDB_ATTR_TYPE_INT32:
            case SAMDB_ATTR_TYPE_BOOLEAN:
            case SAMDB_ATTR_TYPE_DATETIME:

                if (pIter->pAttrValues)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pAttrValues[0].data.ulValue);
                }
                else
                if (pIter->pDirMod)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pDirMod->pAttrValues[0].data.ulValue);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                break;

            case SAMDB_ATTR_TYPE_INT64:

                if (pIter->pAttrValues)
                {
                    dwError = sqlite3_bind_int64(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pAttrValues[0].data.llValue);
                }
                else
                if (pIter->pDirMod)
                {
                    dwError = sqlite3_bind_int64(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pDirMod->pAttrValues[0].data.llValue);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                break;

            case SAMDB_ATTR_TYPE_BLOB:
            case SAMDB_ATTR_TYPE_SECURITY_DESCRIPTOR:
            {
                POCTET_STRING pOctetString = NULL;

                if (pIter->pAttrValues)
                {
                    pOctetString = pIter->pAttrValues[0].data.pOctetString;
                }
                else
                if (pIter->pDirMod)
                {
                    pOctetString = pIter->pDirMod->pAttrValues[0].data.pOctetString;
                }

                if (pOctetString)
                {
                    dwError = sqlite3_bind_blob(
                                    pSqlStatement,
                                    ++iParam,
                                    pOctetString->pBytes,
                                    pOctetString->ulNumBytes,
                                    SQLITE_TRANSIENT);
                }
                else
                {
                    dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                break;
            }

            default:

                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
        }
    }

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    ++iParam,
                    pszObjectDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

cleanup:
    /* If both pointers are set then it means pszValue was
       allocated from pwszValue */
    if (pwszValue && pszValue)
    {
        LW_SAFE_FREE_STRING(pszValue);
    }

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
