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
 *        samdbadd.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM objects creation routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

typedef DWORD (*PFN_SAMDB_ADD_VALUE_GENERATOR)(
                    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
                    PWSTR                  pwszDN,
                    PWSTR                  pwszParentDN,
                    PWSTR                  pwszObjectName,
                    PWSTR                  pwszDomainName,
                    PATTRIBUTE_VALUE*      ppAttrValues,
                    PDWORD                 pdwNumValues
                    );

typedef struct _SAMDB_ADD_VALUE_GENERATOR
{
    PSTR                          pszDbColName;
    PFN_SAMDB_ADD_VALUE_GENERATOR pfnValueGenerator;

} SAMDB_ADD_VALUE_GENERATOR, *PSAMDB_ADD_VALUE_GENERATOR;

static
DWORD
SamDbInsertObjectToDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          modifications[]
    );

static
DWORD
SamDbValidateIds(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    DWORD                  objectClass,
    PDIRECTORY_MOD         pMods
    );

static
DWORD
SamDbBuildAddObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbBuildAddColumnValueList(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbAddGenerateValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_DN             pDN,
    SAMDB_OBJECT_CLASS     objectClass,
    PSAM_DB_COLUMN_VALUE   pColumnValueList
    );

static
DWORD
SamDbAddGenerateUID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateGID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateObjectSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateParentDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateDomain(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGeneratePrimaryGroup(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateAccountFlags(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateLogonHours(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddGenerateSecurityDescriptor(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    );

static
DWORD
SamDbAddConvertUnicodeAttrValues(
    PATTRIBUTE_VALUE  pSrcValues,
    DWORD             dwSrcNumValues,
    PATTRIBUTE_VALUE* ppAttrValues,
    PDWORD            pdwNumValues
    );

static
DWORD
SamDbAddBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    );

static
DWORD
SamDbFindDomainSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDomainName,
    PSTR*                  ppszDomainSID
    );

static SAMDB_ADD_VALUE_GENERATOR gSamDbValueGenerators[] =
{
    {
        SAM_DB_COL_UID,
        &SamDbAddGenerateUID
    },
    {
        SAM_DB_COL_GID,
        &SamDbAddGenerateGID
    },
    {
        SAM_DB_COL_OBJECT_SID,
        &SamDbAddGenerateObjectSID
    },
    {
        SAM_DB_COL_PARENT_DN,
        &SamDbAddGenerateParentDN
    },
    {
        SAM_DB_COL_DISTINGUISHED_NAME,
        &SamDbAddGenerateDN
    },
    {
        SAM_DB_COL_DOMAIN,
        &SamDbAddGenerateDomain
    },
    {
        SAM_DB_COL_PRIMARY_GROUP,
        &SamDbAddGeneratePrimaryGroup
    },
    {
        SAM_DB_COL_ACCOUNT_FLAGS,
        &SamDbAddGenerateAccountFlags
    },
    {
        SAM_DB_COL_LOGON_HOURS,
        &SamDbAddGenerateLogonHours
    },
    {
        SAM_DB_COL_SECURITY_DESCRIPTOR,
        &SamDbAddGenerateSecurityDescriptor
    }
};

DWORD
SamDbAddObject(
    HANDLE        hBindHandle,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    dwError = SamDbGetObjectClass(
                    modifications,
                    &objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSchemaAddValidateDirMods(
                    pDirectoryContext,
                    objectClass,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbInsertObjectToDatabase(
                    pDirectoryContext,
                    pwszObjectDN,
                    objectClass,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbIncrementSequenceNumber(
                    pDirectoryContext);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

   return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbInsertObjectToDatabase(
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

    dwError = SamDbParseDN(
                pwszObjectDN,
                &pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbFindObjectClassMapInfo(
                    objectClass,
                    pDirectoryContext->pObjectClassAttrMaps,
                    pDirectoryContext->dwNumObjectClassAttrMaps,
                    &pObjectClassMapInfo);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (objectClass == SAMDB_OBJECT_CLASS_USER ||
        objectClass == SAMDB_OBJECT_CLASS_LOCAL_GROUP)
    {
        dwError = SamDbValidateIds(pDirectoryContext,
                                   objectClass,
                                   modifications);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbBuildAddObjectQuery(
                    pDirectoryContext,
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

    dwError = SamDbAddGenerateValues(
                    pDirectoryContext,
                    pDN,
                    objectClass,
                    pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddBindValues(
                    pDirectoryContext,
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

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbValidateIds(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    DWORD                  objectClass,
    PDIRECTORY_MOD         pMods
    )
{
    DWORD dwError = 0;
    DWORD iMod = 0;
    PDIRECTORY_MOD pMod = &(pMods[iMod]);
    WCHAR wszAttrObjectSid[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrUid[] = SAM_DB_DIR_ATTR_UID;
    WCHAR wszAttrGid[] = SAM_DB_DIR_ATTR_GID;
    PSTR pszSid = NULL;
    DWORD dwUID = 0;
    DWORD dwGID = 0;

    while (pMod->pwszAttrName)
    {
        if (!wc16scasecmp(pMod->pwszAttrName, wszAttrObjectSid))
        {
            if (pMod->pAttrValues[0].Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
            {
                dwError = LwWc16sToMbs(
                                 pMod->pAttrValues[0].data.pwszStringValue,
                                 &pszSid);
            }
            else if (pMod->pAttrValues[0].Type == DIRECTORY_ATTR_TYPE_ANSI_STRING)
            {
                dwError = LwAllocateString(
                                 pMod->pAttrValues[0].data.pszStringValue,
                                 &pszSid);
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = SamDbCheckAvailableSID(pDirectoryContext,
                                             pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (objectClass == SAMDB_OBJECT_CLASS_USER &&
                 !wc16scasecmp(pMod->pwszAttrName, wszAttrUid))
        {
            dwUID = pMod->pAttrValues[0].data.ulValue;

            dwError = SamDbCheckAvailableUID(pDirectoryContext,
                                             dwUID);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (objectClass == SAMDB_OBJECT_CLASS_LOCAL_GROUP &&
                 !wc16scasecmp(pMod->pwszAttrName, wszAttrGid))
        {
            dwGID = pMod->pAttrValues[0].data.ulValue;

            dwError = SamDbCheckAvailableGID(pDirectoryContext,
                                             dwGID);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pMod = &(pMods[++iMod]);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pszSid);

    return dwError;

error:
    goto cleanup;
}


#define SAMDB_ADD_OBJECT_QUERY_ROWID     "rowid"
#define SAMDB_ADD_OBJECT_QUERY_SEPARATOR ","
#define SAMDB_ADD_OBJECT_QUERY_PREFIX    "INSERT INTO " SAM_DB_OBJECTS_TABLE "("
#define SAMDB_ADD_OBJECT_QUERY_MEDIAN    ") VALUES ("
#define SAMDB_ADD_OBJECT_QUERY_SUFFIX    ");"

static
DWORD
SamDbBuildAddObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszQueryCursor = NULL;
    PSTR  pszQueryValuesCursor = NULL;
    DWORD dwQueryValuesOffset = 0;
    PSTR  pszCursor = 0;
    DWORD iCol = 0;
    DWORD dwQueryLen = 0;
    DWORD dwColNamesLen = 0;
    DWORD dwColValuesLen = 0;
    CHAR  szBuf[32];
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pIter = NULL;

    dwError = SamDbBuildAddColumnValueList(
                    pDirectoryContext,
                    pObjectClassMapInfo,
                    modifications,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    //
    // We are building a query which will look like
    // INSERT INTO samdbobjects (col1,col2,col3) VALUES (?1,?2,?3);
    //
    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (pIter->pAttrMap->bIsRowId ||
            pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATED_BY_DB)
        {
            continue;
        }

        if (dwColNamesLen)
        {
            dwColNamesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR) - 1;
        }

        dwColNamesLen += strlen(&pIter->pAttrMap->szDbColumnName[0]);

        if (dwColValuesLen)
        {
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR) - 1;
        }

        sprintf(szBuf, "\?%u", ++iCol);

        dwColValuesLen += strlen(szBuf);
    }

    dwQueryLen = sizeof(SAMDB_ADD_OBJECT_QUERY_PREFIX) - 1;
    dwQueryLen += dwColNamesLen;
    dwQueryLen += sizeof(SAMDB_ADD_OBJECT_QUERY_MEDIAN) - 1;
    dwQueryLen += dwColValuesLen;
    dwQueryLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SUFFIX) - 1;
    dwQueryLen++;

    dwQueryValuesOffset = sizeof(SAMDB_ADD_OBJECT_QUERY_PREFIX) - 1;
    dwQueryValuesOffset += dwColNamesLen;
    dwQueryValuesOffset += sizeof(SAMDB_ADD_OBJECT_QUERY_MEDIAN) - 1;

    dwError = DirectoryAllocateMemory(
                    dwQueryLen,
                    (PVOID*)&pszQuery);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszQueryCursor = pszQuery;
    pszQueryValuesCursor = pszQuery + dwQueryValuesOffset;
    iCol = 0;
    dwColNamesLen = 0;
    dwColValuesLen = 0;

    pszCursor = SAMDB_ADD_OBJECT_QUERY_PREFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (pIter->pAttrMap->bIsRowId ||
            pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATED_BY_DB)
        {
            continue;
        }

        if (dwColNamesLen)
        {
            pszCursor = SAMDB_ADD_OBJECT_QUERY_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColNamesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR)  - 1;
        }

        pszCursor = &pIter->pAttrMap->szDbColumnName[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
            dwColNamesLen++;
        }

        if (dwColValuesLen)
        {
            pszCursor = SAMDB_ADD_OBJECT_QUERY_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryValuesCursor++ = *pszCursor++;
            }
            dwColValuesLen += sizeof(SAMDB_ADD_OBJECT_QUERY_SEPARATOR)  - 1;
        }

        sprintf(szBuf, "\?%u", ++iCol);

        pszCursor = &szBuf[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryValuesCursor++ = *pszCursor++;
            dwColValuesLen++;
        }
    }

    pszCursor = SAMDB_ADD_OBJECT_QUERY_MEDIAN;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    pszCursor = SAMDB_ADD_OBJECT_QUERY_SUFFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryValuesCursor++ = *pszCursor++;
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
SamDbBuildAddColumnValueList(
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

        if (!pColumnValue->pAttrMapInfo)
        {
            dwError = LW_ERROR_NO_SUCH_ATTRIBUTE;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        pColumnValue->pNext = pColumnValueList;
        pColumnValueList = pColumnValue;
        pColumnValue = NULL;

        dwNumMods++;
    }

    //
    // Find attributes that are mandatory, are not specified by the user
    // and are set to be generated
    //
    for(iMap = 0; iMap < pObjectClassMapInfo->dwNumMaps; iMap++)
    {
        PSAMDB_ATTRIBUTE_MAP_INFO pMapInfo = NULL;

        pMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

        if (pMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_MANDATORY)
        {
            PSAM_DB_COLUMN_VALUE pIter = NULL;
            BOOLEAN bFound = FALSE;

            for (pIter = pColumnValueList;
                 !bFound && pIter;
                 pIter = pIter->pNext)
            {
                if (!wc16scasecmp(&pIter->pAttrMapInfo->wszAttributeName[0],
                                  &pMapInfo->wszAttributeName[0]))
                {
                    bFound = TRUE;
                }
            }

            if (!bFound)
            {
                PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;

                dwError = SamDbAttributeLookupByName(
                                pDirectoryContext->pAttrLookup,
                                &pMapInfo->wszAttributeName[0],
                                &pAttrMap);
                BAIL_ON_SAMDB_ERROR(dwError);

                dwError = DirectoryAllocateMemory(
                                sizeof(SAM_DB_COLUMN_VALUE),
                                (PVOID*)&pColumnValue);
                BAIL_ON_SAMDB_ERROR(dwError);

                pColumnValue->pAttrMap = pAttrMap;
                pColumnValue->pAttrMapInfo = pMapInfo;

                pColumnValue->pNext = pColumnValueList;
                pColumnValueList = pColumnValue;
                pColumnValue = NULL;
            }
        }
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
SamDbAddGenerateValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_DN             pDN,
    SAMDB_OBJECT_CLASS     objectClass,
    PSAM_DB_COLUMN_VALUE   pColumnValueList
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pIter = pColumnValueList;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszParentDN = NULL;

    dwError = SamDbGetDNComponents(
                    pDN,
                    &pwszObjectName,
                    &pwszDomainName,
                    &pwszParentDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (; pIter; pIter = pIter->pNext)
    {
        if (!pIter->pDirMod &&
            pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATED_BY_DB)
        {
            continue;
        }

        if (!pIter->pDirMod &&
            ((pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED) ||
             (pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATE_ALWAYS) ||
             (pIter->pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_DERIVATIVE)))
        {
            PFN_SAMDB_ADD_VALUE_GENERATOR pfnValueGenerator = NULL;
            DWORD dwNumGenerators = sizeof(gSamDbValueGenerators)/sizeof(gSamDbValueGenerators[0]);
            DWORD iGen = 0;

            for (; !pfnValueGenerator && (iGen < dwNumGenerators); iGen++)
            {
                if (!strcasecmp(gSamDbValueGenerators[iGen].pszDbColName,
                                &pIter->pAttrMap->szDbColumnName[0]))
                {
                    pfnValueGenerator = gSamDbValueGenerators[iGen].pfnValueGenerator;
                }
            }

            if (!pfnValueGenerator)
            {
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            dwError = pfnValueGenerator(
                            pDirectoryContext,
                            pDN->pwszDN,
                            pwszParentDN,
                            pwszObjectName,
                            pwszDomainName,
                            &pIter->pAttrValues,
                            &pIter->ulNumValues);
            BAIL_ON_SAMDB_ERROR(dwError);
        }
        else
        if (pIter->pDirMod && pIter->pDirMod->pAttrValues)
        {
            dwError = SamDbAddConvertUnicodeAttrValues(
                            pIter->pDirMod->pAttrValues,
                            pIter->pDirMod->ulNumValues,
                            &pIter->pAttrValues,
                            &pIter->ulNumValues);
            BAIL_ON_SAMDB_ERROR(dwError);
        }
        else
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }



    }

cleanup:

    DIRECTORY_FREE_MEMORY(pwszObjectName);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszParentDN);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddGenerateUID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;

    dwError = SamDbGetNextAvailableUID(
                    pDirectoryContext,
                    &pAttrValue->data.ulValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateGID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;

    dwError = SamDbGetNextAvailableGID(
                    pDirectoryContext,
                    &pAttrValue->data.ulValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateObjectSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    DWORD dwRID = 0;
    PSTR  pszDomainSID = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    if (!pwszDomainName || !*pwszDomainName)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbFindDomainSID(
                    pDirectoryContext,
                    pwszDomainName,
                    &pszDomainSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    dwError = SamDbGetNextAvailableRID(
                    pDirectoryContext,
                    &dwRID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pAttrValue->data.pszStringValue,
                    "%s-%u",
                    pszDomainSID,
                    dwRID);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    DIRECTORY_FREE_STRING(pszDomainSID);

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateParentDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    if (pwszParentDN)
    {
        dwError = LwWc16sToMbs(
                        pwszParentDN,
                        &pAttrValue->data.pszStringValue);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateDN(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    if (pwszDN)
    {
        dwError = LwWc16sToMbs(
                        pwszDN,
                        &pAttrValue->data.pszStringValue);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateDomain(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

    if (pwszDomainName)
    {
        dwError = LwWc16sToMbs(
                        pwszDomainName,
                        &pAttrValue->data.pszStringValue);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppAttrValues  = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGeneratePrimaryGroup(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;
    pAttrValue->data.ulValue = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_LW_USERS);

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateAccountFlags(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
		    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;
    pAttrValue->data.ulValue = SAMDB_ACB_NORMAL | SAMDB_ACB_DISABLED;

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:

    return dwError;

error:
    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateLogonHours(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    POCTET_STRING pLogonHoursBlob = NULL;
    DWORD dwHoursPerWeek = 7 * 24;
    PBYTE pLogonHours = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    OUT_PPVOID(&pAttrValue));
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = DirectoryAllocateMemory(
                    sizeof(*pLogonHoursBlob),
                    OUT_PPVOID(&pLogonHoursBlob));
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = DirectoryAllocateMemory(
                    dwHoursPerWeek,
                    OUT_PPVOID(&pLogonHours));
    BAIL_ON_SAMDB_ERROR(dwError);

    /* Set all bytes to 1 - this allows logging in all week long */
    memset(pLogonHours, 1, dwHoursPerWeek);

    pLogonHoursBlob->pBytes     = pLogonHours;
    pLogonHoursBlob->ulNumBytes = dwHoursPerWeek;

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM;
    pAttrValue->data.pOctetString = pLogonHoursBlob;

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:
    return dwError;

error:
    *ppAttrValues = pAttrValue;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddGenerateSecurityDescriptor(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDN,
    PWSTR                  pwszParentDN,
    PWSTR                  pwszObjectName,
    PWSTR                  pwszDomainName,
    PATTRIBUTE_VALUE*      ppAttrValues,
    PDWORD                 pdwNumValues
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    POCTET_STRING pSecDescBlob = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    ULONG ulSecDescLen = 0;
    PSID pOwnerSid = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE),
                    (PVOID*)&pAttrValue);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = DirectoryAllocateMemory(
                    sizeof(*pSecDescBlob),
                    (PVOID*)&pSecDescBlob);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbCreateNewLocalAccountSecDesc(pOwnerSid,
                                                &pSecDesc,
                                                &ulSecDescLen);
    BAIL_ON_SAMDB_ERROR(dwError);

    pSecDescBlob->pBytes     = (PBYTE)pSecDesc;
    pSecDescBlob->ulNumBytes = ulSecDescLen;

    pAttrValue->Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR;
    pAttrValue->data.pOctetString = pSecDescBlob;

    *ppAttrValues = pAttrValue;
    *pdwNumValues = 1;

cleanup:
    return dwError;

error:
    *ppAttrValues = pAttrValue;
    *pdwNumValues = 0;

    if (pAttrValue)
    {
        DirectoryFreeAttributeValues(pAttrValue, 1);
    }

    goto cleanup;
}

static
DWORD
SamDbAddConvertUnicodeAttrValues(
    PATTRIBUTE_VALUE  pSrcValues,
    DWORD             dwSrcNumValues,
    PATTRIBUTE_VALUE* ppAttrValues,
    PDWORD            pdwNumValues
    )
{
    DWORD dwError = 0;
    DWORD iValue = 0;
    PATTRIBUTE_VALUE pTgtValues = NULL;
    DWORD dwNumValues = 0;

    dwError = DirectoryAllocateMemory(
                dwSrcNumValues * sizeof(ATTRIBUTE_VALUE),
                (PVOID*)&pTgtValues);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwNumValues = dwSrcNumValues;

    for (; iValue < dwNumValues; iValue++)
    {
        PATTRIBUTE_VALUE pSrcValue = NULL;
        PATTRIBUTE_VALUE pTgtValue = NULL;

        pSrcValue = &pSrcValues[iValue];
        pTgtValue = &pTgtValues[iValue];

        switch (pSrcValue->Type)
        {
            case DIRECTORY_ATTR_TYPE_ANSI_STRING:

                if (pSrcValue->data.pszStringValue)
                {
                    dwError = DirectoryAllocateString(
                                    pSrcValue->data.pszStringValue,
                                    &pTgtValue->data.pszStringValue);
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

                break;

            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:

                if (pSrcValue->data.pwszStringValue)
                {
                    dwError = LwWc16sToMbs(
                                    pSrcValue->data.pwszStringValue,
                                    &pTgtValue->data.pszStringValue);
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;

                break;


            case DIRECTORY_ATTR_TYPE_INTEGER:

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_INTEGER;

                pTgtValue->data.ulValue = pSrcValue->data.ulValue;

                break;

            case DIRECTORY_ATTR_TYPE_BOOLEAN:

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_BOOLEAN;

                pTgtValue->data.ulValue = pSrcValue->data.ulValue;

                break;

            case DIRECTORY_ATTR_TYPE_LARGE_INTEGER:

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;

                pTgtValue->data.llValue = pSrcValue->data.llValue;

                break;

            case DIRECTORY_ATTR_TYPE_OCTET_STREAM:
            case DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR:

                pTgtValue->Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM;

                dwError = DirectoryAllocateMemory(
                            sizeof(OCTET_STRING),
                            (PVOID*)&pTgtValue->data.pOctetString);
                BAIL_ON_SAMDB_ERROR(dwError);

                if (pSrcValue->data.pOctetString->ulNumBytes)
                {
                    dwError = DirectoryAllocateMemory(
                                    pSrcValue->data.pOctetString->ulNumBytes,
                                    (PVOID*)&pTgtValue->data.pOctetString->pBytes);
                    BAIL_ON_SAMDB_ERROR(dwError);

                    pTgtValue->data.pOctetString->ulNumBytes = pSrcValue->data.pOctetString->ulNumBytes;

                    memcpy( pTgtValue->data.pOctetString->pBytes,
                            pSrcValue->data.pOctetString->pBytes,
                            pSrcValue->data.pOctetString->ulNumBytes);
                }

                break;

            default:

                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);

        }
    }

    *ppAttrValues = pTgtValues;
    *pdwNumValues = dwNumValues;

cleanup:

    return dwError;

error:

    *ppAttrValues = NULL;
    *pdwNumValues = 0;

    if (pTgtValues)
    {
        DirectoryFreeAttributeValues(pTgtValues, dwNumValues);
    }

    goto cleanup;
}

static
DWORD
SamDbAddBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pIter = pColumnValueList;
    DWORD iParam = 0;

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
                PSTR pszValue = NULL;

                if (pIter->pAttrValues)
                {
                    pszValue = pIter->pAttrValues[0].data.pszStringValue;
                }
                else
                if (pIter->pDirMod)
                {
                    pszValue = pIter->pDirMod->pAttrValues[0].data.pszStringValue;
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

            case SAMDB_ATTR_TYPE_SECURITY_DESCRIPTOR:
            case SAMDB_ATTR_TYPE_BLOB:
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

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbFindDomainSID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszDomainName,
    PSTR*                  ppszDomainSID
    )
{
    DWORD dwError = 0;
    wchar16_t wszSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttributes[] =
                {
                    &wszSID[0],
                    NULL
                };
    PDIRECTORY_ENTRY pDirEntries = NULL;
    DWORD dwNumEntries = 0;
    PSTR  pszFilter    = NULL;
    PWSTR pwszFilter   = NULL;
    PSTR  pszDomainSID = NULL;
    PSTR  pszDomainName = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_DOMAIN;
    PCSTR pszQueryClause = SAM_DB_COL_DOMAIN       " = \"%s\"" \
                           "   AND " SAM_DB_COL_OBJECT_CLASS " = %u;";

    dwError = LwWc16sToMbs(
                    pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszQueryClause,
                    pszDomainName,
                    objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSearchObject_inlock(
                    pDirectoryContext,
                    NULL,
                    0,
                    pwszFilter,
                    wszAttributes,
                    FALSE,
                    &pDirEntries,
                    &dwNumEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!dwNumEntries)
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!pDirEntries[0].ulNumAttributes ||
        !pDirEntries[0].pAttributes[0].ulNumValues ||
        pDirEntries[0].pAttributes[0].pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(
                    pDirEntries[0].pAttributes[0].pValues[0].data.pwszStringValue,
                    &pszDomainSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppszDomainSID = pszDomainSID;

cleanup:

    DIRECTORY_FREE_STRING(pszDomainName);
    DIRECTORY_FREE_STRING(pszFilter);
    DIRECTORY_FREE_MEMORY(pwszFilter);

    if (pDirEntries)
    {
        DirectoryFreeEntries(pDirEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppszDomainSID = NULL;

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
