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
 *        samdbgroup.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM Group Management
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

static
DWORD
SamDbBuildGroupSearchSqlQuery(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszSqlQueryTemplate,
    PWSTR                  wszAttributes[],
    PSTR*                  ppszQuery,
    PSAM_DB_COLUMN_VALUE*  ppColumnValueList
    );

static
DWORD
SamDbGroupSearchExecute(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQueryTemplate,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    LONG64                 llObjectRecordId,
    PDIRECTORY_ENTRY*      ppDirectoryEntries,
    PDWORD                 pdwNumEntries
    );

static
DWORD
SamDbCheckExistingMembership_inlock(
    HANDLE hBinding,
    LONG64 llGroupRecordId,
    LONG64 llMemberRecordId
    );

DWORD
SamDbGetGroupCount(
    HANDLE hBindHandle,
    PDWORD pdwNumGroups
    )
{
    return SamDbGetObjectCount(
                hBindHandle,
                SAMDB_OBJECT_CLASS_LOCAL_GROUP,
                pdwNumGroups);
}

DWORD
SamDbGetGroupMembers(
    HANDLE            hBindHandle,
    PWSTR             pwszGroupDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    PCSTR pszSqlQueryTemplate = \
            "  FROM " SAM_DB_OBJECTS_TABLE " sdo" \
            " WHERE sdo." SAM_DB_COL_RECORD_ID \
            "    IN (SELECT " SAM_DB_COL_MEMBER_RECORD_ID \
            "          FROM " SAM_DB_MEMBERS_TABLE " sdm" \
            "         WHERE sdm." SAM_DB_COL_GROUP_RECORD_ID " = ?1);";
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PDIRECTORY_ENTRY     pDirectoryEntries = NULL;
    DWORD   dwNumEntries = 0;
    LONG64  llObjectRecordId = 0;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    PSTR    pszGroupDN = NULL;
    PSTR    pszSqlQuery = NULL;
    BOOLEAN bInLock = FALSE;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    dwError = LwWc16sToMbs(
                    pwszGroupDN,
                    &pszGroupDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetObjectRecordInfo_inlock(
                    pDirectoryContext,
                    pszGroupDN,
                    &llObjectRecordId,
                    &objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (objectClass != SAMDB_OBJECT_CLASS_LOCAL_GROUP)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbBuildGroupSearchSqlQuery(
                    pDirectoryContext,
                    pszSqlQueryTemplate,
                    pwszAttrs,
                    &pszSqlQuery,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbGroupSearchExecute(
                    pDirectoryContext,
                    pszSqlQuery,
                    pColumnValueList,
                    llObjectRecordId,
                    &pDirectoryEntries,
                    &dwNumEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppDirectoryEntries = pDirectoryEntries;
    *pdwNumEntries = dwNumEntries;

cleanup:

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    LW_SAFE_FREE_STRING(pszGroupDN);
    LW_SAFE_FREE_STRING(pszSqlQuery);

    return dwError;

error:

    *ppDirectoryEntries = NULL;
    *pdwNumEntries = 0;

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwNumEntries);
    }

    goto cleanup;
}

DWORD
SamDbGetUserMemberships(
    HANDLE            hBindHandle,
    PWSTR             pwszUserDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    PCSTR pszSqlQueryTemplate = \
            "  FROM " SAM_DB_OBJECTS_TABLE " sdo" \
            " WHERE sdo." SAM_DB_COL_RECORD_ID \
            "    IN (SELECT " SAM_DB_COL_GROUP_RECORD_ID \
            "          FROM " SAM_DB_MEMBERS_TABLE " sdm" \
            "         WHERE sdm." SAM_DB_COL_MEMBER_RECORD_ID " = ?1);";
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PDIRECTORY_ENTRY     pDirectoryEntries = NULL;
    DWORD   dwNumEntries = 0;
    LONG64  llObjectRecordId = 0;
    PSTR    pszUserDN = NULL;
    PSTR    pszSqlQuery = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    BOOLEAN bInLock = FALSE;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    dwError = LwWc16sToMbs(
                    pwszUserDN,
                    &pszUserDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetObjectRecordInfo_inlock(
                    pDirectoryContext,
                    pszUserDN,
                    &llObjectRecordId,
                    &objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (objectClass != SAMDB_OBJECT_CLASS_USER &&
        objectClass != SAMDB_OBJECT_CLASS_LOCALGRP_MEMBER)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbBuildGroupSearchSqlQuery(
                    pDirectoryContext,
                    pszSqlQueryTemplate,
                    pwszAttrs,
                    &pszSqlQuery,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbGroupSearchExecute(
                    pDirectoryContext,
                    pszSqlQuery,
                    pColumnValueList,
                    llObjectRecordId,
                    &pDirectoryEntries,
                    &dwNumEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppDirectoryEntries = pDirectoryEntries;
    *pdwNumEntries = dwNumEntries;

cleanup:

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    LW_SAFE_FREE_STRING(pszUserDN);
    LW_SAFE_FREE_STRING(pszSqlQuery);

    return dwError;

error:

    *ppDirectoryEntries = NULL;
    *pdwNumEntries = 0;

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwNumEntries);
    }

    goto cleanup;
}

#define SAM_DB_GROUP_SEARCH_QUERY_PREFIX          "SELECT "
#define SAM_DB_GROUP_SEARCH_QUERY_FIELD_SEPARATOR ","

static
DWORD
SamDbBuildGroupSearchSqlQuery(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszSqlQueryTemplate,
    PWSTR                  wszAttributes[],
    PSTR*                  ppszQuery,
    PSAM_DB_COLUMN_VALUE*  ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszQueryCursor = NULL;
    PCSTR pszCursor = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pIter = NULL;
    DWORD dwNumAttrs = 0;
    DWORD dwColNamesLen = 0;
    DWORD dwQueryLen = 0;

    while (wszAttributes[dwNumAttrs])
    {
        PWSTR pwszAttrName = wszAttributes[dwNumAttrs];

        PSAM_DB_COLUMN_VALUE  pColumnValue = NULL;
        PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;

        dwError = SamDbAttributeLookupByName(
                        pDirectoryContext->pAttrLookup,
                        pwszAttrName,
                        &pAttrMap);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (!pAttrMap->bIsQueryable)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwError = DirectoryAllocateMemory(
                        sizeof(SAM_DB_COLUMN_VALUE),
                        (PVOID*)&pColumnValue);
        BAIL_ON_SAMDB_ERROR(dwError);

        pColumnValue->pAttrMap = pAttrMap;
        pColumnValue->pNext = pColumnValueList;
        pColumnValueList = pColumnValue;
        pColumnValue = NULL;

        if (dwColNamesLen)
        {
            dwColNamesLen += sizeof(SAM_DB_GROUP_SEARCH_QUERY_FIELD_SEPARATOR)-1;
        }

        dwColNamesLen += strlen(&pAttrMap->szDbColumnName[0]);

        dwNumAttrs++;
    }

    dwQueryLen = sizeof(SAM_DB_GROUP_SEARCH_QUERY_PREFIX) - 1;
    dwQueryLen += dwColNamesLen;
    dwQueryLen += strlen(pszSqlQueryTemplate);
    dwQueryLen++;

    dwError = DirectoryAllocateMemory(
                    dwQueryLen,
                    (PVOID*)&pszQuery);
    BAIL_ON_SAMDB_ERROR(dwError);

    pColumnValueList = SamDbReverseColumnValueList(pColumnValueList);

    pszQueryCursor = pszQuery;
    dwColNamesLen = 0;

    pszCursor = SAM_DB_GROUP_SEARCH_QUERY_PREFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (dwColNamesLen)
        {
            pszCursor = SAM_DB_GROUP_SEARCH_QUERY_FIELD_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
                dwColNamesLen++;
            }
        }

        pszCursor = &pIter->pAttrMap->szDbColumnName[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
            dwColNamesLen++;
        }
    }

    pszCursor = pszSqlQueryTemplate;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    *ppszQuery = pszQuery;
    *ppColumnValueList = pColumnValueList;

cleanup:

    return dwError;

error:

    *ppszQuery = NULL;
    *ppColumnValueList = NULL;

    LW_SAFE_FREE_STRING(pszQuery);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    goto cleanup;
}

static
DWORD
SamDbGroupSearchExecute(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQueryTemplate,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    LONG64                 llObjectRecordId,
    PDIRECTORY_ENTRY*      ppDirectoryEntries,
    PDWORD                 pdwNumEntries
    )
{
    DWORD                dwError = 0;
    PDIRECTORY_ENTRY     pDirectoryEntries = NULL;
    DWORD                dwNumEntries = 0;
    DWORD                dwTotalEntries = 0;
    DWORD                dwEntriesAvailable = 0;
    sqlite3_stmt*        pSqlStatement = NULL;
    DWORD                dwNumCols = 0;
    PSAM_DB_COLUMN_VALUE pIter = NULL;
    PDIRECTORY_ATTRIBUTE pAttrs = NULL;
    DWORD                dwNumAttrs = 0;

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        dwNumCols++;
    }

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQueryTemplate,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    dwError = sqlite3_bind_int64(
                    pSqlStatement,
                    1,
                    llObjectRecordId);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    while ((dwError = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
        DWORD iCol = 0;

        dwNumAttrs = sqlite3_column_count(pSqlStatement);
        if (dwNumAttrs != dwNumCols)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if (!dwEntriesAvailable)
        {
            DWORD dwNewEntryCount = dwTotalEntries + 5;

            dwError = DirectoryReallocMemory(
                            pDirectoryEntries,
                            (PVOID*)&pDirectoryEntries,
                            dwNewEntryCount * sizeof(DIRECTORY_ENTRY));
            BAIL_ON_SAMDB_ERROR(dwError);

            dwEntriesAvailable = dwNewEntryCount - dwTotalEntries;

            memset((PBYTE)pDirectoryEntries + (dwTotalEntries * sizeof(DIRECTORY_ENTRY)),
                   0,
                   dwEntriesAvailable * sizeof(DIRECTORY_ENTRY));

            dwTotalEntries = dwNewEntryCount;
        }

        dwError = DirectoryAllocateMemory(
                        sizeof(DIRECTORY_ATTRIBUTE) * dwNumAttrs,
                        (PVOID*)&pAttrs);
        BAIL_ON_SAMDB_ERROR(dwError);

        for (pIter = pColumnValueList; pIter; pIter = pIter->pNext, iCol++)
        {
            PDIRECTORY_ATTRIBUTE pAttr = &pAttrs[iCol];
            DWORD dwAttrLen = 0;

            dwError = DirectoryAllocateStringW(
                            pIter->pAttrMap->wszDirectoryAttribute,
                            &pAttr->pwszName);
            BAIL_ON_SAMDB_ERROR(dwError);

            switch (pIter->pAttrMap->attributeType)
            {
                case SAMDB_ATTR_TYPE_TEXT:

                    dwAttrLen = sqlite3_column_bytes(pSqlStatement, iCol);
                    if (dwAttrLen)
                    {
                        PATTRIBUTE_VALUE pAttrVal = NULL;

                        const unsigned char* pszStringVal = NULL;

                        pszStringVal = sqlite3_column_text(
                                            pSqlStatement,
                                            iCol);

                        dwError = DirectoryAllocateMemory(
                                        sizeof(ATTRIBUTE_VALUE),
                                        (PVOID*)&pAttr->pValues);
                        BAIL_ON_SAMDB_ERROR(dwError);

                        pAttr->ulNumValues = 1;

                        pAttrVal = &pAttr->pValues[0];

                        pAttrVal->Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;

                        dwError = LwMbsToWc16s(
                                        (PCSTR)pszStringVal,
                                        &pAttrVal->data.pwszStringValue);
                        BAIL_ON_SAMDB_ERROR(dwError);
                    }
                    else
                    {
                        pAttr->ulNumValues = 0;
                    }

                    break;

                case SAMDB_ATTR_TYPE_INT32:
                case SAMDB_ATTR_TYPE_DATETIME:

                    dwError = DirectoryAllocateMemory(
                            sizeof(ATTRIBUTE_VALUE),
                            (PVOID*)&pAttr->pValues);
                    BAIL_ON_SAMDB_ERROR(dwError);

                    pAttr->ulNumValues = 1;

                    pAttr->pValues[0].Type = DIRECTORY_ATTR_TYPE_INTEGER;
                    pAttr->pValues[0].data.ulValue = sqlite3_column_int(
                                                        pSqlStatement,
                                                        iCol);

                    break;

                case SAMDB_ATTR_TYPE_INT64:

                    dwError = DirectoryAllocateMemory(
                                    sizeof(ATTRIBUTE_VALUE),
                                    (PVOID*)&pAttr->pValues);
                    BAIL_ON_SAMDB_ERROR(dwError);

                    pAttr->ulNumValues = 1;

                    pAttr->pValues[0].Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
                    pAttr->pValues[0].data.llValue = sqlite3_column_int64(
                                                        pSqlStatement,
                                                        iCol);

                    break;

                case SAMDB_ATTR_TYPE_BOOLEAN:

                    dwError = DirectoryAllocateMemory(
                                    sizeof(ATTRIBUTE_VALUE),
                                    (PVOID*)&pAttr->pValues);
                    BAIL_ON_SAMDB_ERROR(dwError);

                    pAttr->ulNumValues = 1;

                    pAttr->pValues[0].Type = DIRECTORY_ATTR_TYPE_BOOLEAN;

                    if (sqlite3_column_int(pSqlStatement, iCol))
                    {
                        pAttr->pValues[0].data.bBooleanValue = TRUE;
                    }
                    else
                    {
                        pAttr->pValues[0].data.bBooleanValue = FALSE;
                    }

                    break;

                case SAMDB_ATTR_TYPE_BLOB:

                    dwAttrLen = sqlite3_column_bytes(pSqlStatement, iCol);
                    if (dwAttrLen)
                    {
                        PATTRIBUTE_VALUE pAttrVal = NULL;

                        PCVOID pBlob = sqlite3_column_blob(
                                                pSqlStatement,
                                                iCol);

                        dwError = DirectoryAllocateMemory(
                                        sizeof(ATTRIBUTE_VALUE),
                                        (PVOID*)&pAttr->pValues);
                        BAIL_ON_SAMDB_ERROR(dwError);

                        pAttr->ulNumValues = 1;

                        pAttrVal = &pAttr->pValues[0];

                        pAttrVal->Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM;

                        dwError = DirectoryAllocateMemory(
                                    sizeof(OCTET_STRING),
                                    (PVOID*)&pAttrVal->data.pOctetString);
                        BAIL_ON_SAMDB_ERROR(dwError);

                        dwError = DirectoryAllocateMemory(
                                    dwAttrLen,
                                    (PVOID*)&pAttrVal->data.pOctetString->pBytes);
                        BAIL_ON_SAMDB_ERROR(dwError);

                        memcpy(pAttrVal->data.pOctetString->pBytes,
                                pBlob,
                                dwAttrLen);

                        pAttrVal->data.pOctetString->ulNumBytes = dwAttrLen;
                    }
                    else
                    {
                        pAttr->ulNumValues = 0;
                    }

                    break;

                default:

                    dwError = LW_ERROR_INTERNAL;
                    BAIL_ON_SAMDB_ERROR(dwError);
            }
        }

        pDirectoryEntries[dwNumEntries].ulNumAttributes = dwNumAttrs;
        pDirectoryEntries[dwNumEntries].pAttributes = pAttrs;

        pAttrs = NULL;
        dwNumAttrs = 0;

        dwNumEntries++;
        dwEntriesAvailable--;
    }

    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *ppDirectoryEntries = pDirectoryEntries;
    *pdwNumEntries = dwNumEntries;

cleanup:

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    return dwError;

error:

    *ppDirectoryEntries = NULL;
    *pdwNumEntries = 0;

    if (pAttrs)
    {
        DirectoryFreeAttributes(pAttrs, dwNumAttrs);
    }

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwTotalEntries);
    }

    goto cleanup;
}

