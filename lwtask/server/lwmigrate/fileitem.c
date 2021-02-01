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
 *        fileitem.c
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
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
