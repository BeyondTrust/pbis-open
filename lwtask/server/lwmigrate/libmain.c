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
 *        lwmigrate.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        Library Entry Points
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwTaskMigrateAllSharesA(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PCSTR                       pszServer,
    LW_MIGRATE_FLAGS            dwFlags
    );

static
DWORD
LwTaskMigrateAllSharesW(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PWSTR                       pwszServer,
    LW_MIGRATE_FLAGS            dwFlags
    );

DWORD
LwTaskMigrateInit(
    VOID
    )
{
    DWORD dwError = 0;
    PWSTR pwszDefaultSharePath = NULL;

    dwError = LwTaskGetDefaultSharePathW(&pwszDefaultSharePath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = NetApiInitialize();
    BAIL_ON_LW_TASK_ERROR(dwError);

    gLwTaskGlobals.bNetApiInitialized = TRUE;

    gLwTaskGlobals.pwszDefaultSharePath = pwszDefaultSharePath;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pwszDefaultSharePath);

    goto cleanup;
}

DWORD
LwTaskMigrateCreateContext(
    PCSTR                        pszUsername,
    PCSTR                        pszPassword,
    PLW_SHARE_MIGRATION_CONTEXT* ppContext
    )
{
    DWORD dwError = 0;
    PLW_SHARE_MIGRATION_CONTEXT pContext = NULL;

    BAIL_ON_INVALID_STRING(pszUsername);
    BAIL_ON_INVALID_POINTER(pszPassword);
    BAIL_ON_INVALID_POINTER(ppContext);

    dwError = LwAllocateMemory(
                    sizeof(LW_SHARE_MIGRATION_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    dwError = LwNtStatusToWin32Error(
                    LwIoGetActiveCreds(NULL, &pContext->pLocalCreds));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskAcquireCredsA(
                    pszUsername,
                    pszPassword,
                    &pContext->pRemoteCreds);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppContext = pContext;

cleanup:

    return dwError;

error:

    if (ppContext)
    {
        *ppContext = NULL;
    }

    if (pContext)
    {
        LwTaskMigrateCloseContext(pContext);
    }

    goto cleanup;
}

DWORD
LwTaskMigrateMultipleSharesA(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PCSTR                       pszServer,
    PCSTR                       pszRemoteShares,
    LW_MIGRATE_FLAGS            dwFlags
    )
{
    DWORD dwError = 0;
    PSTR  pszRemoteShare = NULL;

    if (IsNullOrEmptyString(pszRemoteShares))
    {
        dwError = LwTaskMigrateAllSharesA(pContext, pszServer, dwFlags);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    else
    {
        PCSTR  pszCursor = pszRemoteShares;

        do
        {
            size_t sLen = strspn(pszCursor, ", \t");
            if (sLen)
            {
                pszCursor += sLen;
            }

            if (IsNullOrEmptyString(pszCursor))
            {
                break;
            }

            sLen = strcspn(pszCursor, ", \t");
            if (sLen)
            {
                LW_SAFE_FREE_MEMORY(pszRemoteShare);
                pszRemoteShare = NULL;

                dwError = LwStrndup(pszCursor, sLen, &pszRemoteShare);
                BAIL_ON_LW_TASK_ERROR(dwError);

                LW_TASK_LOG_INFO("Migrating share [%s]", pszRemoteShare);

                pszCursor += sLen;

                dwError = LwTaskMigrateShareA(
                                pContext,
                                pszServer,
                                pszRemoteShare,
                                dwFlags);
                BAIL_ON_LW_TASK_ERROR(dwError);
            }

        } while (!IsNullOrEmptyString(pszCursor));
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszRemoteShare);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskMigrateAllSharesA(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PCSTR                       pszServer,
    LW_MIGRATE_FLAGS            dwFlags
    )
{
    DWORD dwError = 0;
    PWSTR pwszServer = NULL;

    dwError = LwMbsToWc16s(pszServer, &pwszServer);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateAllSharesW(pContext, pwszServer, dwFlags);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServer);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskMigrateAllSharesW(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PWSTR                       pwszServer,
    LW_MIGRATE_FLAGS            dwFlags
    )
{
    DWORD dwError       = 0;
    DWORD dwInfoLevel   = 2;
    DWORD dwMaxLen      = 1024;
    DWORD dwTotalShares = 0;
    DWORD dwVisited     = 0;
    DWORD dwResume      = 0;
    PSHARE_INFO_2  pShareInfo = NULL;

    BAIL_ON_INVALID_POINTER(pContext);
    BAIL_ON_INVALID_POINTER(pwszServer);

    do
    {
        DWORD dwNumShares = 0;
        DWORD dwIndex     = 0;

        dwError = LwNtStatusToWin32Error(
                        LwIoSetThreadCreds(pContext->pRemoteCreds->pKrb5Creds));
        BAIL_ON_LW_TASK_ERROR(dwError);

        if (pShareInfo)
        {
            NetApiBufferFree(pShareInfo);
            pShareInfo = NULL;
        }

        dwError = NetShareEnumW(
                        pwszServer,
                        dwInfoLevel,
                        (PBYTE*)&pShareInfo,
                        dwMaxLen,
                        &dwNumShares,
                        &dwTotalShares,
                        &dwResume);
        if (dwError == ERROR_MORE_DATA)
        {
            dwError = 0;
        }
        BAIL_ON_LW_TASK_ERROR(dwError);

        for (dwIndex = 0; dwIndex < dwNumShares; dwIndex++)
        {
            // TODO: Since we have the path already, we should consider
            //       using that directly instead of calling the following API.
            dwError = LwTaskMigrateShareW(
                            pContext,
                            pwszServer,
                            pShareInfo[dwIndex].shi2_netname,
                            dwFlags);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwVisited += dwNumShares;

    } while (dwVisited < dwTotalShares);

cleanup:

    if (pShareInfo)
    {
        NetApiBufferFree(pShareInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskMigrateShareA(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PCSTR                       pszServer,
    PCSTR                       pszShare,
    LW_MIGRATE_FLAGS            dwFlags
    )
{
    DWORD dwError = 0;
    PWSTR pwszServer = NULL;
    PWSTR pwszShare  = NULL;

    BAIL_ON_INVALID_POINTER(pContext);
    BAIL_ON_INVALID_STRING(pszServer);
    BAIL_ON_INVALID_STRING(pszShare);

    dwError = LwMbsToWc16s(pszServer, &pwszServer);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwMbsToWc16s(pszShare, &pwszShare);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateShareW(
                    pContext,
                    pwszServer,
                    pwszShare,
                    dwFlags);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszShare);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskMigrateShareW(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PWSTR                       pwszServer,
    PWSTR                       pwszShare,
    LW_MIGRATE_FLAGS            dwFlags
    )
{
    DWORD dwError = 0;
    PSHARE_INFO_502 pShareInfoRemote = NULL;
    PLW_TASK_FILE   pRemoteFile      = NULL;
    PSHARE_INFO_502 pShareInfoLocal  = NULL;
    PLW_TASK_FILE   pLocalFile       = NULL;
    DWORD           dwInfoLevel   = 502;
    BOOLEAN         bAddShare     = FALSE;
    PWSTR           pwszLocalPath = NULL;

    BAIL_ON_INVALID_POINTER(pContext);
    BAIL_ON_INVALID_STRING(pwszServer);
    BAIL_ON_INVALID_STRING(pwszShare);

    dwError = LwTaskCreateFile(&pRemoteFile);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskCreateFile(&pLocalFile);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                    LwIoSetThreadCreds(pContext->pRemoteCreds->pKrb5Creds));
    BAIL_ON_LW_TASK_ERROR(dwError);

    // Verify that the remote server and share exist
    dwError = NetShareGetInfoW(
                    pwszServer,
                    pwszShare,
                    dwInfoLevel,
                    (PBYTE*)&pShareInfoRemote);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateOpenRemoteShare(
                    pwszServer,
                    pwszShare,
                    &pRemoteFile->hFile);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                    LwIoSetThreadCreds(pContext->pLocalCreds));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = NetShareGetInfoW(
                    NULL,
                    pwszShare,
                    dwInfoLevel,
                    (PBYTE*)&pShareInfoLocal);
    switch (dwError)
    {
        case NERR_NetNameNotFound:
        case ERROR_NOT_FOUND:

            bAddShare = TRUE;

            break;

        default:

            BAIL_ON_LW_TASK_ERROR(dwError);

            // The share exists locally
            // Make sure it points to the path we expect

            dwError = LwTaskGetLocalSharePathW(
							pShareInfoRemote->shi502_path,
							&pwszLocalPath);
            BAIL_ON_LW_TASK_ERROR(dwError);

            if (wc16scasecmp(   pwszLocalPath,
            					pShareInfoLocal->shi502_path))
            {
                dwError = NERR_DeviceShareConflict;
                BAIL_ON_LW_TASK_ERROR(dwError);
            }

            break;
    }

    dwError = LwTaskMigrateCreateShare(
                    pShareInfoRemote,
                    bAddShare,
                    &pLocalFile->hFile);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateShareEx(pContext, pRemoteFile, pLocalFile, dwFlags);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    if (pShareInfoRemote)
    {
        NetApiBufferFree(pShareInfoRemote);
    }

    if (pShareInfoLocal)
    {
        NetApiBufferFree(pShareInfoLocal);
    }

    if (pRemoteFile)
    {
        LwTaskReleaseFile(pRemoteFile);
    }

    if (pLocalFile)
    {
        LwTaskReleaseFile(pLocalFile);
    }

    LW_SAFE_FREE_MEMORY(pwszLocalPath);

    return dwError;

error:

    goto cleanup;
}

VOID
LwTaskMigrateCloseContext(
    PLW_SHARE_MIGRATION_CONTEXT pContext
    )
{
    if (pContext->pLocalCreds)
    {
        LwIoDeleteCreds(pContext->pLocalCreds);
    }

    if (pContext->pRemoteCreds)
    {
        LwTaskFreeCreds(pContext->pRemoteCreds);
    }

    if (pContext->pHead)
    {
        LwTaskFreeDirectoryList(pContext->pHead);
    }

    if (pContext->pMutex)
    {
        pthread_mutex_destroy(&pContext->mutex);
    }

    LwFreeMemory(pContext);
}

VOID
LwTaskMigrateShutdown(
    VOID
    )
{
    LW_SAFE_FREE_MEMORY(gLwTaskGlobals.pwszDefaultSharePath);
    gLwTaskGlobals.pwszDefaultSharePath = NULL;

    if (gLwTaskGlobals.bNetApiInitialized)
    {
        NetApiShutdown();

        gLwTaskGlobals.bNetApiInitialized = FALSE;
    }
}
