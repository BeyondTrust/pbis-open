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
 *        lsa_wbc_idmap.c
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

wbcErr wbcQuerySidToUid(
    const struct wbcDomainSid *sid,
    uid_t *puid
    )
{
    return wbcSidToUid(
                sid,
                puid);
}

wbcErr wbcSidToUid(const struct wbcDomainSid *sid,
           uid_t *puid)
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    PSTR pszSidString = NULL;
    PSTR ppszSidList[2];
    CHAR pszAccountName[512] ="";
    LSA_USER_INFO_0 *pUserInfo = NULL;
    PLSA_SID_INFO pNameList = NULL;
        CHAR chDomainSeparator = 0;

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);

    /* Validate the SID */

    wbc_status = wbcSidToString(sid, &pszSidString);
    dwErr = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(dwErr);

    ppszSidList[0] = pszSidString;
    ppszSidList[1] = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaGetNamesBySidList(
                hLsa,
                1,
                ppszSidList,
                &pNameList,
                &chDomainSeparator);
    BAIL_ON_LSA_ERR(dwErr);

    /* Make sure we have a user accouint */

    if (pNameList[0].accountType != AccountType_User) {
        dwErr = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Lookup the username to get the uid */

    snprintf(pszAccountName,
         sizeof(pszAccountName),
         "%s%c%s",
         pNameList[0].pszDomainName,
                 chDomainSeparator,
         pNameList[0].pszSamAccountName);

    dwErr = LsaFindUserByName(hLsa, pszAccountName, 0, (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    *puid = pUserInfo->uid;

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if (pNameList) {
        LsaFreeSIDInfoList(pNameList, 1);
    }

    if (pszSidString) {
        wbcFreeMemory(pszSidString);
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (pUserInfo) {
        LsaFreeUserInfo(0, pUserInfo);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr wbcQueryUidToSid(
    uid_t uid,
    struct wbcDomainSid *sid
    )
{
    return wbcUidToSid(
            uid,
            sid);
}

wbcErr wbcUidToSid(uid_t uid,
           struct wbcDomainSid *sid)
{
    LSA_USER_INFO_0 *pUserInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaFindUserById(hLsa, uid, 0, (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    wbc_status = wbcStringToSid(pUserInfo->pszSid, sid);
    dwErr = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (pUserInfo) {
        LsaFreeUserInfo(0, pUserInfo);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr wbcQuerySidToGid(const struct wbcDomainSid *sid,
			gid_t *pgid)
{
    return wbcSidToGid(
                sid,
                pgid);
}

wbcErr wbcSidToGid(const struct wbcDomainSid *sid,
           gid_t *pgid)
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    PSTR pszSidString = NULL;
    PSTR ppszSidList[2];
    CHAR pszAccountName[512] ="";
    LSA_GROUP_INFO_1 *pGroupInfo = NULL;
    PLSA_SID_INFO pNameList = NULL;
        CHAR chDomainSeparator = 0;

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);

    /* Validate the SID */

    wbc_status = wbcSidToString(sid, &pszSidString);
    dwErr = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(dwErr);

    ppszSidList[0] = pszSidString;
    ppszSidList[1] = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaGetNamesBySidList(
                hLsa,
                1,
                ppszSidList,
        &pNameList,
                &chDomainSeparator);
    BAIL_ON_LSA_ERR(dwErr);

    /* Make sure we have a user accouint */

    if (pNameList[0].accountType != AccountType_Group) {
        dwErr = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Lookup the username to get the uid */

    snprintf(pszAccountName,
         sizeof(pszAccountName),
         "%s%c%s",
         pNameList[0].pszDomainName,
                 chDomainSeparator,
         pNameList[0].pszSamAccountName);

    dwErr = LsaFindGroupByName(hLsa, pszAccountName, LSA_FIND_FLAGS_NSS, 1, (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    *pgid = pGroupInfo->gid;

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if (pNameList) {
        LsaFreeSIDInfoList(pNameList, 1);
    }
    if (pszSidString) {
        wbcFreeMemory(pszSidString);
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(1, pGroupInfo);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr wbcQueryGidToSid(gid_t gid,
           struct wbcDomainSid *sid)
{
    return wbcGidToSid(
                gid,
                sid);
}

wbcErr wbcGidToSid(gid_t gid,
           struct wbcDomainSid *sid)
{
    LSA_GROUP_INFO_1 *pGroupInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaFindGroupById(hLsa, gid, LSA_FIND_FLAGS_NSS, 1, (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    wbc_status = wbcStringToSid(pGroupInfo->pszSid, sid);
    dwErr = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(1, pGroupInfo);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr wbcSidsToUnixIds(
    const struct wbcDomainSid *pSids,
    uint32_t num_sids,
    struct wbcUnixId *pIds
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_SUCCESS;
    PSTR pszSidString = NULL;
    PSTR* ppszSidList = NULL;
    int index = 0;
    LSA_QUERY_LIST query = { 0 };
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    BAIL_ON_NULL_PTR_PARAM(pSids, dwErr);
    BAIL_ON_NULL_PTR_PARAM(pIds, dwErr);

    dwErr = LwAllocateMemory(
                sizeof(ppszSidList[0]) * (num_sids + 1),
                (PVOID*)&ppszSidList);
    BAIL_ON_LSA_ERR(dwErr);

    for (index = 0; index < num_sids; index++)
    {
        wbc_status = wbcSidToString(&pSids[index], &pszSidString);
        dwErr = map_wbc_to_lsa_error(wbc_status);
        BAIL_ON_LSA_ERR(dwErr);

        ppszSidList[index] = pszSidString;
        pszSidString = NULL;
    }

    ppszSidList[index] = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    query.ppszStrings = (PCSTR *)ppszSidList;
    dwErr = LsaFindObjects(
                hLsa,
                NULL,
                0,
                LSA_OBJECT_TYPE_UNDEFINED,
                LSA_QUERY_TYPE_BY_SID,
                num_sids,
                query,
                &ppObjects);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    for (index = 0; index < num_sids; index++)
    {
        if (ppObjects[index] && ppObjects[index]->enabled)
        {
            switch (ppObjects[index]->type)
            {
                case LSA_OBJECT_TYPE_GROUP:
                    pIds[index].type = WBC_ID_TYPE_GID;
                    pIds[index].id.gid = ppObjects[index]->groupInfo.gid;
                    break;
                case LSA_OBJECT_TYPE_USER:
                    pIds[index].type = WBC_ID_TYPE_UID;
                    break;
                default:
                    pIds[index].type = WBC_ID_TYPE_NOT_SPECIFIED;
                    pIds[index].id.uid = ppObjects[index]->userInfo.uid;
                    break;
            }
        }
        else
        {
            pIds[index].type = WBC_ID_TYPE_NOT_SPECIFIED;
            pIds[index].id.uid = (uid_t)-1;
        }
    }

cleanup:
    if (ppszSidList)
    {
        for (index = 0; index < num_sids; index++)
        {
            wbcFreeMemory(ppszSidList[index]);
        }
        LW_SAFE_FREE_MEMORY(ppszSidList);
    }

    if (pszSidString) {
        wbcFreeMemory(pszSidString);
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(
            num_sids,
            ppObjects);
    }
    return wbc_status;
}

wbcErr wbcAllocateUid(uid_t *puid)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcAllocateGid(gid_t *pgid)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

wbcErr
wbcRemoveUidMapping(
    uid_t uid,
    const struct wbcDomainSid *sid
    )
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

wbcErr
wbcRemoveGidMapping(
    gid_t gid,
    const struct wbcDomainSid *sid
    )
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

wbcErr wbcSetUidMapping(uid_t uid, const struct wbcDomainSid *sid)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcSetGidMapping(gid_t gid, const struct wbcDomainSid *sid)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcSetUidHwm(uid_t uid_hwm)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcSetGidHwm(gid_t gid_hwm)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

wbcErr wbcUnixIdsToSids(const struct wbcUnixId *ids, uint32_t num_ids,
			struct wbcDomainSid *sids)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}
