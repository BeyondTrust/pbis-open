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
 *        samdbsearch.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM objects searching routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
SamDbBuildSqlQuery(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszFilter,
    PWSTR                  wszAttributes[],
    ULONG                  ulAttributesOnly,
    PSTR*                  ppszQuery,
    PBOOLEAN               pbMembersAttrExists,
    PSAM_DB_COLUMN_VALUE*  ppColumnValueList
    );

static
DWORD
SamDbSearchExecute(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQuery,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    ULONG                  ulAttributesOnly,
    PDIRECTORY_ENTRY*      ppDirectoryEntries,
    PDWORD                 pdwNumEntries
    );

static
DWORD
SamDbSearchMarshallResultsAttributes(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQuery,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    ULONG                  ulAttributesOnly,
    PDIRECTORY_ENTRY*      ppDirectoryEntries,
    PDWORD                 pdwNumEntries
    );

static
DWORD
SamDbSearchMarshallResultsAttributesValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQuery,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    ULONG                  ulAttributesOnly,
    PDIRECTORY_ENTRY*      ppDirectoryEntries,
    PDWORD                 pdwNumEntries
    );

DWORD
SamDbSearchObject(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SAMDB_LOCK_RWMUTEX_SHARED(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbSearchObject_inlock(
                    hDirectory,
                    pwszBase,
                    ulScope,
                    pwszFilter,
                    wszAttributes,
                    ulAttributesOnly,
                    ppDirectoryEntries,
                    pdwNumEntries);

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    return dwError;
}

DWORD
SamDbSearchObject_inlock(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PSTR  pszQuery = NULL;
    BOOLEAN bMembersAttrExists = FALSE;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PDIRECTORY_ENTRY pDirectoryEntries = NULL;
    DWORD            dwNumEntries = 0;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbBuildSqlQuery(
                    pDirectoryContext,
                    pwszFilter,
                    wszAttributes,
                    ulAttributesOnly,
                    &pszQuery,
                    &bMembersAttrExists,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSearchExecute(
                    pDirectoryContext,
                    pszQuery,
                    pColumnValueList,
                    ulAttributesOnly,
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

    DIRECTORY_FREE_STRING(pszQuery);

    return(dwError);

error:

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwNumEntries);
    }

    goto cleanup;
}

#define SAM_DB_SEARCH_QUERY_PREFIX          "SELECT "
#define SAM_DB_SEARCH_QUERY_FIELD_SEPARATOR ","
#define SAM_DB_SEARCH_QUERY_FROM            " FROM " SAM_DB_OBJECTS_TABLE " "
#define SAM_DB_SEARCH_QUERY_WHERE           " WHERE "
#define SAM_DB_SEARCH_QUERY_SUFFIX          ";"

static
DWORD
SamDbBuildSqlQuery(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszFilter,
    PWSTR                  wszAttributes[],
    ULONG                  ulAttributesOnly,
    PSTR*                  ppszQuery,
    PBOOLEAN               pbMembersAttrExists,
    PSAM_DB_COLUMN_VALUE*  ppColumnValueList
    )
{
    DWORD dwError = 0;
    BOOLEAN bMembersAttrExists = FALSE;
    DWORD dwQueryLen = 0;
    DWORD dwColNamesLen = 0;
    DWORD dwNumAttrs = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszQueryCursor = NULL;
    PSTR  pszCursor = NULL;
    PSTR  pszFilter = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pIter = NULL;

    if (pwszFilter)
    {
        dwError = LwWc16sToMbs(
                        pwszFilter,
                        &pszFilter);
        BAIL_ON_SAMDB_ERROR(dwError);

        LwStripWhitespace(pszFilter, TRUE, TRUE);
    }

    while (wszAttributes[dwNumAttrs])
    {
        PWSTR pwszAttrName = wszAttributes[dwNumAttrs];
        wchar16_t wszMembersAttrName[] = SAM_DB_DIR_ATTR_MEMBERS;

        if (!wc16scasecmp(pwszAttrName, &wszMembersAttrName[0]))
        {
            bMembersAttrExists = TRUE;
        }
        else
        {
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
                dwColNamesLen += sizeof(SAM_DB_SEARCH_QUERY_FIELD_SEPARATOR)-1;
            }

            dwColNamesLen += strlen(&pAttrMap->szDbColumnName[0]);
        }

        dwNumAttrs++;
    }

    dwQueryLen = sizeof(SAM_DB_SEARCH_QUERY_PREFIX) - 1;
    dwQueryLen += dwColNamesLen;
    dwQueryLen += sizeof(SAM_DB_SEARCH_QUERY_FROM) - 1;

    if (pszFilter && *pszFilter)
    {
        dwQueryLen += sizeof(SAM_DB_SEARCH_QUERY_WHERE) - 1;
        dwQueryLen += strlen(pszFilter);
    }
    dwQueryLen += sizeof(SAM_DB_SEARCH_QUERY_SUFFIX) - 1;
    dwQueryLen++;

    dwError = DirectoryAllocateMemory(
                    dwQueryLen,
                    (PVOID*)&pszQuery);
    BAIL_ON_SAMDB_ERROR(dwError);

    pColumnValueList = SamDbReverseColumnValueList(pColumnValueList);

    pszQueryCursor = pszQuery;
    dwColNamesLen = 0;

    pszCursor = SAM_DB_SEARCH_QUERY_PREFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (dwColNamesLen)
        {
            pszCursor = SAM_DB_SEARCH_QUERY_FIELD_SEPARATOR;
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

    pszCursor = SAM_DB_SEARCH_QUERY_FROM;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    if (pszFilter && *pszFilter)
    {
        pszCursor = SAM_DB_SEARCH_QUERY_WHERE;
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
        }

        pszCursor = pszFilter;
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
        }
    }

    pszCursor = SAM_DB_SEARCH_QUERY_SUFFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    *ppszQuery = pszQuery;
    *pbMembersAttrExists = bMembersAttrExists;
    *ppColumnValueList = pColumnValueList;

