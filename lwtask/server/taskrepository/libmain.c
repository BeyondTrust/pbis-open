/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        taskrepository.h
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
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
