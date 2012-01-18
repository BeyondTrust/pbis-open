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
 *        fileitem.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        File Item to be processed for migration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
LwTaskFreeFile(
    PLW_TASK_FILE pFile
    );

static
VOID
LwTaskFreeDirectory(
    PLW_TASK_DIRECTORY pItem
    );

DWORD
LwTaskCreateFile(
    PLW_TASK_FILE* ppFile
    )
{
    DWORD dwError = 0;
    PLW_TASK_FILE pFile = NULL;

    dwError = LwAllocateMemory(sizeof(LW_TASK_FILE), (PVOID*)&pFile);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pFile->refCount = 1;

    *ppFile = pFile;

cleanup:

    return dwError;

error:

    *ppFile = NULL;

    goto cleanup;
}

PLW_TASK_FILE
LwTaskAcquireFile(
    PLW_TASK_FILE pFile
    )
{
    InterlockedIncrement(&pFile->refCount);

    return pFile;
}

VOID
LwTaskReleaseFile(
    PLW_TASK_FILE pFile
    )
{
    if (InterlockedDecrement(&pFile->refCount) == 0)
    {
        LwTaskFreeFile(pFile);
    }
}

static
VOID
LwTaskFreeFile(
    PLW_TASK_FILE pFile
    )
{
    if (pFile->hFile)
    {
        LwNtCloseFile(pFile->hFile);
    }

    LwFreeMemory(pFile);
}

DWORD
LwTaskCreateDirectory(
    PWSTR               pwszDirname,
    PLW_TASK_FILE       pParentRemote,
    PLW_TASK_FILE       pParentLocal,
    PLW_TASK_DIRECTORY* ppFileItem
    )
{
    DWORD dwError = 0;
    PLW_TASK_DIRECTORY pFileItem = NULL;

    dwError = LwAllocateMemory(sizeof(LW_TASK_DIRECTORY), (PVOID*)&pFileItem);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (pwszDirname)
    {
        dwError = LwAllocateWc16String(&pFileItem->pwszName, pwszDirname);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    pFileItem->pParentRemote = LwTaskAcquireFile(pParentRemote);
    pFileItem->pParentLocal  = LwTaskAcquireFile(pParentLocal);

    *ppFileItem = pFileItem;

cleanup:

    return dwError;

error:

    *ppFileItem = NULL;

    if (pFileItem)
    {
        LwTaskFreeDirectoryList(pFileItem);
    }

    goto cleanup;
}

VOID
LwTaskFreeDirectoryList(
    PLW_TASK_DIRECTORY  pFileItem
    )
{
    while (pFileItem)
    {
        PLW_TASK_DIRECTORY pItem = pFileItem;

        pFileItem = pFileItem->pNext;

        LwTaskFreeDirectory(pItem);
    }
}

static
VOID
LwTaskFreeDirectory(
    PLW_TASK_DIRECTORY pItem
    )
{
    if (pItem->pParentLocal)
    {
        LwTaskReleaseFile(pItem->pParentLocal);
    }

    if (pItem->pParentRemote)
    {
        LwTaskReleaseFile(pItem->pParentRemote);
    }

    LW_SAFE_FREE_MEMORY(pItem->pwszName);

    LwFreeMemory(pItem);
}
