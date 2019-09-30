/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        samdbdel.c
 *
 * Abstract:
 *
 *
 *      BeyondTrust SAM Database Provider
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
