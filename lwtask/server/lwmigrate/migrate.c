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
 *        migrate.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        Core share migration methods
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwTaskMigrateCreateContextW(
    PWSTR                        pwszServer,
    PWSTR                        pwszShare,
    PWSTR                        pwszLocalPath,
    PLW_SHARE_MIGRATION_CONTEXT* ppContext
    );

static
DWORD
LwTaskMigrateBuildPathW(
    PWSTR  pwszPrefix,
    PWSTR  pwszServer,
    PWSTR  pwszShare,
    PWSTR* ppwszPath
    );

static
DWORD
LwTaskMigrateProcessDir(
    PLW_FILE_ITEM pFileItem,
    PLW_FILE_ITEM* ppChildFileItems
    );

static
DWORD
LwTaskMigrateProcessFile(
    PLW_FILE_ITEM pFileItem
    );

static
VOID
LwTaskMigrateFreeContext(
    PLW_SHARE_MIGRATION_CONTEXT pContext
    );

DWORD
LwTaskMigrateShareEx(
    PLW_TASK_CREDS   pCreds,
    PWSTR            pwszServer,
    PWSTR            pwszShare,
    PWSTR            pwszSharePath,
    LW_MIGRATE_FLAGS dwFlags
    )
{
    DWORD dwError = 0;
    PLW_SHARE_MIGRATION_CONTEXT pContext = NULL;
    PLW_FILE_ITEM pFileList = NULL;

    dwError = LwTaskMigrateCreateContextW(
                    pwszServer,
                    pwszShare,
                    pwszSharePath,
                    &pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    while (pContext->pHead)
    {
        PLW_FILE_ITEM pCursor = pContext->pHead;

        while (pCursor)
        {
            PLW_FILE_ITEM pCurrent = pCursor;

            if (pCursor->bIsDir)
            {
                dwError = LwTaskMigrateProcessDir(pCursor, &pFileList);
                BAIL_ON_LW_TASK_ERROR(dwError);
            }
            else // file
            {
                dwError = LwTaskMigrateProcessFile(pCursor);
                BAIL_ON_LW_TASK_ERROR(dwError);
            }

            pCursor = pCursor->pNext; // First advance the cursor

            // Replace the current file item with the new list
            if (pContext->pHead == pCurrent)
            {
                if (pFileList)
                {
                    pContext->pHead = pFileList;
                    // Seek end of list
                    while (pFileList->pNext)
                    {
                        pFileList = pFileList->pNext;
                    }
                    pFileList->pNext = pCurrent->pNext;
                    if (pCurrent->pNext)
                    {
                        pCurrent->pNext->pPrev = pFileList;
                    }

                    pCurrent->pNext = pCurrent->pPrev = NULL;

                    if (!pFileList->pNext)
                    {
                        pContext->pTail = pFileList;
                    }

                    pFileList = NULL;
                }
                else
                {
                    pContext->pHead = pCurrent->pNext;
                    if (pCurrent->pNext)
                    {
                        pCurrent->pNext->pPrev = pContext->pHead;
                    }
                    if (!pContext->pHead)
                    {
                        pContext->pTail = NULL;
                    }
                    pCurrent->pNext = pCurrent->pPrev = NULL;
                }
            }
            else if (pContext->pTail == pCurrent)
            {
                pContext->pTail->pPrev = pFileList;
                if (pFileList)
                {
                    pFileList->pPrev = pContext->pTail;
                    pContext->pTail = pFileList;
                    pFileList = NULL;
                }
            }
            else
            {
                if (pFileList)
                {
                    pCurrent->pPrev->pNext = pFileList;
                    while (pFileList->pNext)
                    {
                        pFileList = pFileList->pNext;
                    }

                    pFileList->pNext = pCurrent->pNext;
                    pCurrent->pNext->pPrev = pFileList;
                    pFileList = NULL;
                }
                else
                {
                    pCurrent->pPrev->pNext = pCurrent->pNext;
                    if (pCurrent->pNext)
                    {
                        pCurrent->pNext->pPrev = pCurrent->pPrev;
                    }
                }

                pCurrent->pPrev = pCurrent->pNext = NULL;
            }

            LwTaskFreeFileItemList(pCurrent);
        }
    }

cleanup:

    if (pFileList)
    {
        LwTaskFreeFileItemList(pFileList);
    }

    if (pContext)
    {
        LwTaskMigrateFreeContext(pContext);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskMigrateCreateContextW(
    PWSTR                        pwszServer,
    PWSTR                        pwszShare,
    PWSTR                        pwszLocalPath,
    PLW_SHARE_MIGRATION_CONTEXT* ppContext
    )
{
    DWORD dwError = 0;
    wchar16_t wszRemotePrefix[] = { '/', 'r', 'd', 'r', 0 };
    wchar16_t wszLocalPrefix[]  = { '/', 'p', 'v', 'f', 's', 0 };
    PLW_SHARE_MIGRATION_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LW_SHARE_MIGRATION_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskCreateFileItem(
                    NULL,
                    NULL,
                    TRUE,
                    &pContext->pHead);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pContext->pTail = pContext->pHead;

    dwError = LwTaskMigrateBuildPathW(
                    &wszRemotePrefix[0],
                    pwszServer,
                    pwszShare,
                    &pContext->pHead->pwszRemotePath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    // TODO: Build the local path
    dwError = LwTaskMigrateBuildPathW(
                    &wszLocalPrefix[0],
                    pwszServer,
                    pwszShare,
                    &pContext->pHead->pwszLocalPath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppContext = pContext;

cleanup:

    return dwError;

error:

    *ppContext = NULL;

    if (pContext)
    {
        LwTaskMigrateFreeContext(pContext);
    }

    goto cleanup;
}

static
DWORD
LwTaskMigrateBuildPathW(
    PWSTR  pwszPrefix,
    PWSTR  pwszServer,
    PWSTR  pwszShare,
    PWSTR* ppwszPath
    )
{
    DWORD dwError = 0;
    wchar16_t wszSeparator[] = { '/', 0 };
    DWORD dwLen = 0;
    PWSTR pwszPath = NULL;
    PWSTR pwszCursor = NULL;
    PLW_SHARE_MIGRATION_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LW_SHARE_MIGRATION_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwLen =  (wc16slen(pwszPrefix) +
              ((sizeof(wszSeparator)/sizeof(wszSeparator[0])) - 1) +
              wc16slen(pwszServer) +
              ((sizeof(wszSeparator)/sizeof(wszSeparator[0])) - 1) +
              wc16slen(pwszShare)) * sizeof(wchar16_t);

    dwError = LwAllocateMemory(dwLen, (PVOID*)&pwszPath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pwszCursor = pwszPath;
    dwLen = wc16slen(pwszPrefix);
    memcpy((PBYTE)pwszCursor, (PBYTE)pwszPrefix, dwLen * sizeof(wchar16_t));
    pwszCursor += dwLen;

    *pwszCursor++ = wszSeparator[0];

    dwLen = wc16slen(pwszServer);
    memcpy((PBYTE)pwszCursor, (PBYTE)pwszServer, dwLen * sizeof(wchar16_t));
    pwszCursor += dwLen;

    *pwszCursor++ = wszSeparator[0];

    dwLen = wc16slen(pwszShare);
    memcpy((PBYTE)pwszCursor, (PBYTE)pwszShare, dwLen * sizeof(wchar16_t));
    // pwszCursor += dwLen;

    *ppwszPath = pwszPath;

cleanup:

    return dwError;

error:

    *ppwszPath = NULL;

    LW_SAFE_FREE_MEMORY(pwszPath);

    goto cleanup;
}

static
DWORD
LwTaskMigrateProcessDir(
    PLW_FILE_ITEM  pFileItem,
    PLW_FILE_ITEM* ppChildFileItems
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
DWORD
LwTaskMigrateProcessFile(
    PLW_FILE_ITEM pFileItem
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
VOID
LwTaskMigrateFreeContext(
    PLW_SHARE_MIGRATION_CONTEXT pContext
    )
{
    if (pContext->pHead)
    {
        LwTaskFreeFileItemList(pContext->pHead);
    }

    LwFreeMemory(pContext);
}



