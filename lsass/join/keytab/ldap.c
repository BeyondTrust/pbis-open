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
 *        ktldap.c
 *
 * Abstract:
 *
 *        Kerberos 5 keytab management library
 * 
 *        LDAP API
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


DWORD
KtLdapBind(
    LDAP **ldret,
    PCSTR pszDc)
{
    const int version = LDAP_VERSION3;
    DWORD dwError = 0;
    int lderr = 0;
    LDAP *ld = NULL;
    int secflags = GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG | GSS_C_INTEG_FLAG;

    *ldret = NULL;

    ld = ldap_open(pszDc, LDAP_PORT);
    if (!ld) BAIL_WITH_KT_ERROR(KT_STATUS_LDAP_ERROR);

    lderr = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
    BAIL_ON_LDAP_ERROR(lderr);

    lderr = ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
    BAIL_ON_LDAP_ERROR(lderr);

    lderr = ldap_set_option(ld, LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL,
            LDAP_OPT_ON);
    BAIL_ON_LDAP_ERROR(lderr);

    lderr = ldap_set_option(ld, LDAP_OPT_SSPI_FLAGS, &secflags);
    BAIL_ON_LDAP_ERROR(lderr);

    lderr = ldap_bind_s(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    BAIL_ON_LDAP_ERROR(lderr);

    *ldret = ld;

cleanup:
    return dwError;

error:
    if (ld) ldap_memfree(ld);
    goto cleanup;
}


DWORD
KtLdapQuery(
    LDAP *ld,
    PCSTR pszBaseDn,
    DWORD dwScope,
    PCSTR pszFilter,
    PCSTR pszAttrName,
    PSTR *pszAttrVal)
{
    DWORD dwError = 0;
    int lderr = 0;
    char *attrs[2] = {
        NULL, // This gets filled in later
        NULL  // This null terminates the list of attributes
    };
    LDAPMessage *res = NULL;
    LDAPMessage *entry = NULL;
    char *attr = NULL;
    char **val = NULL;
    BerElement *ptr = NULL;
    struct timeval timeout = { .tv_sec  = 10,
                               .tv_usec = 0 };

    dwError = KtAllocateString(pszAttrName, &attrs[0]);
    BAIL_ON_KT_ERROR(dwError);

    lderr = ldap_search_ext_s(ld, pszBaseDn, dwScope, pszFilter, attrs, 0, NULL,
                              NULL, &timeout, 0, &res);
    BAIL_ON_LDAP_ERROR(lderr);

    if (ldap_count_entries(ld, res)) {
        entry = ldap_first_entry(ld, res);
        if (entry == NULL) BAIL_WITH_KT_ERROR(KT_STATUS_LDAP_ERROR);
        
        attr = ldap_first_attribute(ld, entry, &ptr);
        if (attr) {
            val = ldap_get_values(ld, entry, attr);

            ldap_memfree(attr);
        }

        ldap_msgfree(res);
    }

    if (val && val[0]) {
        dwError = KtAllocateString(val[0], pszAttrVal);
        BAIL_ON_KT_ERROR(dwError);

    } else {
        *pszAttrVal = NULL;
    }

cleanup:
    if (val) {
        ldap_value_free(val);
    }

    if (ptr != NULL)
    {
        ber_free( ptr, 0 );
    }

    KT_SAFE_FREE_STRING(attrs[0]);

    return dwError;

error:
    goto cleanup;
}


DWORD
KtLdapUnbind(
   LDAP *ld)
{
    DWORD dwError = 0;
    int lderr = 0;

    lderr = ldap_unbind_s(ld);
    BAIL_ON_LDAP_ERROR(lderr);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
KtLdapGetBaseDn(
    PCSTR pszDcName,
    PSTR *pszBaseDn)
{
    PCSTR pszDefNamingCtxAttr = "defaultNamingContext";

    DWORD dwError = KT_STATUS_SUCCESS;
    LDAP *ld = NULL;
    PSTR pszBaseDnVal = NULL;

    /* Bind to directory service on the DC */
    dwError = KtLdapBind(&ld, pszDcName);
    BAIL_ON_KT_ERROR(dwError);

    /* Get naming context first */
    dwError = KtLdapQuery(ld, "", LDAP_SCOPE_BASE,
                          "(objectClass=*)", pszDefNamingCtxAttr, &pszBaseDnVal);
    BAIL_ON_KT_ERROR(dwError);

    /* Close connection to the directory */
    dwError = KtLdapUnbind(ld);
    BAIL_ON_KT_ERROR(dwError);

    *pszBaseDn = pszBaseDnVal;

cleanup:
    return dwError;

error:
    KT_SAFE_FREE_STRING(pszBaseDnVal);

    *pszBaseDn = NULL;

    goto cleanup;
}


DWORD
KtLdapGetKeyVersion(
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PCSTR pszPrincipal,
    DWORD *dwKvno)
{
    PCSTR pszKvnoAttr = "msDS-KeyVersionNumber";
    PCSTR pszSamAcctAttr = "sAMAccountName";

    DWORD dwError = 0;
    LDAP *ld = NULL;
    PSTR pszRealm = NULL;
    PSTR pszAcctName = NULL;
    PSTR pszFilter = NULL;    
    PSTR pszKvnoVal = NULL;

    /* Bind to directory service on the DC */
    dwError = KtLdapBind(&ld, pszDcName);
    BAIL_ON_KT_ERROR(dwError);

    /* Extract a username by cutting off the realm part of principal */
    dwError = KtAllocateString(pszPrincipal, &pszAcctName);
    BAIL_ON_KT_ERROR(dwError);

    KtStrChr(pszAcctName, '@', &pszRealm);
    pszRealm[0] = '\0';

    /* Prepare ldap query filter */
    dwError = KtAllocateStringPrintf(&pszFilter, "(%s=%s)",
                                     pszSamAcctAttr, pszAcctName);
    BAIL_ON_KT_ERROR(dwError);

    /* Look for key version number attribute */
    dwError = KtLdapQuery(ld, pszBaseDn, LDAP_SCOPE_SUBTREE,
                          pszFilter, pszKvnoAttr, &pszKvnoVal);
    BAIL_ON_KT_ERROR(dwError);

    /* Close connection to the directory */
    dwError = KtLdapUnbind(ld);
    BAIL_ON_KT_ERROR(dwError);

    if (pszKvnoVal == NULL) {
        BAIL_WITH_KT_ERROR(KT_STATUS_LDAP_NO_KVNO_FOUND);
    }

    *dwKvno = atoi(pszKvnoVal);

cleanup:
    KT_SAFE_FREE_STRING(pszAcctName);
    KT_SAFE_FREE_STRING(pszFilter);
    KT_SAFE_FREE_STRING(pszKvnoVal);

    return dwError;

error:
    *dwKvno = (DWORD)(-1);
    goto cleanup;
}



DWORD
KtLdapGetSaltingPrincipal(
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PCSTR pszMachAcctName,
    PSTR *pszSalt)
{
    PCSTR pszUpnAttr = "userPrincipalName";
    PCSTR pszSamAcctAttr = "sAMAccountName";

    DWORD dwError = KT_STATUS_SUCCESS;
    LDAP *ld = NULL;
    PSTR pszFilter = NULL;
    PSTR pszUpnVal = NULL;

    /* Bind to directory service on the DC */
    dwError = KtLdapBind(&ld, pszDcName);
    BAIL_ON_KT_ERROR(dwError);

    /* Prepare ldap query filter */
    dwError = KtAllocateStringPrintf(&pszFilter, "(%s=%s)",
                                     pszSamAcctAttr, pszMachAcctName);
    BAIL_ON_KT_ERROR(dwError);

    /* Look for key version number attribute */
    dwError = KtLdapQuery(ld, pszBaseDn, LDAP_SCOPE_SUBTREE,
                          pszFilter, pszUpnAttr, &pszUpnVal);
    BAIL_ON_KT_ERROR(dwError);

    /* Close connection to the directory */
    dwError = KtLdapUnbind(ld);
    BAIL_ON_KT_ERROR(dwError);

    *pszSalt = pszUpnVal;

cleanup:
    KT_SAFE_FREE_STRING(pszFilter);

    return dwError;

error:
    KT_SAFE_FREE_STRING(pszUpnVal);

    *pszSalt = NULL;
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
