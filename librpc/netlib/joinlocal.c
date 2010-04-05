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


NTSTATUS
ResetAccountPasswordTimer(
    handle_t samr_b,
    ACCOUNT_HANDLE hAccount,
    UINT32 account_flags
    )
{
    UINT32 flags_enable  = account_flags & (~ACB_DISABLED);
    UINT32 flags_disable = account_flags | ACB_DISABLED;
    const UINT32 level = 16;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo info;
    UserInfo16 *info16 = NULL;

    memset((void*)&info, 0, sizeof(info));
    info16 = &info.info16;

    BAIL_ON_INVALID_PTR(samr_b, err);
    BAIL_ON_INVALID_PTR(hAccount, err);

    /* flip ACB_DISABLED flag - this way password timeout counter
       gets restarted */

    info16->account_flags = flags_enable;
    status = SamrSetUserInfo(samr_b, hAccount, level, &info);
    BAIL_ON_NT_STATUS(status);

    info16->account_flags = flags_disable;
    status = SamrSetUserInfo(samr_b, hAccount, level, &info);
    BAIL_ON_NT_STATUS(status);

    info16->account_flags = flags_enable;
    status = SamrSetUserInfo(samr_b, hAccount, level, &info);
    BAIL_ON_NT_STATUS(status);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
ResetWksAccount(
    PNET_CONN       pConn,
    wchar16_t      *name,
    ACCOUNT_HANDLE  hAccount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    UserInfo *info = NULL;

    BAIL_ON_INVALID_PTR(pConn, err);
    BAIL_ON_INVALID_PTR(name, err);
    BAIL_ON_INVALID_PTR(hAccount, err);

    samr_b = pConn->Rpc.Samr.hBinding;

    status = SamrQueryUserInfo(samr_b, hAccount, 16, &info);
    if (status == STATUS_SUCCESS &&
        !(info->info16.account_flags & ACB_WSTRUST)) {
        status = STATUS_INVALID_ACCOUNT_NAME;
        goto error;

    } else if (status != STATUS_SUCCESS) {
        goto error;
    }

    status = ResetAccountPasswordTimer(samr_b, hAccount,
                                       info->info16.account_flags);
    BAIL_ON_NT_STATUS(status);

cleanup:
    if (info) {
        SamrFreeMemory((void*)info);
    }

    return status;

error:
    goto cleanup;
}


NTSTATUS
CreateWksAccount(
    PNET_CONN       pConn,
    wchar16_t      *samacct_name,
    ACCOUNT_HANDLE *phAccount
    )
{
    const UINT32 user_access = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD |
                               USER_ACCESS_SET_ATTRIBUTES;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ret = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    UINT32 access_granted = 0;
    UINT32 rid = 0;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAccount = NULL;
    PwInfo pwinfo;
    UserInfo *info = NULL;

    memset((void*)&pwinfo, 0, sizeof(pwinfo));

    BAIL_ON_INVALID_PTR(pConn, err);
    BAIL_ON_INVALID_PTR(samacct_name, err);
    BAIL_ON_INVALID_PTR(phAccount, err);

    samr_b  = pConn->Rpc.Samr.hBinding;
    hDomain = pConn->Rpc.Samr.hDomain;

    status = SamrCreateUser2(samr_b, hDomain, samacct_name, ACB_WSTRUST,
                             user_access, &hAccount, &access_granted, &rid);
    BAIL_ON_NT_STATUS(status);

    status = SamrQueryUserInfo(samr_b, hAccount, 16, &info);
    if (status == STATUS_SUCCESS &&
        !(info->info16.account_flags & ACB_WSTRUST)) {
        status = STATUS_INVALID_ACCOUNT_NAME;
    }

    BAIL_ON_NT_STATUS(status);

    /* It's not certain yet what is this call for here.
       Access denied is not fatal here, so we don't want to report
       a fatal error */
    ret = SamrGetUserPwInfo(samr_b, hAccount, &pwinfo);
    if (ret != STATUS_SUCCESS &&
        ret != STATUS_ACCESS_DENIED) {
        status = ret;
    }

    *phAccount = hAccount;

cleanup:
    if (info) {
        SamrFreeMemory((void*)info);
    }

    return status;

error:
    *phAccount = NULL;

    goto cleanup;
}


NTSTATUS
SetMachinePassword(
    PNET_CONN       pConn,
    ACCOUNT_HANDLE  hAccount,
    UINT32          new,
    wchar16_t      *name,
    wchar16_t      *password
    )
{	
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    UINT32 level = 0;
    UINT32 password_len = 0;
    UserInfo25 *info25 = NULL;
    UserInfo26 *info26 = NULL;
    UserInfo pwinfo;
    UnicodeString *full_name = NULL;

    memset((void*)&pwinfo, 0, sizeof(pwinfo));

    BAIL_ON_INVALID_PTR(pConn, err);
    BAIL_ON_INVALID_PTR(hAccount, err);
    BAIL_ON_INVALID_PTR(name, err);
    BAIL_ON_INVALID_PTR(password, err);

    samr_b       = pConn->Rpc.Samr.hBinding;
    password_len = wc16slen(password);

    if (new) {
        /* set account password */
        info25 = &pwinfo.info25;
        err = NetEncryptPasswordBufferEx(info25->password.data,
                                         sizeof(info25->password.data),
                                         password,
                                         password_len,
                                         pConn);
        BAIL_ON_WIN_ERROR(err);

        full_name = &info25->info.full_name;

        /*
         * This clears ACB_DISABLED flag and thus makes the account
         * active with password timeout reset
         */
        info25->info.account_flags = ACB_WSTRUST;
        status = InitUnicodeString(full_name, name);
        BAIL_ON_NT_STATUS(status);

        info25->info.fields_present = SAMR_FIELD_FULL_NAME |
                                      SAMR_FIELD_ACCT_FLAGS |
                                      SAMR_FIELD_PASSWORD;
        level = 25;

	} else {
        /* set account password */
        info26 = &pwinfo.info26;
        err = NetEncryptPasswordBufferEx(info26->password.data,
                                         sizeof(info26->password.data),
                                         password,
                                         password_len,
                                         pConn);
        BAIL_ON_WIN_ERROR(err);

        level = 26;
    }

    status = SamrSetUserInfo(samr_b, hAccount, level, &pwinfo);
    BAIL_ON_NT_STATUS(status);

cleanup:
    if (full_name) {
        FreeUnicodeString(full_name);
    }

    if (status == STATUS_SUCCESS &&
        err != ERROR_SUCCESS)
    {
        status = LwWin32ErrorToNtStatus(err);
    }

    return status;

error:
    goto cleanup;
}


NET_API_STATUS
DirectoryConnect(
    const wchar16_t *domain,
    LDAP **ldconn,
    wchar16_t **dn_context
    )
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    int close_lderr = LDAP_SUCCESS;
    LDAP *ld = NULL;
    LDAPMessage *info = NULL;
    LDAPMessage *res = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;

    BAIL_ON_INVALID_PTR(domain, err);
    BAIL_ON_INVALID_PTR(ldconn, err);
    BAIL_ON_INVALID_PTR(dn_context, err);

    *ldconn     = NULL;
    *dn_context = NULL;

    lderr = LdapInitConnection(&ld, domain, GSS_C_INTEG_FLAG);
    BAIL_ON_LDERR_ERROR(lderr);

    lderr = LdapGetDirectoryInfo(&info, &res, ld);
    BAIL_ON_LDERR_ERROR(lderr);

    dn_context_name = ambstowc16s("defaultNamingContext");
    goto_if_no_memory_lderr(dn_context_name, error);

    dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
    if (dn_context_val == NULL) {
        /* TODO: find more descriptive error code */
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto error;

    }

    *dn_context = wc16sdup(dn_context_val[0]);
    goto_if_no_memory_lderr((*dn_context), error);

    *ldconn = ld;

cleanup:
    SAFE_FREE(dn_context_name);

    if (dn_context_val) {
        LdapAttributeValueFree(dn_context_val);
    }

    if (res) {
        LdapMessageFree(res);
    }

    return LdapErrToWin32Error(lderr);

error:
    if (ld) {
        close_lderr = LdapCloseConnection(ld);
        if (lderr == LDAP_SUCCESS &&
            close_lderr != STATUS_SUCCESS) {
            lderr = close_lderr;
        }
    }

    *dn_context = NULL;
    *ldconn     = NULL;
    goto cleanup;
}


