/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lsa_wbc_user.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include <stdio.h>
#include <lwmem.h>

wbcErr wbcLookupUserSids(const struct wbcDomainSid *user_sid,
             bool domain_groups_only,
             uint32_t *num_sids,
             struct wbcDomainSid **sids)
{
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    DWORD dwNumGids = 0;
    gid_t *gids = NULL;
    int i = 0;
    LSA_GROUP_INFO_1 *pGroupInfo = NULL;
    struct wbcDomainSid *sidList = NULL;
    PSTR pszSidString = NULL;
    PSTR ppszSidList[2];
    CHAR pszAccountName[512] = "";
    PLSA_SID_INFO pNameList = NULL;
        CHAR chDomainSeparator = 0;

    BAIL_ON_NULL_PTR_PARAM(user_sid, dwErr);
    BAIL_ON_NULL_PTR_PARAM(num_sids, dwErr);
    BAIL_ON_NULL_PTR_PARAM(sids, dwErr);

    /* Conver the SID to a name */

    wbc_status = wbcSidToString(user_sid, &pszSidString);
    dwErr = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    /* map the SID to a name */

    ppszSidList[0] = pszSidString;
    ppszSidList[1] = NULL;

    dwErr = LsaGetNamesBySidList(
                hLsa,
                1,
                ppszSidList,
                &pNameList,
                &chDomainSeparator);
    BAIL_ON_LSA_ERR(dwErr);

    if (pNameList[0].accountType != AccountType_User) {
        dwErr = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERR(dwErr);
    }

    snprintf(pszAccountName,
         sizeof(pszAccountName),
         "%s%c%s",
         pNameList[0].pszDomainName,
                 chDomainSeparator,
         pNameList[0].pszSamAccountName);

    /* Now lookup groups for user SID */

    dwErr = LsaGetGidsForUserByName(hLsa, pszAccountName,
                    &dwNumGids, &gids);
    BAIL_ON_LSA_ERR(dwErr);

    /* Allocate array for sids */

    sidList = _wbc_malloc_zero(sizeof(struct wbcDomainSid)*dwNumGids, NULL);
    BAIL_ON_NULL_PTR(sidList, dwErr);

    /* Now convert gids to SIDs */

    for (i=0; i<dwNumGids; i++) {
        dwErr = LsaFindGroupById(hLsa, gids[i], LSA_FIND_FLAGS_NSS, 1, (PVOID*)&pGroupInfo);
        BAIL_ON_LSA_ERR(dwErr);

        wbc_status = wbcStringToSid(pGroupInfo->pszSid, &sidList[i]);
        dwErr = map_wbc_to_lsa_error(wbc_status);
        BAIL_ON_LSA_ERR(dwErr);

        LsaFreeGroupInfo(1, pGroupInfo);
        pGroupInfo = NULL;
    }

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    *sids = sidList;
    *num_sids = dwNumGids;

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if (pNameList) {
        LsaFreeSIDInfoList(pNameList, 1);
    }

    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(sidList);
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (gids) {
        LwFreeMemory(gids);
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(1, pGroupInfo);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

static DWORD AddUsersToList(char ***pppUserList, uint32_t *pUserSize,
                LSA_USER_INFO_0 **ppUserInfo, DWORD userInfoSize)
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    char **ppUsers = NULL;
    uint32_t nUsers = 0;
    int i;

    BAIL_ON_NULL_PTR_PARAM(pppUserList, dwErr);
    BAIL_ON_NULL_PTR_PARAM(pUserSize, dwErr);

    /* Check for a no-op */

    if (!ppUserInfo || (userInfoSize == 0)) {
        return LW_ERROR_SUCCESS;
    }

    ppUsers = *pppUserList;
    nUsers = *pUserSize;

    if (!ppUsers) {
        ppUsers = _wbc_malloc((userInfoSize + nUsers + 1) * sizeof(char*),
                       _wbc_free_string_array);
        BAIL_ON_NULL_PTR(ppUsers, dwErr);
    } else {
        ppUsers = _wbc_realloc(*pppUserList,
                    (userInfoSize + nUsers + 1) * sizeof(char*));
        BAIL_ON_NULL_PTR(ppUsers, dwErr);
    }

    for (i=0; i<userInfoSize; i++) {
        ppUsers[nUsers] = _wbc_strdup(ppUserInfo[i]->pszName);
        BAIL_ON_NULL_PTR(ppUsers[nUsers], dwErr);

        nUsers++;
    }

    /* Terminate */

    ppUsers[nUsers] = NULL;

    *pppUserList = ppUsers;
        *pUserSize = nUsers;

    dwErr = LW_ERROR_SUCCESS;
cleanup:
    if (dwErr != LW_ERROR_SUCCESS)  {
        _WBC_FREE(ppUsers);
    }

    return dwErr;
}

wbcErr wbcListUsers(const char *domain_name,
            uint32_t *num_users,
            const char ***users)
{
    LSA_USER_INFO_0 **pUserInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;
    DWORD dwNumUsers = 0;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    bool bDone = false;
    uint32_t userSize = 0;
    char **userList = NULL;

    /* For now ignore the domain name nutil the LsaXXX() API supports it */

    BAIL_ON_NULL_PTR_PARAM(users, dwErr);
    BAIL_ON_NULL_PTR_PARAM(num_users, dwErr);

    *users = NULL;
    *num_users = 0;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaBeginEnumUsers(hLsa, 0, 250, 0, &hResume);
    BAIL_ON_LSA_ERR(dwErr);

    while (!bDone) {
        dwErr = LsaEnumUsers(hLsa, hResume,
                     &dwNumUsers,
                     (PVOID**)&pUserInfo);
        BAIL_ON_LSA_ERR(dwErr);

        /* Add new users to list */

        dwErr = AddUsersToList(&userList, &userSize,
                       pUserInfo, dwNumUsers);
        BAIL_ON_LSA_ERR(dwErr);

        /* FIXME! */

        if (dwNumUsers == 0) {
            bDone = true;
            continue;
        }

        LsaFreeUserInfoList(0, (PVOID*)pUserInfo, dwNumUsers);
        pUserInfo = NULL;

    }

    *users = (const char **)userList;
    *num_users = userSize;

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(userList);
    }

    if (hResume) {
        LsaEndEnumUsers(hLsa, hResume);
        hResume = (HANDLE)NULL;
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (pUserInfo) {
        LsaFreeUserInfoList(0, (PVOID*)&pUserInfo, dwNumUsers);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}


