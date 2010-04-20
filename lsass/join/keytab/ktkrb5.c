/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsakrb5.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Kerberos 5 API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rszczesniak@likewisesoftware.com)
 *
 */

#include "includes.h"


DWORD
KtGetSaltingPrincipalA(
    PCSTR pszMachineName,
    PCSTR pszMachAcctName,
    PCSTR pszDnsDomainName,
    PCSTR pszRealmName,
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PSTR *pszSalt)
{
    DWORD dwError = ERROR_SUCCESS;
    krb5_error_code ret = 0;
    PSTR pszSaltOut = NULL;
    PSTR pszRealm = NULL;
    PSTR pszMachine = NULL;
    krb5_context ctx = NULL;

    /* Try to query for userPrincipalName attribute first */
    dwError = KtLdapGetSaltingPrincipalA(pszDcName,
                                         pszBaseDn,
                                         pszMachAcctName,
                                         &pszSaltOut);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszSaltOut)
    {
        *pszSalt = pszSaltOut;
        goto cleanup;
    }

    if (pszRealmName)
    {
        /* Use passed realm name */
        dwError = LwAllocateString(pszRealmName, &pszRealm);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* No realm name was passed so get the default */
        ret = krb5_init_context(&ctx);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

        ret = krb5_get_default_realm(ctx, &pszRealm);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
    }

    /* Ensure realm name uppercased */
    LwStrToUpper(pszRealm);

    /* Ensure host name lowercased */
    dwError = LwAllocateString(pszMachineName, &pszMachine);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToLower(pszMachine);

    dwError = LwAllocateStringPrintf(&pszSaltOut,
                                     "host/%s.%s@%s",
                                     pszMachine,
                                     pszDnsDomainName,
                                     pszRealm);
    BAIL_ON_LSA_ERROR(dwError);

    *pszSalt = pszSaltOut;

cleanup:
    if (ctx)
    {
        krb5_free_context(ctx);
    }

    LW_SAFE_FREE_MEMORY(pszRealm);
    LW_SAFE_FREE_MEMORY(pszMachine);

    return dwError;

error:
    *pszSalt = NULL;
    goto cleanup;
}


DWORD
KtGetSaltingPrincipalW(
    PCWSTR  pwszMachineName,
    PCWSTR  pwszMachAcctName,
    PCWSTR  pwszDnsDomainName,
    PCWSTR  pwszRealmName,
    PCWSTR  pwszDcName,
    PCWSTR  pwszBaseDn,
    PWSTR  *ppwszSalt
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszMachineName = NULL;
    PSTR pszMachAcctName = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszRealmName = NULL;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszSalt = NULL;
    PWSTR pwszSalt = NULL;

    dwError = LwWc16sToMbs(pwszMachineName,
                           &pszMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszMachAcctName,
                           &pszMachAcctName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszDnsDomainName,
                           &pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszDcName,
                           &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszBaseDn,
                           &pszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszRealmName)
    {
        dwError = LwWc16sToMbs(pwszRealmName,
                               &pszRealmName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = KtGetSaltingPrincipalA(pszMachineName,
                                     pszMachAcctName,
                                     pszDnsDomainName,
                                     pszRealmName,
                                     pszDcName,
                                     pszBaseDn,
                                     &pszSalt);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszSalt)
    {
        dwError = LwMbsToWc16s(pszSalt, &pwszSalt);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppwszSalt = pwszSalt;

cleanup:
    LW_SAFE_FREE_MEMORY(pszMachineName);
    LW_SAFE_FREE_MEMORY(pszMachAcctName);
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pszRealmName);
    LW_SAFE_FREE_MEMORY(pszDcName);
    LW_SAFE_FREE_MEMORY(pszBaseDn);
    LW_SAFE_FREE_MEMORY(pszSalt);

    return dwError;

error:
    pwszSalt = NULL;
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
