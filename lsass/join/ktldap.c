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
 *        ktldap.c
 *
 * Abstract:
 *
 *        Kerberos 5 keytab functions
 * 
 *        LDAP API
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
KtLdapBind(
    LDAP  **ppLd,
    PCSTR   pszDc
    )
{
    const int version = LDAP_VERSION3;
    DWORD dwError = ERROR_SUCCESS;
    int lderr = 0;
    PSTR pszUrl = NULL;
    LDAP *pLd = NULL;

    dwError = LwAllocateStringPrintf(&pszUrl,
                                     "ldap://%s",
                                     pszDc);
    BAIL_ON_LSA_ERROR(dwError);

    lderr = ldap_initialize(&pLd,
                            pszUrl);
    BAIL_ON_LDAP_ERROR(lderr);

    lderr = ldap_set_option(pLd,
                            LDAP_OPT_PROTOCOL_VERSION,
                            &version);
    BAIL_ON_LDAP_ERROR(lderr);

    lderr = ldap_set_option(pLd,
                            LDAP_OPT_REFERRALS,
                            LDAP_OPT_OFF);
    BAIL_ON_LDAP_ERROR(lderr);

    dwError = LwLdapBindDirectorySasl(pLd, pszDc, FALSE);
    BAIL_ON_LSA_ERROR(dwError);

    *ppLd = pLd;

cleanup:
    LW_SAFE_FREE_MEMORY(pszUrl);

    if (dwError == ERROR_SUCCESS &&
        lderr != LDAP_SUCCESS)
    {
        dwError = LwMapLdapErrorToLwError(lderr);
    }

    return dwError;

error:
    if (pLd)
    {
        ldap_memfree(pLd);
    }

    *ppLd = NULL;

    goto cleanup;
}

