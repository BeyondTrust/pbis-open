/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        taskrepository.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Task Repository
 *
 *        Library Entry Points
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

DWORD LwTaskRepositoryInit(VOID)
{
    DWORD dwError = 0;
    PCSTR pszDbDirPath = LW_TASK_DB_DIR;
    PCSTR pszDbPath = LW_TASK_DB;
    BOOLEAN bExists = FALSE;
    BOOLEAN bCleanupDb = FALSE;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;

    pthread_rwlock_init(&gLwTaskDbGlobals.mutex, NULL);
    gLwTaskDbGlobals.pMutex = &gLwTaskDbGlobals.mutex;

    dwError = LwCheckFileTypeExists(
                    pszDbDirPath,
                    LWFILE_DIRECTORY,
                    &bExists);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!bExists)
    {
        mode_t mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base folder */
        dwError = LwCreateDirectory(pszDbDirPath, mode);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LwChangeOwnerAndPermissions(
                    pszDbDirPath,
                    0, /* uid: root */
                    0, /* gid: root */
                    S_IRWXU);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwCheckFileTypeExists(pszDbPath, LWFILE_REGULAR, &bExists);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!bExists)
    {
        dwError = LwTaskDbOpen(&pDbContext);
        BAIL_ON_LW_TASK_ERROR(dwError);

        bCleanupDb = TRUE;

        dwError = LwTaskDbCreateTables(pDbContext);
        BAIL_ON_LW_TASK_ERROR(dwError);

        dwError = LwTaskDbAddDefaultEntries(pDbContext);
        BAIL_ON_LW_TASK_ERROR(dwError);

        dwError = LwChangeOwnerAndPermissions(
                        pszDbPath,
                        0, /* uid: root */
                        0, /* gid: root */
                        S_IRWXU);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    return dwError;

error:

    if (bCleanupDb)
    {
        LwRemoveFile(pszDbPath);
    }

    goto cleanup;
}

VOID
LwTaskRepositoryShutdown(
    VOID
    )
{
    if (gLwTaskDbGlobals.pMutex)
    {
        pthread_rwlock_destroy(&gLwTaskDbGlobals.mutex);
        gLwTaskDbGlobals.pMutex = NULL;
    }
}
