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
 *        worker.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Test Program for stress testing AD Provider
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

PVOID
LADSFindUserByName(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PVOID pResult = NULL;
    PCSTR pszName = NULL;
    DWORD dwInfoLevel = gLADSStressData[LADS_FIND_USER_BY_NAME].dwInfoLevel;
    PVOID pUserInfo = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    while (!LADSProcessShouldStop())
    {
        DWORD iName = 0;

        for (iName = 0;
             (!LADSProcessShouldStop() &&
              (iName < gLADSStressData[LADS_FIND_USER_BY_NAME].dwNumItems));
             iName++)
        {
            pszName = gLADSStressData[LADS_FIND_USER_BY_NAME].data.ppszNames[iName];

            dwError = gpAuthProvider->pFnTable->pfnOpenHandle(
                                          geteuid(),
                                          getegid(),
                                          getpid(),
                                          &hProvider);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = gpAuthProvider->pFnTable->pfnLookupUserByName(
                                          hProvider,
                                          pszName,
                                          dwInfoLevel,
                                          &pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);

            gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
            hProvider = (HANDLE)NULL;

            LsaFreeUserInfo(dwInfoLevel, pUserInfo);
            pUserInfo = NULL;

            if (gLADSStressData[LADS_FIND_USER_BY_NAME].dwSleepMSecs)
            {
                sleep(gLADSStressData[LADS_FIND_USER_BY_NAME].dwSleepMSecs);
            }
        }
    }

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    if (hProvider != (HANDLE)NULL)
    {
        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
    }

    return pResult;

error:

    LSA_LOG_ERROR("Failed to find user by name [%s] [error code: %d]",
                  (LW_IS_NULL_OR_EMPTY_STR(pszName) ? "" : pszName), dwError);

    LADSStopProcess();

    goto cleanup;
}

PVOID
LADSFindUserById(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PVOID pResult = NULL;
    uid_t userId = 0;
    DWORD dwInfoLevel = gLADSStressData[LADS_FIND_USER_BY_ID].dwInfoLevel;
    PVOID pUserInfo = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    while (!LADSProcessShouldStop())
    {
        DWORD iUid = 0;

        for (iUid = 0;
             (!LADSProcessShouldStop() &&
              (iUid < gLADSStressData[LADS_FIND_USER_BY_ID].dwNumItems));
             iUid++)
        {
            userId = gLADSStressData[LADS_FIND_USER_BY_ID].data.pUidArray[iUid];

            dwError = gpAuthProvider->pFnTable->pfnOpenHandle(
                                          geteuid(),
                                          getegid(),
                                          getpid(),
                                          &hProvider);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = gpAuthProvider->pFnTable->pfnLookupUserById(
                                          hProvider,
                                          userId,
                                          dwInfoLevel,
                                          &pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);

            gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
            hProvider = (HANDLE)NULL;

            LsaFreeUserInfo(dwInfoLevel, pUserInfo);
            pUserInfo = NULL;

            if (gLADSStressData[LADS_FIND_USER_BY_ID].dwSleepMSecs)
            {
                sleep(gLADSStressData[LADS_FIND_USER_BY_ID].dwSleepMSecs);
            }
        }
    }

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    if (hProvider != (HANDLE)NULL)
    {
        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
    }

    return pResult;

error:

    LSA_LOG_ERROR("Failed to find user by id [%ld] [error code: %d]", (long)userId, dwError);

    LADSStopProcess();

    goto cleanup;
}

