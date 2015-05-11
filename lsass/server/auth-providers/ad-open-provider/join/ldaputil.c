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

#include "includes.h"


static int LdapModSetStrValue(LDAPMod **mod,
                              const char *t, const wchar16_t *wsv, int chg)
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMod *m = NULL;
    int count = 0;
    char *sv = NULL;

    if (!mod || !t || !wsv) return LDAP_PARAM_ERROR;

    dwError = LwWc16sToMbs(wsv, &sv);
    BAIL_ON_LSA_ERROR(dwError);

    m = *mod;
    if (!m) {
        dwError = LwAllocateMemory(sizeof(LDAPMod),
                                   OUT_PPVOID(&m));
        BAIL_ON_LSA_ERROR(dwError);
        
        m->mod_type   = NULL;
        m->mod_values = NULL;
        m->mod_op     = chg;
        *mod = m;
    }

    if (m->mod_op != chg) {
        lderr = LDAP_PARAM_ERROR;
        goto error;

    } else {
        m->mod_op = chg;
    }

    if (m->mod_type) {
        if (strcmp(m->mod_type, t)) {
            lderr = LDAP_PARAM_ERROR;
            goto error;
        }

    } else {
        m->mod_type = strdup(t);
    }

    if (m->mod_values) {
        while (m->mod_values[++count]);
    }

    count += 2;
    dwError = LwReallocMemory(m->mod_values,
                              OUT_PPVOID(&m->mod_values),
                              sizeof(char*) * count);
    BAIL_ON_LSA_ERROR(dwError);

    m->mod_values[count - 2] = strdup(sv);
    m->mod_values[count - 1] = NULL;

error:
    LW_SAFE_FREE_MEMORY(sv);

    return lderr;
}

static
int LdapModAddStrValue(LDAPMod **mod, const char *t, const wchar16_t *sv)
{
    return LdapModSetStrValue(mod, t, sv, LDAP_MOD_ADD);
}

static
int LdapModReplStrValue(LDAPMod **mod, const char *t, const wchar16_t *sv)
{
    return LdapModSetStrValue(mod, t, sv, LDAP_MOD_REPLACE);
}


static
DWORD
LdapModAddIntValue(
    LDAPMod **mod,
    const char *t,
    const int iv
    )
{
    wchar16_t sv[11] = {0};
    if (sw16printfw(
                sv,
                sizeof(sv)/sizeof(sv[0]),
                L"%u",
                iv) < 0)
    {
        return LwErrnoToWin32Error(errno);
    }

    return LwMapLdapErrorToLwError(LdapModAddStrValue(
                mod,
                t,
                (const wchar16_t*)sv));
}

static
VOID
LdapModFree(LDAPMod **mod)
{
    LDAPMod *m;
    int i = 0;

    if (!mod) return;

    m = *mod;

    if (m) {
        LW_SAFE_FREE_MEMORY(m->mod_type);

        if (m->mod_values) {
            for (i = 0; m->mod_values[i]; i++) {
                LW_SAFE_FREE_MEMORY(m->mod_values[i]);
            }

            LW_SAFE_FREE_MEMORY(m->mod_values);
        }

        LW_SAFE_FREE_MEMORY(m);
    }

    *mod = NULL;

    return;
}


int LdapMessageFree(LDAPMessage *msg)
{
    int lderr = LDAP_SUCCESS;

    if (!msg) return LDAP_PARAM_ERROR;

    lderr = ldap_msgfree(msg);
    return lderr;
}


