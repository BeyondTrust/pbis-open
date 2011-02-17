/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa_wbc_group.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"

static DWORD AddGroupsToList(char ***pppGroupList, uint32_t *pGroupSize,
                 LSA_GROUP_INFO_0 **ppGroupInfo, DWORD groupInfoSize)
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    char **ppGroups = NULL;
    uint32_t nGroups = 0;
    int i;

    BAIL_ON_NULL_PTR_PARAM(pppGroupList, dwErr);
    BAIL_ON_NULL_PTR_PARAM(pGroupSize, dwErr);

    /* Check for a no-op */

    if (!ppGroupInfo || (groupInfoSize == 0)) {
        return LW_ERROR_SUCCESS;
    }

    ppGroups = *pppGroupList;
    nGroups = *pGroupSize;

    if (!ppGroups) {
        ppGroups = _wbc_malloc((groupInfoSize+1) * sizeof(char*),
                       _wbc_free_string_array);
        BAIL_ON_NULL_PTR(ppGroups, dwErr);
    } else {
        ppGroups = _wbc_realloc(*pppGroupList,
                    (groupInfoSize+1) * sizeof(char*));
        BAIL_ON_NULL_PTR(ppGroups, dwErr);
    }

    for (i=0; i<groupInfoSize; i++) {
        ppGroups[nGroups] = _wbc_strdup(ppGroupInfo[i]->pszName);
        BAIL_ON_NULL_PTR(ppGroups[nGroups], dwErr);

        nGroups++;
    }

    /* Terminate */

    ppGroups[nGroups] = NULL;

    *pppGroupList = ppGroups;
        *pGroupSize = nGroups;

    dwErr = LW_ERROR_SUCCESS;
cleanup:
    if (dwErr != LW_ERROR_SUCCESS)  {
        _WBC_FREE(ppGroups);
    }

    return dwErr;
}


wbcErr wbcListGroups(const char *domain_name,
             uint32_t *num_groups,
             const char ***groups)
{
    LSA_GROUP_INFO_0 **pGroupInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;
    DWORD dwNumGroups = 0;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    bool bDone = false;
    uint32_t groupSize = 0;
    char **groupList = NULL;

    /* For now ignore the domain name nutil the LsaXXX() API supports it */

    BAIL_ON_NULL_PTR_PARAM(groups, dwErr);
    BAIL_ON_NULL_PTR_PARAM(num_groups, dwErr);

    *groups = NULL;
    *num_groups = 0;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaBeginEnumGroups(hLsa, 0, 250, 0, &hResume);
    BAIL_ON_LSA_ERR(dwErr);

    while (!bDone) {
        dwErr = LsaEnumGroups(hLsa, hResume,
                      &dwNumGroups,
                      (PVOID**)&pGroupInfo);
        BAIL_ON_LSA_ERR(dwErr);

        /* Add new groups to list */

        dwErr = AddGroupsToList(&groupList, &groupSize,
                    pGroupInfo, dwNumGroups);
        BAIL_ON_LSA_ERR(dwErr);

        /* FIXME! */

        if (dwNumGroups == 0) {
            bDone = true;
            continue;
        }

        LsaFreeGroupInfoList(0, (PVOID*)pGroupInfo, dwNumGroups);
        pGroupInfo = NULL;

    }

    *groups = (const char **)groupList;
    *num_groups = groupSize;

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(groupList);
    }

    if (hResume) {
        LsaEndEnumGroups(hLsa, hResume);
        hResume = (HANDLE)NULL;
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (pGroupInfo) {
        LsaFreeGroupInfoList(0, (PVOID*)pGroupInfo, dwNumGroups);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

