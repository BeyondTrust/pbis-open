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
 *        lsa.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa rpc server functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSASRV_H_
#define _LSASRV_H_


NTSTATUS
LsaSrvClose(
    handle_t b,
    POLICY_HANDLE *hInOut
    );


NTSTATUS
LsaSrvEnumPrivileges(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PDWORD pResume,
    DWORD PreferredMaxSize,
    PLSA_PRIVILEGE_ENUM_BUFFER pBuffer
    );


NTSTATUS
LsaSrvQuerySecurity(
    handle_t b,
    PVOID hObject,
    SECURITY_INFORMATION SecurityInformation,
    PLSA_SECURITY_DESCRIPTOR_BUFFER *ppSecurityDesc
    );


NTSTATUS
LsaSrvSetSecurity(
    handle_t b,
    PVOID hObject,
    SECURITY_INFORMATION SecurityInformation,
    PLSA_SECURITY_DESCRIPTOR_BUFFER pSecurityDesc
    );


NTSTATUS
LsaSrvOpenPolicy2(
    handle_t b,
    wchar16_t *system_name,
    ObjectAttribute *attrib,
    UINT32 access_mask,
    POLICY_HANDLE *phPolicy
    );


NTSTATUS
LsaSrvCreateAccount(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PSID pAccountSid,
    DWORD AccessMask,
    LSAR_ACCOUNT_HANDLE *phAccount
    );


NTSTATUS
LsaSrvEnumAccounts(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PDWORD pResume,
    PLSA_ACCOUNT_ENUM_BUFFER pAccounts,
    DWORD PrefMaxSize
    );


NTSTATUS
LsaSrvLookupNames(
    handle_t b,
    POLICY_HANDLE hPolicy,
    UINT32 num_names,
    UNICODE_STRING *names,
    RefDomainList **domains,
    TranslatedSidArray *sids,
    UINT16 level,
    UINT32 *count
    );


NTSTATUS
LsaSrvLookupSids(
    handle_t b,
    POLICY_HANDLE hPolicy,
    SID_ARRAY *sids,
    RefDomainList **domains,
    TranslatedNameArray *names,
    UINT16 level,
    UINT32 *count
    );


NTSTATUS
LsaSrvOpenAccount(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PSID pAccountSid,
    DWORD AccessMask,
    LSAR_ACCOUNT_HANDLE *phAccount
    );


NTSTATUS
LsaSrvEnumPrivilegesAccount(
    handle_t b,
    LSAR_ACCOUNT_HANDLE hAccount,
    PPRIVILEGE_SET *ppPrivileges
    );


NTSTATUS
LsaSrvAddPrivilegesToAccount(
    handle_t b,
    LSAR_ACCOUNT_HANDLE hAccount,
    PPRIVILEGE_SET pPrivileges
    );


NTSTATUS
LsaSrvGetSystemAccessAccount(
    handle_t b,
    LSAR_ACCOUNT_HANDLE hAccount,
    PDWORD pSystemAccess
    );


NTSTATUS
LsaSrvSetSystemAccessAccount(
    handle_t b,
    LSAR_ACCOUNT_HANDLE hAccount,
    DWORD SystemAccess
    );


NTSTATUS
LsaSrvLookupPrivilegeValue(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PUNICODE_STRING pName,
    PLUID pValue
    );


NTSTATUS
LsaSrvLookupPrivilegeName(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PLUID pValue,
    PUNICODE_STRING *ppName
    );


NTSTATUS
LsaSrvLookupPrivilegeDisplayName(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PUNICODE_STRING pName,
    INT16 ClientLang,
    INT16 ClientSystemLanguage,
    PUNICODE_STRING *ppDisplayName,
    UINT16 *pLanguage
    );


NTSTATUS
LsaRpcSrvDeleteObject(
    handle_t b,
    PVOID *phObject
    );


NTSTATUS
LsaSrvEnumAccountsWithUserRight(
    handle_t b,
    POLICY_HANDLE hPolicy,
    PUNICODE_STRING pName,
    LSA_ACCOUNT_ENUM_BUFFER *pAccounts
    );


NTSTATUS
LsaSrvEnumAccountRights(
    handle_t IDL_handle,
    POLICY_HANDLE hPolicy,
    PSID pAccountSid,
    PLSA_ACCOUNT_RIGHTS pAccountRights
    );


NTSTATUS
LsaSrvAddAccountRights(
    handle_t IDL_handle,
    POLICY_HANDLE hPolicy,
    PSID pAccountSid,
    PLSA_ACCOUNT_RIGHTS pAccountRights
    );


NTSTATUS
LsaSrvRemoveAccountRights(
    handle_t IDL_handle,
    POLICY_HANDLE hPolicy,
    PSID pAccountSid,
    BOOLEAN RemoveAll,
    PLSA_ACCOUNT_RIGHTS pAccountRights
    );


NTSTATUS
LsaSrvAddRemoveAccountRights(
    handle_t IDL_handle,
    POLICY_HANDLE hPolicy,
    PSID pAccountSid,
    BOOLEAN Add,
    BOOLEAN RemoveAll,
    PLSA_ACCOUNT_RIGHTS pAccountRights
    );


NTSTATUS
LsaSrvRemovePrivilegesFromAccount(
    handle_t b,
    LSAR_ACCOUNT_HANDLE hAccount,
    BOOLEAN AllPrivileges,
    PPRIVILEGE_SET pPrivileges
    );


NTSTATUS
LsaSrvLookupNames2(
    handle_t b,
    POLICY_HANDLE hPolicy,
    UINT32 num_names,
    UNICODE_STRING *names,
    RefDomainList **domains,
    TranslatedSidArray2 *sids,
    UINT16 level,
    UINT32 *count,
    UINT32 unknown1,
    UINT32 unknown2
    );


NTSTATUS
LsaSrvLookupSids2(
    handle_t hBinding,
    POLICY_HANDLE hPolicy,
    SID_ARRAY *sids,
    RefDomainList **domains,
    TranslatedNameArray2 *names,
    UINT16 level,
    UINT32 *count,
    UINT32 unknown1,
    UINT32 unknown2
    );


NTSTATUS
LsaSrvQueryInfoPolicy(
    handle_t b,
    POLICY_HANDLE hPolicy,
    UINT16 level,
    LsaPolicyInformation **info
    );


NTSTATUS
LsaSrvQueryInfoPolicy2(
    handle_t b,
    POLICY_HANDLE hPolicy,
    UINT16 level,
    LsaPolicyInformation **info
    );


NTSTATUS
LsaSrvLookupNames3(
    handle_t b,
    POLICY_HANDLE hPolicy,
    UINT32 num_names,
    UNICODE_STRING *names,
    RefDomainList **domains,
    TranslatedSidArray3 *sids,
    UINT16 level,
    UINT32 *count,
    UINT32 unknown1,
    UINT32 unknown2
    );


#endif /* _LSASRV_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
