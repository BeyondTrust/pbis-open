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
 *        samdbdel.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM objects deletion routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

DWORD
SamDbDeleteObject(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PSTR pszObjectDN = NULL;
    PSAM_DB_DN pDN = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumDependents = 0;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    if (!hDirectory || !pwszObjectDN || !*pwszObjectDN)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pwszObjectDN,
                    &pszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetNumberOfDependents_inlock(
                    pDirectoryContext,
                    pszObjectDN,
                    &dwNumDependents);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDependents)
    {
        dwError = LW_ERROR_OBJECT_IN_USE;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!pDirectoryContext->pDbContext->pDelObjectStmt)
    {
        PCSTR pszDelQueryTemplate =
            "DELETE FROM " SAM_DB_OBJECTS_TABLE \
            " WHERE " SAM_DB_COL_DISTINGUISHED_NAME " = ?1;";

        dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszDelQueryTemplate,
                    -1,
                    &pDirectoryContext->pDbContext->pDelObjectStmt,
                    NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(
                        dwError,
                        pDirectoryContext->pDbContext->pDbHandle);
    }

    dwError = sqlite3_bind_text(
                    pDirectoryContext->pDbContext->pDelObjectStmt,
                    1,
                    pszObjectDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(
                    dwError,
                    pDirectoryContext->pDbContext->pDelObjectStmt);

    dwError = sqlite3_step(pDirectoryContext->pDbContext->pDelObjectStmt);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(
                    dwError,
                    pDirectoryContext->pDbContext->pDelObjectStmt);

    dwError = SamDbIncrementSequenceNumber_inlock(
                    pDirectoryContext);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (pDirectoryContext->pDbContext->pDelObjectStmt)
    {
        sqlite3_reset(pDirectoryContext->pDbContext->pDelObjectStmt);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    DIRECTORY_FREE_STRING(pszObjectDN);

    if (pDN)
    {
        SamDbFreeDN(pDN);
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
