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
 *        lsa_wbc_idmap.c
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
#include <stdio.h>

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

