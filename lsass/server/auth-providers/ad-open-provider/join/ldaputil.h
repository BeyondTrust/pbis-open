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

#ifndef _LDAP_UTIL_H_
#define _LDAP_UTIL_H_


#define BAIL_ON_LDAP_ERROR(e)                \
    if ((e) != LDAP_SUCCESS)                 \
    {                                        \
        LSA_LOG_DEBUG("Ldap error code: %u ", (e)); \
        goto error;                 \
    }

int
LdapMessageFree(
    LDAPMessage *msg
    );

int
LdapInitConnection(
    LDAP **ldconn,
    const wchar16_t *host,
    BOOLEAN bSeal
    );

int
LdapCloseConnection(
    LDAP *ldconn
    );

int
LdapGetDirectoryInfo(
    LDAPMessage **info,
    LDAPMessage **result,
    LDAP *ld
    );

wchar16_t**
LdapAttributeGet(
    LDAP *ld,
    LDAPMessage *info,
    const wchar16_t *name,
    int *count
    );

void
LdapAttributeValueFree(
    wchar16_t *val[]
    );

wchar16_t*
LdapAttrValDnsHostName(
    const wchar16_t *name,
    const wchar16_t *dnsdomain
    );

wchar16_t*
LdapAttrValSvcPrincipalName(
    const char *type,
    const wchar16_t *name
    );

int
LdapMachAcctCreate(
    LDAP *ld,
    const wchar16_t *machacct_name,
    const wchar16_t *ou
    );

int
LdapMachDnsNameSearch(
    LDAPMessage **out,
    LDAP *ld,
    const wchar16_t *name,
    const wchar16_t *dns_domain_name,
    const wchar16_t *base,
    PCWSTR pSchemaContext
    );

int
LdapMachAcctSearch(
    LDAPMessage **out,
    LDAP *ld,
    const wchar16_t *name,
    const wchar16_t *base
    );

int
LdapMachAcctMove(
    LDAP *ld,
    const wchar16_t *dn,
    const wchar16_t *name,
    const wchar16_t *newparent
    );

int
LdapMachAcctSetAttribute(
    LDAP *ld,
    const wchar16_t *dn,
    const wchar16_t *name,
    const wchar16_t *value[],
    int new
    );

#endif /* _LDAP_UTIL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
