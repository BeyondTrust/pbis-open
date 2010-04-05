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
 *        keytab_w16.c
 *
 * Abstract:
 *
 *        Kerberos 5 keytab management library
 *
 *        Public libkeytab API (2-byte unicode version)
 * 
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 */

#include "includes.h"


DWORD
KtKrb5AddKeyW(
    PCWSTR pwszPrincipal,
    PVOID pKey,
    DWORD dwKeyLen,
    PCWSTR pwszKtPath,
    PCWSTR pwszSalt,
    PCWSTR pwszDcName,
    DWORD dwKeyVersion)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    PSTR pszPrincipal = NULL;
    PSTR pszKey = NULL;
    PSTR pszKtPath = NULL;
    PSTR pszSalt = NULL;
    PSTR pszDcName = NULL;

    pszPrincipal = awc16stombs(pwszPrincipal);
    BAIL_IF_NO_MEMORY(pszPrincipal);

    dwError = KtAllocateMemory(dwKeyLen + 1, (PVOID*)&pszKey);
    BAIL_ON_KT_ERROR(dwError);

    wc16stombs(pszKey, (const wchar16_t*)pKey, dwKeyLen + 1);

    if (pwszKtPath) {
        pszKtPath = awc16stombs(pwszKtPath);
        BAIL_IF_NO_MEMORY(pszKtPath);
    }

    pszSalt = awc16stombs(pwszSalt);
    BAIL_IF_NO_MEMORY(pszSalt);

    pszDcName = awc16stombs(pwszDcName);
    BAIL_IF_NO_MEMORY(pszDcName);

    dwError = KtKrb5AddKey(pszPrincipal, (PVOID)pszKey, dwKeyLen, pszSalt,
                           pszKtPath, pszDcName, dwKeyVersion);

cleanup:
    KT_SAFE_FREE_STRING(pszPrincipal);
    KT_SAFE_FREE_STRING(pszKey);
    KT_SAFE_FREE_STRING(pszKtPath);
    KT_SAFE_FREE_STRING(pszSalt);
    KT_SAFE_FREE_STRING(pszDcName);

    return dwError;

error:
    goto cleanup;
}


DWORD
KtKrb5FormatPrincipalW(
    PCWSTR pwszAccount,
    PCWSTR pwszRealm,
    PWSTR *ppwszPrincipal)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    PSTR pszAccount = NULL;
    PSTR pszRealm = NULL;
    PSTR pszPrincipal = NULL;

    pszAccount = awc16stombs(pwszAccount);
    BAIL_IF_NO_MEMORY(pszAccount);

    /* We don' bail out on NULL returned as it is a valid argument
       of KtKrb5FormatPrincpal */
    pszRealm = awc16stombs(pwszRealm);

    dwError = KtKrb5FormatPrincipal(pszAccount, pszRealm, &pszPrincipal);
    BAIL_ON_KT_ERROR(dwError);

    *ppwszPrincipal = ambstowc16s(pszPrincipal);

cleanup:
    KT_SAFE_FREE_STRING(pszAccount);
    KT_SAFE_FREE_STRING(pszRealm);
    KT_SAFE_FREE_STRING(pszPrincipal);

    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
