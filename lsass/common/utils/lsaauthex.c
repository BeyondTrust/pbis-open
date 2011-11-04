/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lsaauthex.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "includes.h"

VOID
LsaFreeAuthUserPamInfo(
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo
    )
{
    if (pPamAuthInfo)
    {
        LW_SAFE_FREE_STRING(pPamAuthInfo->pszMessage);
        LW_SAFE_FREE_MEMORY(pPamAuthInfo);
    }
}

DWORD
LsaFreeAuthUserInfo(
    PLSA_AUTH_USER_INFO *ppAuthUserInfo
    )
{
    PLSA_AUTH_USER_INFO p = NULL;
    DWORD i = 0;

    if (!ppAuthUserInfo || !*ppAuthUserInfo)
    {
        return LW_ERROR_SUCCESS;
    }

    p = *ppAuthUserInfo;

    LW_SAFE_FREE_MEMORY(p->pszAccount);
    LW_SAFE_FREE_MEMORY(p->pszUserPrincipalName);
    LW_SAFE_FREE_MEMORY(p->pszFullName);
    LW_SAFE_FREE_MEMORY(p->pszDomain);
    LW_SAFE_FREE_MEMORY(p->pszDnsDomain);

    if (p->pSessionKey) {
        LsaDataBlobFree(&p->pSessionKey);
    }

    if (p->pLmSessionKey) {
        LsaDataBlobFree(&p->pLmSessionKey);
    }

    LW_SAFE_FREE_MEMORY(p->pszLogonServer);
    LW_SAFE_FREE_MEMORY(p->pszLogonScript);
    LW_SAFE_FREE_MEMORY(p->pszProfilePath);
    LW_SAFE_FREE_MEMORY(p->pszHomeDirectory);
    LW_SAFE_FREE_MEMORY(p->pszHomeDrive);

    LW_SAFE_FREE_MEMORY(p->pszDomainSid);

    LW_SAFE_FREE_MEMORY(p->pRidAttribList);

    for (i = 0; i < p->dwNumSids; i++)
    {
        LW_SAFE_FREE_MEMORY(p->pSidAttribList[i].pszSid);
    }

    LW_SAFE_FREE_MEMORY(p->pSidAttribList);


    LW_SAFE_FREE_MEMORY(p);

    *ppAuthUserInfo = NULL;

    return LW_ERROR_SUCCESS;
}


DWORD
LsaFreeAuthUserParams(
    PLSA_AUTH_USER_PARAMS *ppAuthUserParams
    )
{
    PLSA_AUTH_USER_PARAMS p = NULL;

    if (!ppAuthUserParams || !*ppAuthUserParams)
    {
        return LW_ERROR_SUCCESS;
    }

    p = *ppAuthUserParams;

    LW_SAFE_FREE_MEMORY(p->pszAccountName);
    LW_SAFE_FREE_MEMORY(p->pszDomain);
    LW_SAFE_FREE_MEMORY(p->pszWorkstation);

    switch (p->AuthType)
    {
    case LSA_AUTH_PLAINTEXT:
        LW_SECURE_FREE_STRING(p->pass.clear.pszPassword);
        break;
    case LSA_AUTH_CHAP:
        LsaDataBlobFree(&p->pass.chap.pChallenge);
        LsaDataBlobFree(&p->pass.chap.pNT_resp);
        LsaDataBlobFree(&p->pass.chap.pLM_resp);
        break;
    }

    LW_SAFE_FREE_MEMORY(p);

    *ppAuthUserParams = NULL;

    return LW_ERROR_SUCCESS;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
