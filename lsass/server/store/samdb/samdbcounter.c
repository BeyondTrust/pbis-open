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
 *        samdbcounter.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Database Counters
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
SamDbGetNextId(
    HANDLE hDirectory,
    PCSTR  pszQuery,
    PDWORD pdwId
    );

static
DWORD
SamDbIsAvailableId(
    HANDLE hDirectory,
    PCSTR  pszQuery,
    DWORD  dwId
    );

DWORD
SamDbInitConfig(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PCSTR pszQueryTemplate =
            "INSERT INTO " SAM_DB_CONFIG_TABLE \
            " (UIDCounter,"     \
            "  GIDCounter,"     \
            "  RIDCounter,"     \
            "  Version"         \
            " )"                \
            " VALUES ("         \
            "  %u,"             \
            "  %u,"             \
            "  %u,"             \
            "  %u"              \
            " )";
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    pszQuery = sqlite3_mprintf(
                    pszQueryTemplate,
                    SAM_DB_MIN_UID,
                    SAM_DB_MIN_GID,
                    SAM_DB_MIN_RID,
                    SAM_DB_SCHEMA_VERSION);

    if (pszQuery == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = sqlite3_exec(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    return dwError;

error:

    SAMDB_LOG_DEBUG("Error (code: %u): %s",
                    dwError,
                    LSA_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbGetNextAvailableUID(
    HANDLE hDirectory,
    PDWORD pdwUID
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hDirectory;
    PCSTR pszQueryTemplate =
        "SELECT UIDCounter FROM " SAM_DB_CONFIG_TABLE \
        "; UPDATE " SAM_DB_CONFIG_TABLE               \
        "   SET UIDCounter = UIDCounter + 1";
    PCSTR pszUIDQueryTemplate =
        "SELECT " SAM_DB_COL_UID " FROM " SAM_DB_OBJECTS_TABLE \
        " WHERE " SAM_DB_COL_UID " = %u";
    DWORD dwUID = 0;

    /*
     * Get the next UID and make sure it's not used yet
     */
    do
    {
        dwError = SamDbGetNextId(
                        hDirectory,
                        pszQueryTemplate,
                        &dwUID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbIncrementSequenceNumber_inlock(
                        pDirectoryContext);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbIsAvailableId(
                        hDirectory,
                        pszUIDQueryTemplate,
                        dwUID);
        if (dwError != LW_ERROR_SUCCESS &&
            dwError != LW_ERROR_USER_EXISTS)
        {
            BAIL_ON_SAMDB_ERROR(dwError);
        }

    } while (dwError == LW_ERROR_USER_EXISTS &&
             dwUID <= SAM_DB_MAX_UID);

    if (dwUID > SAM_DB_MAX_UID)
    {
        /* Not likely but it's happened - we've run out of UID space */
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    *pdwUID = dwUID;

cleanup:

    return dwError;

error:

    *pdwUID = 0;

    goto cleanup;
}

DWORD
SamDbGetNextAvailableGID(
    HANDLE hDirectory,
    PDWORD pdwGID
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hDirectory;
    PCSTR pszQueryTemplate =
        "SELECT GIDCounter FROM " SAM_DB_CONFIG_TABLE \
        "; UPDATE " SAM_DB_CONFIG_TABLE               \
        "   SET GIDCounter = GIDCounter + 1";
    PCSTR pszGIDQueryTemplate =
        "SELECT " SAM_DB_COL_GID " FROM " SAM_DB_OBJECTS_TABLE \
        " WHERE " SAM_DB_COL_GID " = %u";
    DWORD dwGID = 0;

    /*
     * Get the next GID and make sure it's not used yet
     */
    do
    {
        dwError = SamDbGetNextId(
                        hDirectory,
                        pszQueryTemplate,
                        &dwGID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbIncrementSequenceNumber_inlock(
                        pDirectoryContext);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbIsAvailableId(
                        hDirectory,
                        pszGIDQueryTemplate,
                        dwGID);
        if (dwError != LW_ERROR_SUCCESS &&
            dwError != LW_ERROR_USER_EXISTS)
        {
            BAIL_ON_SAMDB_ERROR(dwError);
        }

    } while (dwError == LW_ERROR_USER_EXISTS &&
             dwGID <= SAM_DB_MAX_GID);

    if (dwGID > SAM_DB_MAX_GID)
    {
        /* Not likely but it's happened - we've run out of GID space */
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    *pdwGID = dwGID;

cleanup:

    return dwError;

error:

    *pdwGID = 0;

    goto cleanup;
}

DWORD
SamDbGetNextAvailableRID(
    HANDLE hDirectory,
    PDWORD pdwRID
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PCSTR pszDomainQueryTemplate =
        "SELECT " SAM_DB_COL_OBJECT_SID " FROM " SAM_DB_OBJECTS_TABLE \
        " WHERE " SAM_DB_COL_OBJECT_CLASS " = %u";
    PCSTR pszQueryTemplate =
        "SELECT RIDCounter FROM " SAM_DB_CONFIG_TABLE \
        "; UPDATE " SAM_DB_CONFIG_TABLE               \
        "   SET RIDCounter = RIDCounter + 1";
    PCSTR pszObjectSIDQueryTemplate =
        "SELECT " SAM_DB_COL_OBJECT_SID " FROM " SAM_DB_OBJECTS_TABLE \
        " WHERE " SAM_DB_COL_OBJECT_SID " = '%s-%s'";
    PSTR pszDomainQuery = NULL;
    PSTR *ppszDomainResult = NULL;
    int nDomainRows = 0;
    int nDomainCols = 0;
    PSTR pszError = NULL;
    PSTR pszDomainSID = NULL;
    PSTR pszObjectRIDQueryTemplate = NULL;
    DWORD dwRID = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = LwAllocateStringPrintf(
                    &pszDomainQuery,
                    pszDomainQueryTemplate,
                    SAMDB_OBJECT_CLASS_DOMAIN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszDomainQuery,
                    &ppszDomainResult,
                    &nDomainRows,
                    &nDomainCols,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    if ((nDomainRows != 1) || (nDomainCols != 1))
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszDomainSID = ppszDomainResult[nDomainRows * nDomainCols];

    dwError = LwAllocateStringPrintf(
                    &pszObjectRIDQueryTemplate,
                    pszObjectSIDQueryTemplate,
                    pszDomainSID,
                    "%u");
    BAIL_ON_SAMDB_ERROR(dwError);

    /*
     * Get the next RID and make sure it's not used yet
     */
    do
    {
        dwError = SamDbGetNextId(
                        hDirectory,
                        pszQueryTemplate,
                        &dwRID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbIncrementSequenceNumber_inlock(
                        pDirContext);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbIsAvailableId(
                        hDirectory,
                        pszObjectRIDQueryTemplate,
                        dwRID);
        if (dwError != LW_ERROR_SUCCESS &&
            dwError != LW_ERROR_USER_EXISTS)
        {
            BAIL_ON_SAMDB_ERROR(dwError);
        }

    } while (dwError == LW_ERROR_USER_EXISTS &&
             dwRID <= SAM_DB_MAX_RID);

    if (dwRID > SAM_DB_MAX_RID)
    {
        /* Not likely but it's happened - we've run out of RID space */
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

    *pdwRID = dwRID;

cleanup:
    if (ppszDomainResult)
    {
        sqlite3_free_table(ppszDomainResult);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    DIRECTORY_FREE_MEMORY(pszDomainQuery);
    DIRECTORY_FREE_MEMORY(pszObjectRIDQueryTemplate);

    return dwError;

error:
    *pdwRID = 0;

    goto cleanup;
}


DWORD
SamDbCheckAvailableUID(
    HANDLE hDirectory,
    DWORD  dwUID
    )
{
    DWORD dwError = 0;
    PCSTR pszUIDQueryTemplate =
        "SELECT " SAM_DB_COL_UID " FROM " SAM_DB_OBJECTS_TABLE \
        " WHERE " SAM_DB_COL_UID " = %u";

    dwError = SamDbIsAvailableId(
                    hDirectory,
                    pszUIDQueryTemplate,
                    dwUID);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
SamDbCheckAvailableGID(
    HANDLE hDirectory,
    DWORD  dwGID
    )
{
    DWORD dwError = 0;
    PCSTR pszGIDQueryTemplate =
        "SELECT " SAM_DB_COL_GID " FROM " SAM_DB_OBJECTS_TABLE \
        " WHERE " SAM_DB_COL_GID " = %u";

    dwError = SamDbIsAvailableId(
                    hDirectory,
                    pszGIDQueryTemplate,
                    dwGID);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
SamDbCheckAvailableSID(
    HANDLE hDirectory,
    PCSTR pszSID
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PCSTR pszQueryTemplate =
        "SELECT " SAM_DB_COL_OBJECT_SID " FROM " SAM_DB_OBJECTS_TABLE \
        " WHERE " SAM_DB_COL_OBJECT_SID " = '%s'";
    PSTR pszQuery = NULL;
    PSTR *ppszResult = NULL;
    int nRows = 0;
    int nCols = 0;
    PSTR pszError = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = LwAllocateStringPrintf(
                    &pszQuery,
                    pszQueryTemplate,
                    pszSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (nRows == 0)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    else if (nRows == 1)
    {
        dwError = LW_ERROR_USER_EXISTS;
    }
    else
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:
    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    DIRECTORY_FREE_MEMORY(pszQuery);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
SamDbGetNextId(
    HANDLE hDirectory,
    PCSTR  pszQuery,
    PDWORD pdwId
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!nRows || (nCols != 1))
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *pdwId = strtoul(ppszResult[1], NULL, 10);

cleanup:
    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    return dwError;

error:
    if (pdwId)
    {
        *pdwId = 0;
    }

    SAMDB_LOG_DEBUG("Sqlite3 Error (code: %u): %s",
                    dwError,
                    LSA_SAFE_LOG_STRING(pszError));

    goto cleanup;
}


static
DWORD
SamDbIsAvailableId(
    HANDLE hDirectory,
    PCSTR  pszQueryTemplate,
    DWORD  dwId
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR pszQuery = NULL;
    PSTR *ppszResult = NULL;
    int nRows = 0;
    int nCols = 0;
    PSTR pszError = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = LwAllocateStringPrintf(
                    &pszQuery,
                    pszQueryTemplate,
                    dwId);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (nRows == 0)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    else if (nRows == 1)
    {
        dwError = LW_ERROR_USER_EXISTS;
    }
    else
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
    }
    BAIL_ON_SAMDB_ERROR(dwError);

error:
    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    DIRECTORY_FREE_MEMORY(pszQuery);

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