DWORD
LdapInitConnection(
    OUT LDAP** ldconn,
    IN PCWSTR host,
    IN BOOLEAN bSeal
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAP *ld = NULL;
    unsigned int version;
    char* ldap_srv = NULL;
    char* ldap_url = NULL;

    BAIL_ON_INVALID_POINTER(ldconn);
    BAIL_ON_INVALID_POINTER(host);

    dwError = LwWc16sToMbs(host, &ldap_srv);
    BAIL_ON_LSA_ERROR(dwError);

    if (strchr(ldap_srv, ':'))
    {
        dwError = LwAllocateStringPrintf(&ldap_url, "ldap://[%s]", ldap_srv);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(&ldap_url, "ldap://%s", ldap_srv);
        BAIL_ON_LSA_ERROR(dwError);
    }

    lderr = ldap_initialize(&ld, ldap_url);
    dwError = LwMapLdapErrorToLwError(lderr);
    BAIL_ON_LSA_ERROR(dwError);

    version = LDAP_VERSION3;
    lderr = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
    dwError = LwMapLdapErrorToLwError(lderr);
    BAIL_ON_LSA_ERROR(dwError);

    lderr = ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
    dwError = LwMapLdapErrorToLwError(lderr);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapBindDirectorySasl(
                  ld,
                  ldap_srv,
                  bSeal);
    BAIL_ON_LSA_ERROR(dwError);

    *ldconn = ld;

cleanup:
    LW_SAFE_FREE_MEMORY(ldap_url);
    LW_SAFE_FREE_MEMORY(ldap_srv);

    return dwError;

error:
    if (ld) {
        /* we don't check returned error code since we're not
           going to return it anway */
        ldap_unbind_ext_s(ld, NULL, NULL);
    }

    *ldconn = NULL;
    goto cleanup;
}


int LdapCloseConnection(LDAP *ldconn)
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;

    BAIL_ON_INVALID_POINTER(ldconn);

    lderr = ldap_unbind_ext_s(ldconn, NULL, NULL);

error:
    return lderr;
}


int LdapGetDirectoryInfo(LDAPMessage **info, LDAPMessage **result, LDAP *ld)
{
    const char *basedn = "";
    const int scope = LDAP_SCOPE_BASE;
    const char *filter = "(objectClass=*)";

    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    char *allattr[] = { NULL };
    LDAPMessage *res = NULL;
    LDAPMessage *entry = NULL;

    BAIL_ON_INVALID_POINTER(info);
    BAIL_ON_INVALID_POINTER(result);
    BAIL_ON_INVALID_POINTER(ld);

    lderr = ldap_search_ext_s(ld, basedn, scope, filter, allattr, 0,
                              NULL, NULL, NULL, 0, &res);
    BAIL_ON_LDAP_ERROR(lderr);

    entry = ldap_first_entry(ld, res);
    if (!entry) {
        lderr = LDAP_NO_SUCH_OBJECT;
        goto error;
    }

    *info   = entry;
    *result = res;

cleanup:
    return lderr;

error:
    *info   = NULL;
    *result = NULL;
    goto cleanup;
}


wchar16_t **LdapAttributeGet(LDAP *ld, LDAPMessage *info, const wchar16_t *name,
                             int *count)
{
    DWORD dwError = ERROR_SUCCESS;
    int i = 0;
    int vcount = 0;
    wchar16_t *attr_w16 = NULL;
    wchar16_t **out = NULL;
    char *attr = NULL;
    BerElement *be = NULL;
    struct berval **value = NULL;
    char *strval = NULL;

    BAIL_ON_INVALID_POINTER(info);
    BAIL_ON_INVALID_POINTER(name);

    attr = ldap_first_attribute(ld, info, &be);
    while (attr) {
        dwError = LwMbsToWc16s(attr, &attr_w16);
        BAIL_ON_LSA_ERROR(dwError);

        if (wc16scmp(attr_w16, name) == 0) {
            value = ldap_get_values_len(ld, info, attr);
            vcount = ldap_count_values_len(value);

            dwError = LwReallocMemory(out,
                                      OUT_PPVOID(&out),
                                      sizeof(wchar16_t*) * (vcount+1));
            BAIL_ON_LSA_ERROR(dwError);

            for (i = 0; i < vcount; i++) {
                dwError = LwAllocateMemory(value[i]->bv_len + 1,
                                           OUT_PPVOID(&strval));
                BAIL_ON_LSA_ERROR(dwError);

                memcpy(strval, value[i]->bv_val, value[i]->bv_len);

                dwError = LwMbsToWc16s(strval, &out[i]);
                BAIL_ON_LSA_ERROR(dwError);

                LW_SAFE_FREE_MEMORY(strval);
                strval = NULL;
            }

            if (count) *count = vcount;

            ldap_value_free_len(value);
            ber_free(be, 0);
            be = NULL;
            goto cleanup;
        }

        ldap_memfree(attr);
        attr = ldap_next_attribute(ld, info, be);

        LW_SAFE_FREE_MEMORY(attr_w16);
    }

cleanup:
    if (out) {
        out[vcount] = NULL;
    }

    LW_SAFE_FREE_MEMORY(attr_w16);
    ldap_memfree(attr);
    if (be)
    {
        ber_free(be, 0);
    }

    return out;

error:
    out = NULL;
    goto cleanup;
}