NET_API_STATUS
DirectoryDisconnect(
    LDAP *ldconn
    )
{
    int lderr = LdapCloseConnection(ldconn);
    return LdapErrToWin32Error(lderr);
}


NET_API_STATUS
MachDnsNameSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    const wchar16_t *dns_domain_name,
    wchar16_t **samacct)
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *samacct_attr_name = NULL;
    wchar16_t **samacct_attr_val = NULL;

    BAIL_ON_INVALID_PTR(ldconn, err);
    BAIL_ON_INVALID_PTR(name, err);
    BAIL_ON_INVALID_PTR(dn_context, err);
    BAIL_ON_INVALID_PTR(dns_domain_name, err);
    BAIL_ON_INVALID_PTR(samacct, err);

    *samacct = NULL;

    lderr = LdapMachDnsNameSearch(
                &res,
                ldconn,
                name,
                dns_domain_name,
                dn_context);
    BAIL_ON_LDERR_ERROR(lderr);

    samacct_attr_name = ambstowc16s("sAMAccountName");
    goto_if_no_memory_lderr(samacct_attr_name, error);

    samacct_attr_val = LdapAttributeGet(ldconn, res, samacct_attr_name, NULL);
    if (!samacct_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto error;
    }

    *samacct = wc16sdup(samacct_attr_val[0]);
    goto_if_no_memory_lderr((*samacct), error);