DWORD
SamDbAddToGroup(
    HANDLE hBindHandle,
    PWSTR  pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntry
    )
{
    DWORD dwError = 0;
    DWORD iEntry = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    sqlite3_stmt* pSqlStatement = NULL;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    LONG64 llGroupRecordId  = 0;
    LONG64 llMemberRecordId = 0;
    WCHAR   wszAttrNameDistinguishedName[] = SAM_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR   wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    PSTR   pszGroupDN  = NULL;
    PWSTR  pwszMemberDN = NULL;
    PSTR   pszMemberDN = NULL;
    PWSTR  pwszMemberSID = NULL;
    PSTR   pszMemberSID = NULL;
    SAMDB_OBJECT_CLASS groupObjectClass  = SAMDB_OBJECT_CLASS_UNKNOWN;
    SAMDB_OBJECT_CLASS memberObjectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    PCSTR  pszQueryTemplate = "INSERT INTO " SAM_DB_MEMBERS_TABLE            \
                                        " (" SAM_DB_COL_GROUP_RECORD_ID  "," \
                                             SAM_DB_COL_MEMBER_RECORD_ID ")" \
                              " VALUES ( ?1, ?2 )";
    BOOLEAN bInLock = FALSE;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    dwError = LwWc16sToMbs(
                    pwszGroupDN,
                    &pszGroupDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetObjectRecordInfo_inlock(
                    pDirectoryContext,
                    pszGroupDN,
                    &llGroupRecordId,
                    &groupObjectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (groupObjectClass != SAMDB_OBJECT_CLASS_LOCAL_GROUP)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    while (pDirectoryEntry[iEntry].ulNumAttributes &&
           pDirectoryEntry[iEntry].pAttributes) {

        pEntry = &(pDirectoryEntry[iEntry++]);

        dwError = DirectoryGetEntryAttrValueByName(
                        pEntry,
                        wszAttrNameDistinguishedName,
                        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                        &pwszMemberDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (pwszMemberDN) {
            dwError = LwWc16sToMbs(
                            pwszMemberDN,
                            &pszMemberDN);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = SamDbGetObjectRecordInfo_inlock(
                            pDirectoryContext,
                            pszMemberDN,
                            &llMemberRecordId,
                            &memberObjectClass);
            BAIL_ON_SAMDB_ERROR(dwError);

        } else {
            dwError = DirectoryGetEntryAttrValueByName(
                            pEntry,
                            wszAttrNameObjectSID,
                            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                            &pwszMemberSID);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = LwWc16sToMbs(
                            pwszMemberSID,
                            &pszMemberSID);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = SamDbGetObjectRecordInfo_inlock(
                            pDirectoryContext,
                            pszMemberSID,
                            &llMemberRecordId,
                            &memberObjectClass);
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if ((memberObjectClass != SAMDB_OBJECT_CLASS_USER) &&
            (memberObjectClass != SAMDB_OBJECT_CLASS_LOCALGRP_MEMBER))
        {
            dwError = LW_ERROR_NO_SUCH_OBJECT;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwError = SamDbCheckExistingMembership_inlock(
                        hBindHandle,
                        llGroupRecordId,
                        llMemberRecordId);
        if (dwError == ERROR_MEMBER_NOT_IN_GROUP)
        {
            dwError = ERROR_SUCCESS;
        }
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_prepare_v2(
                        pDirectoryContext->pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pSqlStatement,
                        NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError,
                                      pDirectoryContext->pDbContext->pDbHandle);

        dwError = sqlite3_bind_int64(
                        pSqlStatement,
                        1,
                        llGroupRecordId);
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

        dwError = sqlite3_bind_int64(
                        pSqlStatement,
                        2,
                        llMemberRecordId);
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

        dwError = sqlite3_step(pSqlStatement);
        if (dwError == SQLITE_DONE)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);
    }

cleanup:
    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    DIRECTORY_FREE_STRING(pszGroupDN);
    DIRECTORY_FREE_STRING(pszMemberDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbRemoveFromGroup(
    HANDLE hBindHandle,
    PWSTR  pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntry
    )
{
    DWORD dwError = 0;
    DWORD iEntry = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    sqlite3_stmt* pSqlStatement = NULL;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    LONG64 llGroupRecordId  = 0;
    LONG64 llMemberRecordId = 0;
    WCHAR   wszAttrNameDistinguishedName[] = SAM_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR   wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    PSTR   pszGroupDN  = NULL;
    PWSTR  pwszMemberSID = NULL;
    PSTR   pszMemberSID = NULL;
    PWSTR  pwszMemberDN = NULL;
    PSTR   pszMemberDN = NULL;
    SAMDB_OBJECT_CLASS groupObjectClass  = SAMDB_OBJECT_CLASS_UNKNOWN;
    SAMDB_OBJECT_CLASS memberObjectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    PCSTR  pszQueryTemplate = "DELETE FROM " SAM_DB_MEMBERS_TABLE            \
                              " WHERE "  SAM_DB_COL_GROUP_RECORD_ID  " = ?1" \
                              "   AND "  SAM_DB_COL_MEMBER_RECORD_ID " = ?2";
    BOOLEAN bInLock = FALSE;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    dwError = LwWc16sToMbs(
                    pwszGroupDN,
                    &pszGroupDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetObjectRecordInfo_inlock(
                    pDirectoryContext,
                    pszGroupDN,
                    &llGroupRecordId,
                    &groupObjectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (groupObjectClass != SAMDB_OBJECT_CLASS_LOCAL_GROUP)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    while (pDirectoryEntry[iEntry].ulNumAttributes &&
           pDirectoryEntry[iEntry].pAttributes) {

        pEntry = &(pDirectoryEntry[iEntry++]);

        dwError = DirectoryGetEntryAttrValueByName(
                        pEntry,
                        wszAttrNameDistinguishedName,
                        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                        &pwszMemberDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (pwszMemberDN) {
            dwError = LwWc16sToMbs(
                            pwszMemberDN,
                            &pszMemberDN);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = SamDbGetObjectRecordInfo_inlock(
                            pDirectoryContext,
                            pszMemberDN,
                            &llMemberRecordId,
                            &memberObjectClass);
            BAIL_ON_SAMDB_ERROR(dwError);

        } else {
            dwError = DirectoryGetEntryAttrValueByName(
                            pEntry,
                            wszAttrNameObjectSID,
                            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                            &pwszMemberSID);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = LwWc16sToMbs(
                            pwszMemberSID,
                            &pszMemberSID);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = SamDbGetObjectRecordInfo_inlock(
                            pDirectoryContext,
                            pszMemberSID,
                            &llMemberRecordId,
                            &memberObjectClass);
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if ((memberObjectClass != SAMDB_OBJECT_CLASS_USER) &&
            (memberObjectClass != SAMDB_OBJECT_CLASS_LOCALGRP_MEMBER))
        {
            dwError = LW_ERROR_NO_SUCH_OBJECT;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwError = SamDbCheckExistingMembership_inlock(
                        hBindHandle,
                        llGroupRecordId,
                        llMemberRecordId);
        if (dwError == ERROR_MEMBER_IN_GROUP)
        {
            dwError = ERROR_SUCCESS;
        }
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_prepare_v2(
                        pDirectoryContext->pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pSqlStatement,
                        NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

        dwError = sqlite3_bind_int64(
                        pSqlStatement,
                        1,
                        llGroupRecordId);
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

        dwError = sqlite3_bind_int64(
                        pSqlStatement,
                        2,
                        llMemberRecordId);
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

        dwError = sqlite3_step(pSqlStatement);
        if (dwError == SQLITE_DONE)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);
    }

cleanup:
    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    DIRECTORY_FREE_STRING(pszGroupDN);
    DIRECTORY_FREE_STRING(pszMemberDN);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbCheckExistingMembership_inlock(
    HANDLE hBinding,
    LONG64 llGroupRecordId,
    LONG64 llMemberRecordId
    )
{
    DWORD dwError = 0;
    sqlite3_stmt* pSqlStatement = NULL;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PCSTR pszQueryTemplate = "SELECT * FROM " SAM_DB_MEMBERS_TABLE         \
                             " WHERE " SAM_DB_COL_GROUP_RECORD_ID  " = ?1" \
                             "   AND " SAM_DB_COL_MEMBER_RECORD_ID " = ?2";
    DWORD dwRowCount = 0;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBinding;

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQueryTemplate,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    dwError = sqlite3_bind_int64(
                    pSqlStatement,
                    1,
                    llGroupRecordId);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    dwError = sqlite3_bind_int64(
                    pSqlStatement,
                    2,
                    llMemberRecordId);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    while ((dwError = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
        dwRowCount++;
    }

    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    switch (dwRowCount)
    {
    case 0:
        /* No membership records found - member is not in local group */
        dwError = ERROR_MEMBER_NOT_IN_GROUP;
        break;

    case 1:
        /* One membership record found - member is in local group */
        dwError = ERROR_MEMBER_IN_GROUP;
        break;

    default:
        /* More than one membership record found - this cannot happen
           as it would be a primary key violation. */
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        break;
    }

cleanup:
    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
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