void LdapAttributeValueFree(wchar16_t *val[])
{
    int i = 0;

    if (val == NULL) return;

    for (i = 0; val[i]; i++) {
        LW_SAFE_FREE_MEMORY(val[i]);
    }

    LW_SAFE_FREE_MEMORY(val);
}

static
wchar16_t* LdapAttrValDn(const wchar16_t *name, const wchar16_t *value,
                         const wchar16_t *base)
{
    if (base) {
        return asw16printfw(
                    L"%ws=%ws,%ws",
                    name,
                    value,
                    base);
    } else {
        return asw16printfw(
                    L"%ws=%ws",
                    name,
                    value);
    }
}


wchar16_t* LdapAttrValDnsHostName(const wchar16_t *name, const wchar16_t *dnsdomain)
{
    if (dnsdomain) {
        return asw16printfw(
                    L"%ws.%ws",
                    name,
                    dnsdomain);
    } else {
        return asw16printfw(
                    L"%ws",
                    name);
    }
}


wchar16_t*
LdapAttrValSvcPrincipalName(
    const char *type,
    const wchar16_t *name
    )
{
    return asw16printfw(L"%s/%ws", type, name);
}


int
LdapMachAcctCreate(
    LDAP *ld,
    const wchar16_t *machacct_name,
    const wchar16_t *ou
    )
{
    int lderr = LDAP_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    wchar16_t *cn_name = NULL;
    wchar16_t *machname = NULL;
    size_t machname_len = 0;
    wchar16_t *dname = NULL;
    char *dn = NULL;
    wchar16_t *objclass[5] = {0};
    int flags, i;
    LDAPMod *name_m = NULL;
    LDAPMod *samacct_m = NULL;
    LDAPMod *objectclass_m = NULL;
    LDAPMod *acctflags_m = NULL;
    LDAPMod *attrs[5];

    BAIL_ON_INVALID_POINTER(ld);
    BAIL_ON_INVALID_POINTER(machacct_name);
    BAIL_ON_INVALID_POINTER(ou);

    dwError = LwAllocateWc16String(&machname,
                                   machacct_name);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(machname, &machname_len);
    BAIL_ON_LSA_ERROR(dwError);

    if (machname_len)
    {
        machname[--machname_len] = 0;
    }

    dwError = LwMbsToWc16s("cn", &cn_name);
    BAIL_ON_LSA_ERROR(dwError);

    dname = LdapAttrValDn(cn_name, machname, ou);
    if (!dname)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(dname, &dn);
    BAIL_ON_LSA_ERROR(dwError);

    objclass[0] = ambstowc16s("top");
    objclass[1] = ambstowc16s("organizationalPerson");
    objclass[2] = ambstowc16s("user");
    objclass[3] = ambstowc16s("computer");
    objclass[4] = NULL;

    flags = LSAJOIN_ACCOUNTDISABLE | LSAJOIN_WORKSTATION_TRUST_ACCOUNT;

    LdapModAddStrValue(&name_m, "name", machname);
    LdapModAddStrValue(&samacct_m, "sAMAccountName", machacct_name);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[0]);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[1]);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[2]);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[3]);
    LdapModAddIntValue(&acctflags_m, "userAccountControl", flags);

    attrs[0] = name_m;
    attrs[1] = samacct_m;
    attrs[2] = objectclass_m;
    attrs[3] = acctflags_m;
    attrs[4] = NULL;

    lderr = ldap_add_ext_s(ld, dn, attrs, NULL, NULL);
    BAIL_ON_LDAP_ERROR(lderr);

