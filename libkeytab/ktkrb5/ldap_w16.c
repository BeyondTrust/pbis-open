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
 *        ldap_w16.c
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
KtLdapGetBaseDnW(
    PCWSTR pwszDcName,
    PWSTR *pwszBaseDn)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;

    pszDcName = awc16stombs(pwszDcName);
    BAIL_IF_NO_MEMORY(pszDcName);

    dwError = KtLdapGetBaseDn(pszDcName, &pszBaseDn);
    BAIL_ON_KT_ERROR(dwError);

    if (pszBaseDn) {
        *pwszBaseDn = ambstowc16s(pszBaseDn);
        BAIL_IF_NO_MEMORY(*pwszBaseDn);
    }

cleanup:
    KT_SAFE_FREE_STRING(pszBaseDn);
    KT_SAFE_FREE_STRING(pszDcName);

    return dwError;

error:
    goto cleanup;
}


DWORD
KtLdapGetKeyVersionW(
    PCWSTR pwszDcName,
    PCWSTR pwszBaseDn,
    PCWSTR pwszPrincipal,
    DWORD *dwKvno)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszPrincipal = NULL;

    pszDcName = awc16stombs(pwszDcName);
    BAIL_IF_NO_MEMORY(pszDcName);

    pszBaseDn = awc16stombs(pwszBaseDn);
    BAIL_IF_NO_MEMORY(pszBaseDn);

    pszPrincipal = awc16stombs(pwszPrincipal);
    BAIL_IF_NO_MEMORY(pszPrincipal);

    dwError = KtLdapGetKeyVersion(pszDcName, pszBaseDn, pszPrincipal, dwKvno);

cleanup:
    KT_SAFE_FREE_STRING(pszDcName);
    KT_SAFE_FREE_STRING(pszBaseDn);
    KT_SAFE_FREE_STRING(pszPrincipal);

    return dwError;

error:
    goto cleanup;
}


DWORD
KtLdapGetSaltingPrincipalW(
    PCWSTR pwszDcName,
    PCWSTR pwszBaseDn,
    PCWSTR pwszMachAcctName,
    PWSTR *pwszSalt)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszMachAcctName = NULL;
    PSTR pszSalt = NULL;

    pszDcName = awc16stombs(pwszDcName);
    BAIL_IF_NO_MEMORY(pszDcName);

    pszBaseDn = awc16stombs(pwszBaseDn);
    BAIL_IF_NO_MEMORY(pszBaseDn);

    pszMachAcctName = awc16stombs(pwszMachAcctName);
    BAIL_IF_NO_MEMORY(pszMachAcctName);

    dwError = KtLdapGetSaltingPrincipal(pszDcName, pszBaseDn, pszMachAcctName,
                                        &pszSalt);
    BAIL_ON_KT_ERROR(dwError);

    if (pszSalt) {
        *pwszSalt = ambstowc16s(pszSalt);
        BAIL_IF_NO_MEMORY(*pwszSalt);
    }

cleanup:
    KT_SAFE_FREE_STRING(pszDcName);
    KT_SAFE_FREE_STRING(pszBaseDn);
    KT_SAFE_FREE_STRING(pszMachAcctName);

    return dwError;

error:
    *pwszSalt = NULL;
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
