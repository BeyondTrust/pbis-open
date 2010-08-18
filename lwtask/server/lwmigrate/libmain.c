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

DWORD
LwTaskMigrateInit(
    VOID
    )
{
    return 0;
}

DWORD
LwTaskMigrateAllSharesA(
    PSTR             pszServer,
    PSTR             pszUsername,
    PSTR             pszPassword,
    LW_MIGRATE_FLAGS dwFlags
    )
{
    DWORD dwError = 0;
    PWSTR pwszServer = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;

    dwError = LwMbsToWc16s(pszServer, &pwszServer);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwMbsToWc16s(pszUsername, &pwszUsername);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwMbsToWc16s(pszPassword, &pwszPassword);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateAllSharesW(
                    pwszServer,
                    pwszUsername,
                    pwszPassword,
                    dwFlags);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskMigrateAllSharesW(
    PWSTR            pwszServer,
    PWSTR            pwszUsername,
    PWSTR            pwszPassword,
    LW_MIGRATE_FLAGS dwFlags
    )
{
    DWORD dwError       = 0;
    DWORD dwInfoLevel   = 2;
    DWORD dwMaxLen      = 1024;
    DWORD dwTotalShares = 0;
    DWORD dwVisited     = 0;
    DWORD dwResume      = 0;
    PLW_TASK_CREDS pCreds     = NULL;
    PSHARE_INFO_2  pShareInfo = NULL;

    dwError = LwTaskAcquireCredsW(pwszUsername, pwszPassword, &pCreds);
    BAIL_ON_LW_TASK_ERROR(dwError);

    do
    {
        DWORD dwNumShares = 0;
        DWORD dwIndex     = 0;

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
            dwError = LwTaskMigrateShareEx(
                            pCreds,
                            pwszServer,
                            pShareInfo[dwIndex].shi2_netname,
                            pShareInfo[dwIndex].shi2_path,
                            dwFlags);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        if (pShareInfo)
        {
            NetApiBufferFree(pShareInfo);
            pShareInfo = NULL;
        }

        dwVisited += dwNumShares;

    } while (dwVisited < dwTotalShares);

cleanup:

    if (pShareInfo)
    {
        NetApiBufferFree(pShareInfo);
    }

    if (pCreds)
    {
        LwTaskFreeCreds(pCreds);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskMigrateShareA(
    PSTR             pszServer,
    PSTR             pszShare,
    PSTR             pszUsername,
    PSTR             pszPassword,
    LW_MIGRATE_FLAGS dwFlags
    )
{
    DWORD dwError = 0;
    PWSTR pwszServer = NULL;
    PWSTR pwszShare  = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;

    dwError = LwMbsToWc16s(pszServer, &pwszServer);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwMbsToWc16s(pszShare, &pwszShare);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwMbsToWc16s(pszUsername, &pwszUsername);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwMbsToWc16s(pszPassword, &pwszPassword);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateShareW(
                    pwszServer,
                    pwszShare,
                    pwszUsername,
                    pwszPassword,
                    dwFlags);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszShare);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskMigrateShareW(
    PWSTR            pwszServer,
    PWSTR            pwszShare,
    PWSTR            pwszUsername,
    PWSTR            pwszPassword,
    LW_MIGRATE_FLAGS dwFlags
    )
{
    DWORD dwError = 0;
    PLW_TASK_CREDS pCreds = NULL;
    PSHARE_INFO_2 pShareInfo = NULL;
    DWORD dwInfoLevel = 2;

    dwError = LwTaskAcquireCredsW(pwszUsername, pwszPassword, &pCreds);
    BAIL_ON_LW_TASK_ERROR(dwError);

    // Verify that the remote server and share exist
    dwError = NetShareGetInfoW(
                    pwszServer,
                    pwszShare,
                    dwInfoLevel,
                    (PBYTE*)&pShareInfo);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateShareEx(
                    pCreds,
                    pwszServer,
                    pShareInfo->shi2_netname,
                    pShareInfo->shi2_path,
                    dwFlags);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    if (pShareInfo)
    {
        NetApiBufferFree(pShareInfo);
    }

    if (pCreds)
    {
        LwTaskFreeCreds(pCreds);
    }

    return dwError;

error:

    goto cleanup;
}

VOID
LwTaskMigrateShutdown(
    VOID
    )
{
}
