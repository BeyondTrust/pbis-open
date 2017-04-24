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

#define LW_GUID_COMPUTERS_CONTAINER "AA312825768811D1ADED00C04FD8D5CD"

#define LW_GUID_DELETED_OBJECTS_CONTAINER "18E2EA80684F11D2B9AA00C04F79F805"

#define LW_GUID_DOMAIN_CONTROLLERS_CONTAINER "A361B2FFFFD211D1AA4B00C04FD7D83A"

#define LW_GUID_FOREIGNSECURITYPRINCIPALS_CONTAINER "22B70C67D56E4EFB91E9300FCA3DC1AA"

#define LW_GUID_INFRASTRUCTURE_CONTAINER "2FBAC1870ADE11D297C400C04FD8D5CD"

#define LW_GUID_LOSTANDFOUND_CONTAINER "AB8153B7768811D1ADED00C04FD8D5CD"

#define LW_GUID_MICROSOFT_PROGRAM_DATA_CONTAINER "F4BE92A4C777485E878E9421D53087DB"

#define LW_GUID_NTDS_QUOTAS_CONTAINER "6227F0AF1FC2410D8E3BB10615BB5B0F"

#define LW_GUID_PROGRAM_DATA_CONTAINER "09460C08AE1E4A4EA0F64AEE7DAA1E5A"

#define LW_GUID_SYSTEMS_CONTAINER "AB1D30F3768811D1ADED00C04FD8D5CD"

#define LW_GUID_USERS_CONTAINER "A9D1CA15768811D1ADED00C04FD8D5CD"

#define LW_GUID_MANAGED_SERVICE_ACCOUNTS_CONTAINER "1EB93889E40C45DF9F0C64D23BBB6237"

int
LdapMessageFree(
    LDAPMessage *msg
    );

DWORD
LdapInitConnection(
    OUT LDAP** ldconn,
    IN PCWSTR host,
    IN BOOLEAN bSeal
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

wchar16_t*
LdapGetWellKnownObject(
    LDAP *ld, 
    const wchar16_t *dn,
    const char *wko
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
    const wchar16_t *machine_name,
    const wchar16_t *machacct_name,
    const wchar16_t *ou
    );

int
LdapMachDnsNameSearch(
    LDAPMessage **out,
    LDAP *ld,
    const wchar16_t *fqdn,
    const wchar16_t *base
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
