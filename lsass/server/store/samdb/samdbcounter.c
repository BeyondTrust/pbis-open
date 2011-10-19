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
 *
 */

#include "includes.h"

static
DWORD
SamDbGetNextAvailableId(
    HANDLE hDirectory,
    PCSTR  pszQuery,
    PDWORD pdwId
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
            "  %d,"             \
            "  %d,"             \
            "  %d,"             \
            "  %d"              \
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

    SAMDB_LOG_DEBUG("Error (code: %d): %s",
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
    PCSTR pszQueryTemplate =
        "SELECT UIDCounter FROM " SAM_DB_CONFIG_TABLE \
        "; UPDATE " SAM_DB_CONFIG_TABLE               \
        "   SET UIDCounter = UIDCounter + 1";
    DWORD dwUID = 0;

    // TODO: Make sure the id is not being used
    dwError = SamDbGetNextAvailableId(
                    hDirectory,
                    pszQueryTemplate,
                    &dwUID);
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
    PCSTR pszQueryTemplate =
        "SELECT GIDCounter FROM " SAM_DB_CONFIG_TABLE \
        "; UPDATE " SAM_DB_CONFIG_TABLE               \
        "   SET GIDCounter = GIDCounter + 1";
    DWORD dwGID = 0;

    // TODO: Make sure the id is not being used
    dwError = SamDbGetNextAvailableId(
                    hDirectory,
                    pszQueryTemplate,
                    &dwGID);
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
    PCSTR pszQueryTemplate =
        "SELECT RIDCounter FROM " SAM_DB_CONFIG_TABLE \
        "; UPDATE " SAM_DB_CONFIG_TABLE               \
        "   SET RIDCounter = RIDCounter + 1";
    DWORD dwRID = 0;

    // TODO: Make sure the id is not being used
    dwError = SamDbGetNextAvailableId(
                    hDirectory,
                    pszQueryTemplate,
                    &dwRID);
    BAIL_ON_SAMDB_ERROR(dwError);

    *pdwRID = dwRID;

cleanup:

    return dwError;

error:

    *pdwRID = 0;

    goto cleanup;
}

static
DWORD
SamDbGetNextAvailableId(
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

    *pdwId = atoi(ppszResult[1]);

cleanup:

    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    *pdwId = 0;

    SAMDB_LOG_DEBUG("Sqlite3 Error (code: %d): %s",
                    dwError,
                    LSA_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
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
