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
 *        lsa_wbc_nss_user.c
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
#include <memory.h>
#include <lwmem.h>
#include <string.h>

static int FreeStructPasswd(void *p)
{
    struct passwd *pw = p;

    if (!p)
        return 0;

    _WBC_FREE(pw->pw_name);
    _WBC_FREE(pw->pw_gecos);
    _WBC_FREE(pw->pw_shell);
    _WBC_FREE(pw->pw_dir);

    return 0;
}

static DWORD FillStructPasswdFromUserInfo0(struct passwd **pwd, LSA_USER_INFO_0 *pUser)
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    struct passwd *pw = NULL;

    pw = _wbc_malloc_zero(sizeof(struct passwd), FreeStructPasswd);
    BAIL_ON_NULL_PTR(pw, dwErr);

    pw->pw_uid = pUser->uid;
    pw->pw_gid = pUser->gid;

    /* Always have to have a name, loginShell, and homedir */

    pw->pw_name = _wbc_strdup(pUser->pszName);
    BAIL_ON_NULL_PTR(pw->pw_name, dwErr);

    pw->pw_dir = _wbc_strdup(pUser->pszHomedir);
    BAIL_ON_NULL_PTR(pw->pw_dir, dwErr);

    pw->pw_shell = _wbc_strdup(pUser->pszShell);
    BAIL_ON_NULL_PTR(pw->pw_shell, dwErr);

    /* Gecos and passwd fields are technically optional */

    if (pUser->pszGecos) {
        pw->pw_gecos = _wbc_strdup(pUser->pszGecos);
        BAIL_ON_NULL_PTR(pw->pw_gecos, dwErr);
    }

    if (pUser->pszPasswd) {
        pw->pw_passwd = _wbc_strdup(pUser->pszPasswd);
    } else {
        pw->pw_passwd = _wbc_strdup("x");
    }
    BAIL_ON_NULL_PTR(pw->pw_passwd, dwErr);

    *pwd = pw;
    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        if (pw) {
            _WBC_FREE(pw);
        }
    }

    return dwErr;
}

static
DWORD
FillStructPasswdFromUser(
    PLSA_SECURITY_OBJECT pUser,
    struct passwd **pwd
    )
{
    DWORD error = 0;
    struct passwd *pw = NULL;
    DWORD requiredSize = 0;
    char *pDataPtr = NULL;

    requiredSize = sizeof(struct passwd);
    requiredSize += strlen(pUser->userInfo.pszUnixName) + 1;
    requiredSize += strlen(pUser->userInfo.pszHomedir) + 1;
    requiredSize += strlen(pUser->userInfo.pszShell) + 1;
    if (pUser->userInfo.pszGecos)
    {
        requiredSize += strlen(pUser->userInfo.pszGecos) + 1;
    }
    requiredSize += strlen(pUser->userInfo.pszPasswd ?
            pUser->userInfo.pszPasswd : "x") + 1;

    pw = _wbc_malloc_zero(sizeof(struct passwd), NULL);
    BAIL_ON_NULL_PTR(pw, error);

    pDataPtr = (char *)pw + sizeof(struct passwd);

    pw->pw_uid = pUser->userInfo.uid;
    pw->pw_gid = pUser->userInfo.gid;

    /* Always have to have a name, loginShell, and homedir */

    pw->pw_name = pDataPtr;
    strcpy(pDataPtr, pUser->userInfo.pszUnixName);
    pDataPtr += strlen(pDataPtr) + 1;

    pw->pw_dir = pDataPtr;
    strcpy(pDataPtr, pUser->userInfo.pszHomedir);
    pDataPtr += strlen(pDataPtr) + 1;

    pw->pw_shell = pDataPtr;
    strcpy(pDataPtr, pUser->userInfo.pszShell);
    pDataPtr += strlen(pDataPtr) + 1;

    /* Gecos and passwd fields are technically optional */

    if (pUser->userInfo.pszGecos)
    {
        pw->pw_gecos = pDataPtr;
        strcpy(pDataPtr, pUser->userInfo.pszGecos);
        pDataPtr += strlen(pDataPtr) + 1;
    }

    pw->pw_passwd = pDataPtr;
    strcpy(pDataPtr,
            pUser->userInfo.pszPasswd ?  pUser->userInfo.pszPasswd : "x");
    pDataPtr += strlen(pDataPtr) + 1;

    *pwd = pw;

cleanup:
    if (error != LW_ERROR_SUCCESS)
    {
        if (pw)
        {
            _WBC_FREE(pw);
        }
        *pwd = NULL;
    }

    return error;
}

wbcErr wbcGetpwnam(const char *name, struct passwd **pwd)
{
    LSA_USER_INFO_0 *pUserInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(name, dwErr);
    BAIL_ON_NULL_PTR_PARAM(pwd, dwErr);

    *pwd = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaFindUserByName(hLsa, name, 0, (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = FillStructPasswdFromUserInfo0(pwd, pUserInfo);
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(*pwd);
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



wbcErr wbcGetpwuid(uid_t uid, struct passwd **pwd)
{
    LSA_USER_INFO_0 *pUserInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(pwd, dwErr);

    *pwd = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaFindUserById(hLsa, uid, 0, (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = FillStructPasswdFromUserInfo0(pwd, pUserInfo);
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(*pwd);
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

wbcErr wbcSetpwent(void)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcEndpwent(void)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcGetpwent(struct passwd **pwd)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

wbcErr wbcGetGroups(const char *account,
            uint32_t *num_groups,
            gid_t **groups)
{
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    DWORD dwNumGids = 0;
    gid_t *gids = NULL;

    BAIL_ON_NULL_PTR_PARAM(groups, dwErr);
    BAIL_ON_NULL_PTR_PARAM(num_groups, dwErr);

    *groups = NULL;
    *num_groups = 0;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaGetGidsForUserByName(hLsa, account,
                    &dwNumGids, &gids);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    *groups = _wbc_malloc_zero(sizeof(gid_t)*dwNumGids, NULL);
    BAIL_ON_NULL_PTR(*groups, dwErr);

    memcpy(*groups, gids, sizeof(gid_t)*dwNumGids);
    *num_groups = dwNumGids;

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(*groups);
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (gids) {
        LwFreeMemory(gids);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr
wbcGetpwsid(
    struct wbcDomainSid * sid,
    struct passwd **pwd
    )
{
    DWORD error = 0;
    PLSA_SECURITY_OBJECT pUser = NULL;
    struct passwd *pwdLocal = NULL;

    BAIL_ON_NULL_PTR(pwd, error);

    error = wbcFindSecurityObjectBySid(
        sid,
        &pUser);
    BAIL_ON_LSA_ERR(error);

    FillStructPasswdFromUser(
        pUser,
        &pwdLocal);
    BAIL_ON_LSA_ERR(error);

    *pwd = pwdLocal;

cleanup:
    if (pUser)
    {
        LsaFreeSecurityObject(pUser);
    }

    if (error != LW_ERROR_SUCCESS)
    {
        if (pwd)
        {
            *pwd = NULL;
        }
        if (pwdLocal)
        {
            _WBC_FREE(pwdLocal);
        }
    }

    return map_error_to_wbc_status(error);
}

