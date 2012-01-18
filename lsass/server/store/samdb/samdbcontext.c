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
 *        samdbcontext.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Provider context initialisation routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
SamDbAcquireDbContext(
    PSAM_DB_CONTEXT* ppDbContext
    );

static
VOID
SamDbReleaseDbContext(
    PSAM_DB_CONTEXT pDbContext
    );

DWORD
SamDbBuildDirectoryContext(
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps,
    DWORD                               dwNumObjectClassAttrMaps,
    PSAM_DB_ATTR_LOOKUP                 pAttrLookup,
    PSAM_DIRECTORY_CONTEXT*             ppDirContext
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(SAM_DIRECTORY_CONTEXT),
                    (PVOID*)&pDirContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    pDirContext->pObjectClassAttrMaps = pObjectClassAttrMaps;
    pDirContext->dwNumObjectClassAttrMaps = dwNumObjectClassAttrMaps;
    pDirContext->pAttrLookup = pAttrLookup;

    dwError = SamDbAcquireDbContext(&pDirContext->pDbContext);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDirContext = pDirContext;

cleanup:
    return dwError;

error:
    if (pDirContext)
    {
        SamDbFreeDirectoryContext(pDirContext);
    }

    *ppDirContext = NULL;

    goto cleanup;
}

static
DWORD
SamDbAcquireDbContext(
    PSAM_DB_CONTEXT* ppDbContext
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PCSTR pszDbPath = SAM_DB;
    PSAM_DB_CONTEXT pDbContext = NULL;

    SAMDB_LOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    if (gSamGlobals.pDbContextList)
    {
        pDbContext = gSamGlobals.pDbContextList;

        gSamGlobals.pDbContextList = gSamGlobals.pDbContextList->pNext;
        gSamGlobals.dwNumDbContexts--;

        pDbContext->pNext = NULL;
    }

    SAMDB_UNLOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    if (!pDbContext)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAM_DB_CONTEXT),
                        (PVOID*)&pDbContext);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_open(
                        pszDbPath,
                        &pDbContext->pDbHandle);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppDbContext = pDbContext;

cleanup:
    SAMDB_UNLOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    return dwError;

error:
    if (pDbContext)
    {
        SamDbFreeDbContext(pDbContext);
    }

    *ppDbContext = NULL;

    goto cleanup;
}

VOID
SamDbFreeDirectoryContext(
    PSAM_DIRECTORY_CONTEXT pDirContext
    )
{
    if (pDirContext->pwszCredential)
    {
        DirectoryFreeMemory(pDirContext->pwszCredential);
    }

    if (pDirContext->pwszDistinguishedName)
    {
        DirectoryFreeMemory(pDirContext->pwszDistinguishedName);
    }

    if (pDirContext->pDbContext)
    {
        SamDbReleaseDbContext(pDirContext->pDbContext);
    }

    DirectoryFreeMemory(pDirContext);
}

static
VOID
SamDbReleaseDbContext(
    PSAM_DB_CONTEXT pDbContext
    )
{
    BOOLEAN bInLock = FALSE;

    SAMDB_LOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    if (gSamGlobals.dwNumDbContexts < gSamGlobals.dwNumMaxDbContexts)
    {
        pDbContext->pNext = gSamGlobals.pDbContextList;
        gSamGlobals.pDbContextList = pDbContext;

        gSamGlobals.dwNumDbContexts++;
    }
    else
    {
        SamDbFreeDbContext(pDbContext);
    }

    SAMDB_UNLOCK_MUTEX(bInLock, &gSamGlobals.mutex);
}

VOID
SamDbFreeDbContext(
    PSAM_DB_CONTEXT pDbContext
    )
{
    if (pDbContext->pDelObjectStmt)
    {
        sqlite3_finalize(pDbContext->pDelObjectStmt);
    }

    if (pDbContext->pQueryObjectCountStmt)
    {
        sqlite3_finalize(pDbContext->pQueryObjectCountStmt);
    }

    if (pDbContext->pQueryObjectRecordInfoStmt)
    {
        sqlite3_finalize(pDbContext->pQueryObjectRecordInfoStmt);
    }

    if (pDbContext->pDbHandle)
    {
        sqlite3_close(pDbContext->pDbHandle);
    }

    DirectoryFreeMemory(pDbContext);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
