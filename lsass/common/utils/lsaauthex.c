/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lsaauthex.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
