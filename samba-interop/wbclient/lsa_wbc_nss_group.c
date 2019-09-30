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
 *        lsa_wbc_nss_group.c
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

static int FreeStructGroup(void *p)
{
    struct group *grp = (struct group*)p;

    if (!p)
        return 0;

    _WBC_FREE(grp->gr_name);
    _WBC_FREE(grp->gr_passwd);
    _WBC_FREE(grp->gr_mem);

    return 0;
}

static DWORD CopyGroupMembers(struct group *gr, LSA_GROUP_INFO_1 *pGroup)
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    int i;

    /* Easy case is when there are no listed members */

    if (pGroup->ppszMembers == NULL) {
        return LW_ERROR_SUCCESS;
    }

    /* Get the number of group members */

    for (i=0; pGroup->ppszMembers[i]; i++) {
        /* do nothing; just get count */;
    }
    i++;

    /* Don't forget terminating NULL string */

    gr->gr_mem = _wbc_malloc_zero(sizeof(char*)*i, _wbc_free_string_array);
    BAIL_ON_NULL_PTR(gr->gr_mem, dwErr);

    /* Now copy */

    for (i=0; pGroup->ppszMembers[i]; i++) {
        gr->gr_mem[i] = _wbc_strdup(pGroup->ppszMembers[i]);
        BAIL_ON_NULL_PTR(gr->gr_mem[i], dwErr);
    }

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    return dwErr;
}

static DWORD FillStructGroupFromGroupInfo0(struct group **grp, LSA_GROUP_INFO_1 *pGroup)
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    struct group *gr = NULL;

    gr = _wbc_malloc_zero(sizeof(struct group), FreeStructGroup);
    BAIL_ON_NULL_PTR(gr, dwErr);

    gr->gr_gid = pGroup->gid;

    /* Always have to have a name */

    gr->gr_name = _wbc_strdup(pGroup->pszName);
    BAIL_ON_NULL_PTR(gr->gr_name, dwErr);

    /* Gecos and passwd fields are technically optional */

    if (pGroup->pszPasswd) {
        gr->gr_passwd = _wbc_strdup(pGroup->pszPasswd);
    } else {
        gr->gr_passwd = _wbc_strdup("x");
    }
    BAIL_ON_NULL_PTR(gr->gr_passwd, dwErr);

    dwErr = CopyGroupMembers(gr, pGroup);
    BAIL_ON_LSA_ERR(dwErr);

    *grp = gr;
    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        if (gr) {
            _WBC_FREE(gr);
        }
    }

    return dwErr;
}

wbcErr wbcGetgrnam(const char *name, struct group **grp)
{
    LSA_GROUP_INFO_1 *pGroupInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(name, dwErr);
    BAIL_ON_NULL_PTR_PARAM(grp, dwErr);

    *grp = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaFindGroupByName(hLsa, name, LSA_FIND_FLAGS_NSS, 1, (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = FillStructGroupFromGroupInfo0(grp, pGroupInfo);
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(*grp);
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


wbcErr wbcGetgrgid(gid_t gid, struct group **grp)
{
    LSA_GROUP_INFO_1 *pGroupInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(grp, dwErr);

    *grp = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaFindGroupById(hLsa, gid, LSA_FIND_FLAGS_NSS, 1, (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = FillStructGroupFromGroupInfo0(grp, pGroupInfo);
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(*grp);
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


wbcErr wbcSetgrent(void)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcEndgrent(void)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


wbcErr wbcGetgrent(struct group **grp)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

wbcErr wbcGetgrlist(struct group **grp)
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

