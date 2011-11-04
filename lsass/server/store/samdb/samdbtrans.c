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
 *        samdbtrans.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Transaction
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
SamDbBeginTransaction(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDirContext->pDbContext->pDbHandle,
                    "BEGIN",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SAMDB_LOG_DEBUG("Sqlite3 Error (code: %u): %s",
                dwError,
                LSA_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbRollbackTransaction(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDirContext->pDbContext->pDbHandle,
                    "ROLLBACK",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SAMDB_LOG_DEBUG("Sqlite3 Error (code: %u): %s",
                    dwError,
                    LSA_SAFE_LOG_STRING(pszError));

    if (pszError)
    {

        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbCommitTransaction(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDirContext->pDbContext->pDbHandle,
                    "COMMIT",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SAMDB_LOG_DEBUG("Sqlite3 Error (code: %u): %s",
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