PVOID
LADSEnumUsers(
    PVOID pData
    )
{
    DWORD  dwError = 0;
    PVOID  pResult = NULL;
    DWORD  dwInfoLevel = gLADSStressData[LADS_ENUM_USERS].dwInfoLevel;
    DWORD  dwMaxNumUsers = 500;
    DWORD  dwNumUsersFound = 0;
    PVOID* ppUserInfos = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;

    while (!LADSProcessShouldStop())
    {
        dwError = gpAuthProvider->pFnTable->pfnOpenHandle(
                                      geteuid(),
                                      getegid(),
                                      getpid(),
                                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = gpAuthProvider->pFnTable->pfnBeginEnumUsers(
                              hProvider,
                              dwInfoLevel,
                              0,
                              &hResume);
        BAIL_ON_LSA_ERROR(dwError);

        do
        {
            if (dwNumUsersFound)
            {
                LsaFreeUserInfoList(
                                dwInfoLevel,
                                ppUserInfos,
                                dwNumUsersFound);
                ppUserInfos = NULL;
                dwNumUsersFound = 0;
            }

            dwError = gpAuthProvider->pFnTable->pfnEnumUsers(
                            hProvider,
                            hResume,
                            dwMaxNumUsers,
                            &dwNumUsersFound,
                            &ppUserInfos);
            BAIL_ON_LSA_ERROR(dwError);

        } while (dwNumUsersFound && !LADSProcessShouldStop());

        gpAuthProvider->pFnTable->pfnEndEnumUsers(
                        hProvider,
                        hResume);
        hResume = (HANDLE)NULL;

        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
        hProvider = (HANDLE)NULL;

        if (gLADSStressData[LADS_ENUM_USERS].dwSleepMSecs)
        {
            sleep(gLADSStressData[LADS_ENUM_USERS].dwSleepMSecs);
        }
    }

cleanup:

    if (dwNumUsersFound)
    {
        LsaFreeUserInfoList(
                        dwInfoLevel,
                        ppUserInfos,
                        dwNumUsersFound);
    }

    if (hProvider != (HANDLE)NULL)
    {
        if (hResume != (HANDLE)NULL)
        {
            gpAuthProvider->pFnTable->pfnEndEnumUsers(
                            hProvider,
                            hResume);
        }

        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
    }

    return pResult;

error:

    LSA_LOG_ERROR("Failed to enumerate users [error code: %d]", dwError);

    LADSStopProcess();

    goto cleanup;
}

PVOID
LADSFindGroupByName(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PVOID pResult = NULL;
    PCSTR pszName = NULL;
    DWORD dwInfoLevel = gLADSStressData[LADS_FIND_GROUP_BY_NAME].dwInfoLevel;
    PVOID pGroupInfo = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    while (!LADSProcessShouldStop())
    {
        DWORD iName = 0;

        for (iName = 0;
             (!LADSProcessShouldStop() &&
              (iName < gLADSStressData[LADS_FIND_GROUP_BY_NAME].dwNumItems));
             iName++)
        {
            pszName = gLADSStressData[LADS_FIND_GROUP_BY_NAME].data.ppszNames[iName];

            dwError = gpAuthProvider->pFnTable->pfnOpenHandle(
                                          geteuid(),
                                          getegid(),
                                          getpid(),
                                          &hProvider);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = gpAuthProvider->pFnTable->pfnLookupGroupByName(
                                          hProvider,
                                          pszName,
                                          0,
                                          dwInfoLevel,
                                          &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);

            gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
            hProvider = (HANDLE)NULL;

            LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
            pGroupInfo = NULL;

            if (gLADSStressData[LADS_FIND_GROUP_BY_NAME].dwSleepMSecs)
            {
                sleep(gLADSStressData[LADS_FIND_GROUP_BY_NAME].dwSleepMSecs);
            }
        }
    }

cleanup:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    if (hProvider != (HANDLE)NULL)
    {
        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
    }

    return pResult;

error:

    LSA_LOG_ERROR("Failed to find group by name [%s] [error code: %d]",
                  (LW_IS_NULL_OR_EMPTY_STR(pszName) ? "" : pszName), dwError);

    LADSStopProcess();

    goto cleanup;
}

PVOID
LADSFindGroupById(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PVOID pResult = NULL;
    gid_t groupId = 0;
    DWORD dwInfoLevel = gLADSStressData[LADS_FIND_GROUP_BY_ID].dwInfoLevel;
    PVOID pGroupInfo = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    while (!LADSProcessShouldStop())
    {
        DWORD iGid = 0;

        for (iGid = 0;
             (!LADSProcessShouldStop() &&
              (iGid < gLADSStressData[LADS_FIND_GROUP_BY_ID].dwNumItems));
             iGid++)
        {
            groupId = gLADSStressData[LADS_FIND_GROUP_BY_ID].data.pGidArray[iGid];

            dwError = gpAuthProvider->pFnTable->pfnOpenHandle(
                                          geteuid(),
                                          getegid(),
                                          getpid(),
                                          &hProvider);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = gpAuthProvider->pFnTable->pfnLookupGroupById(
                                          hProvider,
                                          groupId,
                                          0,
                                          dwInfoLevel,
                                          &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);

            gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
            hProvider = (HANDLE)NULL;

            LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
            pGroupInfo = NULL;

            if (gLADSStressData[LADS_FIND_GROUP_BY_ID].dwSleepMSecs)
            {
                sleep(gLADSStressData[LADS_FIND_GROUP_BY_ID].dwSleepMSecs);
            }
        }
    }

cleanup:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    if (hProvider != (HANDLE)NULL)
    {
        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
    }

    return pResult;

error:

    LSA_LOG_ERROR("Failed to find group by id [%ld] [error code: %d]", (long)groupId, dwError);

    LADSStopProcess();

    goto cleanup;
}

PVOID
LADSEnumGroups(
    PVOID pData
    )
{
    DWORD  dwError = 0;
    PVOID  pResult = NULL;
    DWORD  dwInfoLevel = gLADSStressData[LADS_ENUM_GROUPS].dwInfoLevel;
    DWORD  dwMaxNumGroups = 500;
    DWORD  dwNumGroupsFound = 0;
    PVOID* ppGroupInfos = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;

    while (!LADSProcessShouldStop())
    {
        dwError = gpAuthProvider->pFnTable->pfnOpenHandle(
                                      geteuid(),
                                      getegid(),
                                      getpid(),
                                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = gpAuthProvider->pFnTable->pfnBeginEnumGroups(
                              hProvider,
                              dwInfoLevel,
                              FALSE,
                              0,
                              &hResume);
        BAIL_ON_LSA_ERROR(dwError);

        do
        {
            if (dwNumGroupsFound)
            {
                LsaFreeUserInfoList(
                                dwInfoLevel,
                                ppGroupInfos,
                                dwNumGroupsFound);
                ppGroupInfos = NULL;
                dwNumGroupsFound = 0;
            }

            dwError = gpAuthProvider->pFnTable->pfnEnumGroups(
                            hProvider,
                            hResume,
                            dwMaxNumGroups,
                            &dwNumGroupsFound,
                            &ppGroupInfos);
            BAIL_ON_LSA_ERROR(dwError);

        } while (dwNumGroupsFound && !LADSProcessShouldStop());

        gpAuthProvider->pFnTable->pfnEndEnumGroups(
                        hProvider,
                        hResume);

        hResume = (HANDLE)NULL;

        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
        hProvider = (HANDLE)NULL;

        if (gLADSStressData[LADS_ENUM_GROUPS].dwSleepMSecs)
        {
            sleep(gLADSStressData[LADS_ENUM_GROUPS].dwSleepMSecs);
        }
    }

cleanup:

    if (dwNumGroupsFound)
    {
        LsaFreeUserInfoList(
                        dwInfoLevel,
                        ppGroupInfos,
                        dwNumGroupsFound);
    }

    if (hProvider != (HANDLE)NULL)
    {
        if (hResume != (HANDLE)NULL)
        {
            gpAuthProvider->pFnTable->pfnEndEnumGroups(
                            hProvider,
                            hResume);
        }

        gpAuthProvider->pFnTable->pfnCloseHandle(hProvider);
    }

    return pResult;

error:

    LSA_LOG_ERROR("Failed to enumerate groups [error code: %d]", dwError);

    LADSStopProcess();

    goto cleanup;
}

DWORD
LADSGetGUID(
    PSTR* ppszGUID
    )
{
    DWORD dwError = 0;
    PSTR  pszGUID = NULL;
    uuid_t uuid = {0};
    CHAR  szUUID[37] = "";

    uuid_generate(uuid);

    uuid_unparse(uuid, szUUID);

    dwError = LwAllocateString(
                       szUUID,
                       &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszGUID = pszGUID;

cleanup:

    return dwError;

error:

    *ppszGUID = NULL;

    LW_SAFE_FREE_STRING(pszGUID);

    goto cleanup;
}