error:
    LdapModFree(&name_m);
    LdapModFree(&samacct_m);
    LdapModFree(&objectclass_m);
    LdapModFree(&acctflags_m);

    LW_SAFE_FREE_MEMORY(machname);
    LW_SAFE_FREE_MEMORY(dname);
    LW_SAFE_FREE_MEMORY(dn);
    LW_SAFE_FREE_MEMORY(cn_name);
    for (i = 0; objclass[i]; i++)
    {
        LW_SAFE_FREE_MEMORY(objclass[i]);
    }

    return lderr;
}


int
LdapMachDnsNameSearch(
    LDAPMessage **out,
    LDAP *ld,
    const wchar16_t *name,
    const wchar16_t *dns_domain_name,
    const wchar16_t *base,
    PCWSTR pSchemaContext
    )
{
    const wchar_t filter_fmt[] = L"(&(objectCategory=CN=Computer,%ws)(dNSHostName=%ws))";

    int lderr = LDAP_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    wchar16_t *dnsname = NULL;
    char *basedn = NULL;
    wchar16_t *filterw16 = NULL;
    char *filter = NULL;
    LDAPMessage *res = NULL;
    LDAPControl **sctrl = NULL;
    LDAPControl **cctrl = NULL;

    BAIL_ON_INVALID_POINTER(out);
    BAIL_ON_INVALID_POINTER(ld);
    BAIL_ON_INVALID_POINTER(name);
    BAIL_ON_INVALID_POINTER(dns_domain_name);
    BAIL_ON_INVALID_POINTER(base);

    dwError = LwWc16sToMbs(base, &basedn);
    BAIL_ON_LSA_ERROR(dwError);

    dnsname = LdapAttrValDnsHostName(name, dns_domain_name);
    if (!dnsname)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateWc16sPrintfW(
                    &filterw16,
                    filter_fmt, 
                    pSchemaContext,
                    dnsname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(filterw16, &filter);
    BAIL_ON_LSA_ERROR(dwError);

    lderr = ldap_search_ext_s(ld, basedn, LDAP_SCOPE_SUBTREE, filter, NULL, 0,
                              sctrl, cctrl, NULL, 0, &res);
    BAIL_ON_LDAP_ERROR(lderr);

    *out = res;

cleanup:
    LW_SAFE_FREE_MEMORY(filter);
    LW_SAFE_FREE_MEMORY(filterw16);
    LW_SAFE_FREE_MEMORY(dnsname);
    LW_SAFE_FREE_MEMORY(basedn);

    return lderr;

error:

    *out = NULL;
    goto cleanup;
}


int
LdapMachAcctSearch(
    LDAPMessage **out,
    LDAP *ld,
    const wchar16_t *samacct_name,
    const wchar16_t *base
    )
{
    const wchar_t filter_fmt[] = L"(&(objectClass=computer)(sAMAccountName=%ws))";

    int lderr = LDAP_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t filter_len = 0;
    size_t samacctname_len = 0;
    char *basedn = NULL;
    wchar16_t *filterw16 = NULL;
    char *filter = NULL;
    LDAPMessage *res = NULL;
    LDAPControl **sctrl = NULL;
    LDAPControl **cctrl = NULL;

    BAIL_ON_INVALID_POINTER(out);
    BAIL_ON_INVALID_POINTER(ld);
    BAIL_ON_INVALID_POINTER(samacct_name);
    BAIL_ON_INVALID_POINTER(base);

    dwError = LwWc16sToMbs(base, &basedn);
    BAIL_ON_LSA_ERROR(dwError);

    samacctname_len = wc16slen(samacct_name);

    filter_len = samacctname_len + (sizeof(filter_fmt)/sizeof(filter_fmt[0]));

    dwError = LwAllocateMemory(sizeof(wchar16_t) * filter_len,
                               OUT_PPVOID(&filterw16));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(filterw16, filter_len, filter_fmt, samacct_name) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(filterw16, &filter);
    BAIL_ON_LSA_ERROR(dwError);

    lderr = ldap_search_ext_s(ld, basedn, LDAP_SCOPE_SUBTREE, filter, NULL, 0,
                              sctrl, cctrl, NULL, 0, &res);
    BAIL_ON_LDAP_ERROR(lderr);

cleanup:
    LW_SAFE_FREE_MEMORY(filter);
    LW_SAFE_FREE_MEMORY(filterw16);
    LW_SAFE_FREE_MEMORY(basedn);

    *out = res;

    return lderr;

error:
    *out = NULL;
    goto cleanup;
}


int LdapMachAcctMove(LDAP *ld, const wchar16_t *dn, const wchar16_t *name,
                     const wchar16_t *newparent)
{
    const char *cn_fmt = "cn=%s";

    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    size_t newname_len;
    char *dname = NULL;
    char *machname = NULL;
    char *newname = NULL;
    char *newpar = NULL;
    LDAPControl **sctrl = NULL, **cctrl = NULL;

    dwError = LwWc16sToMbs(dn, &dname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(name, &machname);
    BAIL_ON_LSA_ERROR(dwError);

    newname_len = wc16slen(name) + strlen(cn_fmt);
    dwError = LwAllocateMemory(sizeof(char) * newname_len,
                               OUT_PPVOID(&newname));
    BAIL_ON_LSA_ERROR(dwError);

    if (snprintf(newname, newname_len, cn_fmt, machname) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(newparent, &newpar);
    BAIL_ON_LSA_ERROR(dwError);

    lderr = ldap_rename_s(ld, dname, newname, newpar, 1, sctrl, cctrl);
    BAIL_ON_LDAP_ERROR(lderr);

error:
    LW_SAFE_FREE_MEMORY(newpar);
    LW_SAFE_FREE_MEMORY(newname);
    LW_SAFE_FREE_MEMORY(machname);
    LW_SAFE_FREE_MEMORY(dname);

    return lderr;
}


int LdapMachAcctSetAttribute(LDAP *ld, const wchar16_t *dn,
                             const wchar16_t *name, const wchar16_t **value,
                             int new)
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMod *mod = NULL;
    LDAPMod *mods[2];
    LDAPControl **sctrl = NULL;
    LDAPControl **cctrl = NULL;
    char *dname = NULL;
    char *n = NULL;
    int i = 0;

    dwError = LwWc16sToMbs(dn, &dname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(name, &n);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; value[i] != NULL; i++) {
        if (new) {
            lderr = LdapModAddStrValue(&mod, n, value[i]);
        } else {
            lderr = LdapModReplStrValue(&mod, n, value[i]);
        }

        BAIL_ON_LDAP_ERROR(lderr);
    }

    mods[0] = mod;
    mods[1] = NULL;

    lderr = ldap_modify_ext_s(ld, dname, mods, sctrl, cctrl);
    BAIL_ON_LDAP_ERROR(lderr);

cleanup:
    LW_SAFE_FREE_MEMORY(n);
    LW_SAFE_FREE_MEMORY(dname);

    if (mod)
    {
        LdapModFree(&mod);
    }

    return lderr;

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