static
DWORD
KtLdapQuery(
    LDAP  *pLd,
    PCSTR  pszBaseDn,
    DWORD  dwScope,
    PCSTR  pszFilter,
    PCSTR  pszAttrName,
    PSTR  *ppszAttrVal
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    char *attrs[2] = {
        NULL, // This gets filled in later
        NULL  // This null terminates the list of attributes
    };
    LDAPMessage *res = NULL;
    LDAPMessage *entry = NULL;
    char *attr = NULL;
    struct berval **ppBv = NULL;
    BerElement *ptr = NULL;
    struct timeval timeout = { .tv_sec  = 10,
                               .tv_usec = 0 };
    PSTR pszAttrVal = NULL;

    dwError = LwAllocateString(pszAttrName,
                               &attrs[0]);
    BAIL_ON_LSA_ERROR(dwError);

    lderr = ldap_search_ext_s(pLd,
                              pszBaseDn,
                              dwScope,
                              pszFilter,
                              attrs,
                              0,
                              NULL,
                              NULL,
                              &timeout,
                              0,
                              &res);
    BAIL_ON_LDAP_ERROR(lderr);

    if (ldap_count_entries(pLd, res))
    {
        entry = ldap_first_entry(pLd, res);
        if (entry == NULL)
        {
            dwError = ERROR_DS_GENERIC_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        attr = ldap_first_attribute(pLd, entry, &ptr);
        if (attr)
        {
            ppBv = ldap_get_values_len(pLd, entry, attr);
            if (ldap_count_values_len(ppBv))
            {
                dwError = LwAllocateMemory(ppBv[0]->bv_len + 1,
                                           OUT_PPVOID(&pszAttrVal));
                BAIL_ON_LSA_ERROR(dwError);

                memcpy(pszAttrVal, ppBv[0]->bv_val, ppBv[0]->bv_len);
            }

            ldap_memfree(attr);
        }

        ldap_msgfree(res);
    }

    *ppszAttrVal = pszAttrVal;

cleanup:
    if (ppBv)
    {
        ldap_value_free_len(ppBv);
    }

    if (ptr != NULL)
    {
        ber_free( ptr, 0 );
    }

    LW_SAFE_FREE_STRING(attrs[0]);

    if (dwError == ERROR_SUCCESS &&
        lderr != LDAP_SUCCESS)
    {
        dwError = LwMapLdapErrorToLwError(lderr);
    }

    return dwError;

error:
    *ppszAttrVal = NULL;

    goto cleanup;
}

static
DWORD
KtLdapUnbind(
    LDAP *pLd
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;

    lderr = ldap_unbind_ext_s(pLd, NULL, NULL);
    BAIL_ON_LDAP_ERROR(lderr);

cleanup:
    if (lderr)
    {
        dwError = LwMapLdapErrorToLwError(lderr);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
KtLdapGetBaseDnA(
    PCSTR  pszDcName,
    PSTR  *ppszBaseDn
    )
{
    PCSTR pszDefBaseDn = "";
    PCSTR pszDefNamingCtxAttr = "defaultNamingContext";

    DWORD dwError = ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszFilter = "(objectClass=*)";

    /* Bind to directory service on the DC */
    dwError = KtLdapBind(&pLd, pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    /* Get naming context first */
    dwError = KtLdapQuery(pLd,
                          pszDefBaseDn,
                          LDAP_SCOPE_BASE,
                          pszFilter,
                          pszDefNamingCtxAttr,
                          &pszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszBaseDn = pszBaseDn;

cleanup:
    /* Close connection to the directory */
    if (pLd)
    {
        KtLdapUnbind(pLd);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszBaseDn);

    *ppszBaseDn = NULL;

    goto cleanup;
}


DWORD
KtLdapGetBaseDnW(
    PCWSTR  pwszDcName,
    PWSTR  *ppwszBaseDn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PWSTR pwszBaseDn = NULL;

    dwError = LwWc16sToMbs(pwszDcName, &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtLdapGetBaseDnA(pszDcName, &pszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszBaseDn)
    {
        dwError = LwMbsToWc16s(pszBaseDn, &pwszBaseDn);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppwszBaseDn = pwszBaseDn;

cleanup:
    LW_SAFE_FREE_MEMORY(pszBaseDn);
    LW_SAFE_FREE_MEMORY(pszDcName);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszBaseDn);
    *ppwszBaseDn = NULL;

    goto cleanup;
}


DWORD
KtLdapGetKeyVersionA(
    PCSTR   pszDcName,
    PCSTR   pszBaseDn,
    PCSTR   pszPrincipal,
    PDWORD  pdwKvno
    )
{
    PCSTR pszKvnoAttr = "msDS-KeyVersionNumber";
    PCSTR pszSamAcctAttr = "sAMAccountName";

    DWORD dwError = ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR pszRealm = NULL;
    PSTR pszAcctName = NULL;
    PSTR pszFilter = NULL;    
    PSTR pszKvnoVal = NULL;
    DWORD dwKvno = 0;

    /* Bind to directory service on the DC */
    dwError = KtLdapBind(&pLd, pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    /* Extract a username by cutting off the realm part of principal */
    dwError = LwAllocateString(pszPrincipal, &pszAcctName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrChr(pszAcctName, '@', &pszRealm);
    pszRealm[0] = '\0';

    /* Prepare ldap query filter */
    dwError = LwAllocateStringPrintf(&pszFilter,
                                     "(%s=%s)",
                                     pszSamAcctAttr,
                                     pszAcctName);
    BAIL_ON_LSA_ERROR(dwError);

    /* Look for key version number attribute */
    dwError = KtLdapQuery(pLd,
                          pszBaseDn,
                          LDAP_SCOPE_SUBTREE,
                          pszFilter,
                          pszKvnoAttr,
                          &pszKvnoVal);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszKvnoVal == NULL)
    {
        dwError = ERROR_FILE_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwKvno = atoi(pszKvnoVal);
    }

    *pdwKvno = dwKvno;

cleanup:
    /* Close connection to the directory */
    if (pLd)
    {
        KtLdapUnbind(pLd);
    }

    LW_SAFE_FREE_MEMORY(pszAcctName);
    LW_SAFE_FREE_MEMORY(pszFilter);
    LW_SAFE_FREE_MEMORY(pszKvnoVal);

    return dwError;

error:
    *pdwKvno = (DWORD)(-1);

    goto cleanup;
}



DWORD
KtLdapGetKeyVersionW(
    PCWSTR pwszDcName,
    PCWSTR pwszBaseDn,
    PCWSTR pwszPrincipal,
    PDWORD pdwKvno)
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszPrincipal = NULL;

    dwError = LwWc16sToMbs(pwszDcName, &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszBaseDn, &pszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszPrincipal, &pszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtLdapGetKeyVersionA(pszDcName, pszBaseDn, pszPrincipal, pdwKvno);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszDcName);
    LW_SAFE_FREE_STRING(pszBaseDn);
    LW_SAFE_FREE_STRING(pszPrincipal);

    return dwError;

error:
    goto cleanup;
}


DWORD
KtLdapGetSaltingPrincipalA(
    PCSTR  pszDcName,
    PCSTR  pszBaseDn,
    PCSTR  pszMachAcctName,
    PSTR  *ppszSalt
    )
{
    PCSTR pszUpnAttr = "userPrincipalName";
    PCSTR pszSamAcctAttr = "sAMAccountName";

    DWORD dwError = ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR pszFilter = NULL;
    PSTR pszUpn = NULL;

    /* Bind to directory service on the DC */
    dwError = KtLdapBind(&pLd, pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    /* Prepare ldap query filter */
    dwError = LwAllocateStringPrintf(&pszFilter,
                                     "(%s=%s)",
                                     pszSamAcctAttr,
                                     pszMachAcctName);
    BAIL_ON_LSA_ERROR(dwError);

    /* Look for key version number attribute */
    dwError = KtLdapQuery(pLd,
                          pszBaseDn,
                          LDAP_SCOPE_SUBTREE,
                          pszFilter,
                          pszUpnAttr,
                          &pszUpn);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSalt = pszUpn;

cleanup:
    /* Close connection to the directory */
    if (pLd)
    {
        KtLdapUnbind(pLd);
    }

    LW_SAFE_FREE_MEMORY(pszFilter);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszUpn);

    *ppszSalt = NULL;

    goto cleanup;
}


DWORD
KtLdapGetSaltingPrincipalW(
    PCWSTR  pwszDcName,
    PCWSTR  pwszBaseDn,
    PCWSTR  pwszMachAcctName,
    PWSTR  *ppwszSalt
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszMachAcctName = NULL;
    PSTR pszSalt = NULL;
    PWSTR pwszSalt = NULL;

    dwError = LwWc16sToMbs(pwszDcName, &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszBaseDn, &pszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszMachAcctName, &pszMachAcctName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtLdapGetSaltingPrincipalA(pszDcName,
                                         pszBaseDn,
                                         pszMachAcctName,
                                         &pszSalt);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszSalt)
    {
        dwError = LwMbsToWc16s(pszSalt, &pwszSalt);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppwszSalt = pwszSalt;

cleanup:
    LW_SAFE_FREE_STRING(pszDcName);
    LW_SAFE_FREE_STRING(pszBaseDn);
    LW_SAFE_FREE_STRING(pszMachAcctName);

    return dwError;

error:
    *ppwszSalt = NULL;
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