cleanup:

    SAFE_FREE(samacct_attr_name);
    LdapAttributeValueFree(samacct_attr_val);

    if (res)
    {
        LdapMessageFree(res);
    }

    return LdapErrToWin32Error(lderr);

error:

    *samacct = NULL;
    goto cleanup;
}


NET_API_STATUS
MachAcctSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    wchar16_t **dn
    )
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *dn_attr_name = NULL;
    wchar16_t **dn_attr_val = NULL;

    BAIL_ON_INVALID_PTR(ldconn, err);
    BAIL_ON_INVALID_PTR(name, err);
    BAIL_ON_INVALID_PTR(dn_context, err);
    BAIL_ON_INVALID_PTR(dn, err);

    *dn = NULL;

    lderr = LdapMachAcctSearch(&res, ldconn, name, dn_context);
    BAIL_ON_LDERR_ERROR(lderr);

    dn_attr_name = ambstowc16s("distinguishedName");
    goto_if_no_memory_lderr(dn_attr_name, error);

    dn_attr_val = LdapAttributeGet(ldconn, res, dn_attr_name, NULL);
    if (!dn_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto error;
    }
    
    *dn = wc16sdup(dn_attr_val[0]);
    goto_if_no_memory_lderr((*dn), error);

cleanup:
    SAFE_FREE(dn_attr_name);
    LdapAttributeValueFree(dn_attr_val);

    if (res) {
        LdapMessageFree(res);
    }

    return LdapErrToWin32Error(lderr);

error:
    *dn = NULL;
    goto cleanup;
}


NET_API_STATUS
MachAcctCreate(
    LDAP *ld,
    const wchar16_t *machine_name,
    const wchar16_t *machacct_name,
    const wchar16_t *ou,
    int rejoin
    )
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *machacct = NULL;
    LDAPMessage *res = NULL;
    LDAPMessage *info = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;
    wchar16_t *dn_name = NULL;
    wchar16_t **dn_val = NULL;

    BAIL_ON_INVALID_PTR(ld, err);
    BAIL_ON_INVALID_PTR(machine_name, err);
    BAIL_ON_INVALID_PTR(machacct_name, err);
    BAIL_ON_INVALID_PTR(ou, err);

    lderr = LdapMachAcctCreate(ld, machine_name, machacct_name, ou);
    if (lderr == LDAP_ALREADY_EXISTS && rejoin) {
        lderr = LdapGetDirectoryInfo(&info, &res, ld);
        BAIL_ON_LDERR_ERROR(lderr);

        dn_context_name = ambstowc16s("defaultNamingContext");
        if (!dn_context_name) {
            lderr = ENOMEM;
            goto error;
        }

        dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
        if (dn_context_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto error;
        }

        lderr = LdapMachAcctSearch(&machacct, ld, machacct_name,
                                   dn_context_val[0]);
        BAIL_ON_LDERR_ERROR(lderr);

        dn_name = ambstowc16s("distinguishedName");
        if (!dn_name) {
            lderr = ENOMEM;
            goto error;
        }

        dn_val = LdapAttributeGet(ld, machacct, dn_name, NULL);
        if (dn_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto error;
        }

        lderr = LdapMachAcctMove(ld, dn_val[0], machine_name, ou);
        BAIL_ON_LDERR_ERROR(lderr);
    }

cleanup:
    SAFE_FREE(dn_context_name);
    SAFE_FREE(dn_name);

    if (dn_context_val) {
        LdapAttributeValueFree(dn_context_val);
    }

    if (dn_val) {
        LdapAttributeValueFree(dn_val);
    }

    return LdapErrToWin32Error(lderr);

error:
    goto cleanup;
}


NET_API_STATUS
MachAcctSetAttribute(
    LDAP *ldconn,
    const wchar16_t *dn,
    const wchar16_t *attr_name,
    const wchar16_t **attr_val,
    int new
    )
{
    int lderr = LDAP_SUCCESS;

    lderr = LdapMachAcctSetAttribute(ldconn, dn, attr_name, attr_val, new);
    return LdapErrToWin32Error(lderr);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