cleanup:

    DIRECTORY_FREE_STRING(pszFilter);

    return dwError;

error:

    *ppszQuery = NULL;
    *pbMembersAttrExists = FALSE;
    *ppColumnValueList = NULL;

    DIRECTORY_FREE_STRING(pszQuery);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    goto cleanup;
}

static
DWORD
SamDbSearchExecute(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQuery,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    ULONG                  ulAttributesOnly,
    PDIRECTORY_ENTRY*      ppDirectoryEntries,
    PDWORD                 pdwNumEntries
    )
{
    DWORD dwError = 0;

    if (ulAttributesOnly)
    {
        dwError = SamDbSearchMarshallResultsAttributes(
                        pDirectoryContext,
                        pszQuery,
                        pColumnValueList,
                        ulAttributesOnly,
                        ppDirectoryEntries,
                        pdwNumEntries);
    }
    else
    {
        dwError = SamDbSearchMarshallResultsAttributesValues(
                        pDirectoryContext,
                        pszQuery,
                        pColumnValueList,
                        ulAttributesOnly,
                        ppDirectoryEntries,
                        pdwNumEntries);
    }

    return dwError;
}

static
DWORD
SamDbSearchMarshallResultsAttributes(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQuery,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    ULONG                  ulAttributesOnly,
    PDIRECTORY_ENTRY*      ppDirectoryEntries,
    PDWORD                 pdwNumEntries
    )
{
    DWORD                dwError = 0;
    PDIRECTORY_ENTRY     pDirectoryEntries = NULL;
    PDIRECTORY_ENTRY     pDirEntry = NULL;
    DWORD                dwNumEntries = 0;
    DWORD                iCol = 0;
    PSAM_DB_COLUMN_VALUE pIter = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(DIRECTORY_ENTRY),
                    (PVOID*)&pDirectoryEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwNumEntries = 1;

    pDirEntry = &pDirectoryEntries[0];

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        pDirEntry->ulNumAttributes++;
    }

    dwError = DirectoryAllocateMemory(
                    sizeof(DIRECTORY_ATTRIBUTE) * pDirEntry->ulNumAttributes,
                    (PVOID*)&pDirEntry->pAttributes);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext, iCol++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pDirEntry->pAttributes[iCol];

        dwError = DirectoryAllocateStringW(
                        pIter->pAttrMap->wszDirectoryAttribute,
                        &pAttr->pwszName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = DirectoryAllocateMemory(
                        sizeof(ATTRIBUTE_VALUE),
                        (PVOID*)&pAttr->pValues);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttr->ulNumValues = 1;

        switch (pIter->pAttrMap->attributeType)
        {
            case SAMDB_ATTR_TYPE_TEXT:

                pAttr->pValues[0].Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;

                break;

            case SAMDB_ATTR_TYPE_INT32:
            case SAMDB_ATTR_TYPE_BOOLEAN:
            case SAMDB_ATTR_TYPE_DATETIME:

                pAttr->pValues[0].Type = DIRECTORY_ATTR_TYPE_INTEGER;

                break;

            case SAMDB_ATTR_TYPE_INT64:

                pAttr->pValues[0].Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;

                break;

            case SAMDB_ATTR_TYPE_BLOB:

                pAttr->pValues[0].Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM;

                break;

            default:

                dwError = LW_ERROR_DATA_ERROR;
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
        }
    }

    *ppDirectoryEntries = pDirectoryEntries;
    *pdwNumEntries = dwNumEntries;

cleanup:

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

static
DWORD
SamDbSearchMarshallResultsAttributesValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszQuery,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    ULONG                  ulAttributesOnly,
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
                    pszQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

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

            memset((PBYTE)pDirectoryEntries+(dwTotalEntries * sizeof(DIRECTORY_ENTRY)),
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
                case SAMDB_ATTR_TYPE_SECURITY_DESCRIPTOR:

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

                        if (pIter->pAttrMap->attributeType == SAMDB_ATTR_TYPE_BLOB)
                        {
                            pAttrVal->Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM;
                        }
                        else
                        {
                            pAttrVal->Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR;
                        }

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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
