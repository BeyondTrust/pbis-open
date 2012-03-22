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
 *        lsa.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa rpc server stub functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS srv_LsaClose(
    /* [in] */ handle_t IDL_handle,
    /* [in,out] */ POLICY_HANDLE *hInOut
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvClose(IDL_handle,
                         hInOut);

    return status;
}


NTSTATUS srv_lsa_Function01(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaEnumPrivileges(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in, out] */ UINT32 *pResume,
    /* [in] */ UINT32 PreferredMaxSize,
    /* [out] */ LSA_PRIVILEGE_ENUM_BUFFER *pBuffer
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvEnumPrivileges(IDL_handle,
                                  hPolicy,
                                  pResume,
                                  PreferredMaxSize,
                                  pBuffer);
    return status;
}


NTSTATUS srv_LsaQuerySecurity(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ idl_void_p_t hObject,
    /* [in] */ UINT32 SecurityInformation,
    /* [out] */ PLSA_SECURITY_DESCRIPTOR_BUFFER *ppSecurityDesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvQuerySecurity(IDL_handle,
                                 hObject,
                                 SecurityInformation,
                                 ppSecurityDesc);
    return status;
}


NTSTATUS srv_LsaSetSecurity(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ idl_void_p_t hObject,
    /* [in] */ UINT32 SecurityInformation,
    /* [in] */ PLSA_SECURITY_DESCRIPTOR_BUFFER pSecurityDesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvSetSecurity(IDL_handle,
                               hObject,
                               SecurityInformation,
                               pSecurityDesc);
    return status;
}


NTSTATUS srv_lsa_Function05(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function06(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaQueryInfoPolicy(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UINT16 level,
    /* [out] */ LsaPolicyInformation **info
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvQueryInfoPolicy(IDL_handle,
                                   hPolicy,
                                   level,
                                   info);
    return status;
}


NTSTATUS srv_lsa_Function08(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function09(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaCreateAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ SID *pAccountSid,
    /* [in] */ UINT32 AccessMask,
    /* [out] */ LSAR_ACCOUNT_HANDLE *phAccount
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvCreateAccount(IDL_handle,
                                 hPolicy,
                                 pAccountSid,
                                 AccessMask,
                                 phAccount);
    return status;
}


NTSTATUS srv_LsaEnumAccounts(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in, out] */ UINT32 *Resume,
    /* [out] */ LSA_ACCOUNT_ENUM_BUFFER *pAccounts,
    /* [in] */ UINT32 PrefMaxSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvEnumAccounts(IDL_handle,
                                hPolicy,
                                Resume,
                                pAccounts,
                                PrefMaxSize);
    return status;
}


NTSTATUS srv_lsa_Function0c(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function0d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaLookupNames(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UINT32 num_names,
    /* [in] */ UNICODE_STRING *names,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedSidArray *sids,
    /* [in] */ UINT16 level,
    /* [in, out] */ UINT32 *count
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupNames(IDL_handle,
                               hPolicy,
                               num_names,
                               names,
                               domains,
                               sids,
                               level,
                               count);
    return status;
}


NTSTATUS srv_LsaLookupSids(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ SID_ARRAY *sids,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedNameArray *names,
    /* [in] */ UINT16 level,
    /* [in, out] */ UINT32 *count
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupSids(IDL_handle,
                              hPolicy,
                              sids,
                              domains,
                              names,
                              level,
                              count);
    return status;
}


NTSTATUS srv_lsa_Function10(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaOpenAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ SID *pAccountSid,
    /* [in] */ UINT32 AccessMask,
    /* [out] */ LSAR_ACCOUNT_HANDLE *phAccount
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvOpenAccount(IDL_handle,
                               hPolicy,
                               pAccountSid,
                               AccessMask,
                               phAccount);
    return status;
}


NTSTATUS srv_LsaEnumPrivilegesAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LSAR_ACCOUNT_HANDLE hAccount,
    /* [out] */ PRIVILEGE_SET **Privileges
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvEnumPrivilegesAccount(IDL_handle,
                                         hAccount,
                                         Privileges);
    return status;
}


NTSTATUS srv_LsaAddPrivilegesToAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LSAR_ACCOUNT_HANDLE hAccount,
    /* [in] */ PRIVILEGE_SET *pPrivileges
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvAddPrivilegesToAccount(IDL_handle,
                                          hAccount,
                                          pPrivileges);
    return status;
}


NTSTATUS srv_LsaRemovePrivilegesFromAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LSAR_ACCOUNT_HANDLE hAccount,
    /* [in] */ UINT8 AllPrivileges,
    /* [in] */ PRIVILEGE_SET *pPrivileges
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvRemovePrivilegesFromAccount(IDL_handle,
                                               hAccount,
                                               AllPrivileges,
                                               pPrivileges);
    return status;
}


NTSTATUS srv_lsa_Function15(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function16(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaGetSystemAccessAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LSAR_ACCOUNT_HANDLE hAccount,
    /* [out] */ UINT32 *SystemAccess
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvGetSystemAccessAccount(IDL_handle,
                                          hAccount,
                                          SystemAccess);
    return status;
}


NTSTATUS srv_LsaSetSystemAccessAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LSAR_ACCOUNT_HANDLE hAccount,
    /* [in] */ UINT32 SystemAccess
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvSetSystemAccessAccount(IDL_handle,
                                          hAccount,
                                          SystemAccess);
    return status;
}


NTSTATUS srv_lsa_Function19(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function1a(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function1b(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function1c(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function1d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function1e(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaLookupPrivilegeValue(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UNICODE_STRING *pName,
    /* [out] */ LUID *pValue
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupPrivilegeValue(IDL_handle,
                                        hPolicy,
                                        pName,
                                        pValue);
    return status;
}


NTSTATUS srv_LsaLookupPrivilegeName(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ LUID *pValue,
    /* [out] */ UNICODE_STRING **ppName
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupPrivilegeName(IDL_handle,
                                       hPolicy,
                                       pValue,
                                       ppName);
    return status;
}


NTSTATUS srv_LsaLookupPrivilegeDisplayName(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UNICODE_STRING *pName,
    /* [in] */ INT16 ClientLanguage,
    /* [in] */ INT16 ClientSystemLanguage,
    /* [out] */ UNICODE_STRING **ppDisplayName,
    /* [out] */ UINT16 *pLanguage
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupPrivilegeDisplayName(IDL_handle,
                                              hPolicy,
                                              pName,
                                              ClientLanguage,
                                              ClientSystemLanguage,
                                              ppDisplayName,
                                              pLanguage);
    return status;
}


NTSTATUS srv_LsaDeleteObject(
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ idl_void_p_t *phObject
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaRpcSrvDeleteObject(IDL_handle,
                                   phObject);
    return status;
}


NTSTATUS srv_LsaEnumAccountsWithUserRight(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UNICODE_STRING *pName,
    /* [out] */ LSA_ACCOUNT_ENUM_BUFFER *pAccounts
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvEnumAccountsWithUserRight(IDL_handle,
                                             hPolicy,
                                             pName,
                                             pAccounts);
    return status;
}


NTSTATUS srv_LsaEnumAccountRights(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ SID *pAccountSid,
    /* [out] */ LSA_ACCOUNT_RIGHTS *pAccountRights
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvEnumAccountRights(IDL_handle,
                                     hPolicy,
                                     pAccountSid,
                                     pAccountRights);
    return status;
}


NTSTATUS srv_LsaAddAccountRights(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ SID *pAccountSid,
    /* [in] */ LSA_ACCOUNT_RIGHTS *pAccountRights
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvAddAccountRights(IDL_handle,
                                    hPolicy,
                                    pAccountSid,
                                    pAccountRights);
    return status;
}


NTSTATUS srv_LsaRemoveAccountRights(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ SID *pAccountSid,
    /* [in] */ UINT8 RemoveAll,
    /* [in] */ LSA_ACCOUNT_RIGHTS *pAccountRights
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvRemoveAccountRights(IDL_handle,
                                       hPolicy,
                                       pAccountSid,
                                       RemoveAll,
                                       pAccountRights);
    return status;
}


NTSTATUS srv_lsa_Function27(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function28(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function29(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function2a(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function2b(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaOpenPolicy2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ ObjectAttribute *attrib,
    /* [in] */ UINT32 access_mask,
    /* [out] */ POLICY_HANDLE *hPolicy
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvOpenPolicy2(IDL_handle,
                               system_name,
                               attrib,
                               access_mask,
                               hPolicy);
    return status;
}


NTSTATUS srv_lsa_Function2d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}


NTSTATUS srv_LsaQueryInfoPolicy2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UINT16 level,
    /* [out] */ LsaPolicyInformation **info
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvQueryInfoPolicy2(IDL_handle,
                                    hPolicy,
                                    level,
                                    info);
    return status;
}


NTSTATUS srv_lsa_Function2f(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function30(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function31(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function32(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function33(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function34(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function35(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function36(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function37(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function38(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaLookupSids2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ SID_ARRAY *sids,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedNameArray2 *names,
    /* [in] */ UINT16 level,
    /* [in, out] */ UINT32 *count,
    /* [in] */ UINT32 unknown1,
    /* [in] */ UINT32 unknown2
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupSids2(IDL_handle,
                               hPolicy,
                               sids,
                               domains,
                               names,
                               level,
                               count,
                               unknown1,
                               unknown2);
    return status;
}


NTSTATUS srv_LsaLookupNames2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UINT32 num_names,
    /* [in] */ UNICODE_STRING *names,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedSidArray2 *sids,
    /* [in] */ UINT16 level,
    /* [in, out] */ UINT32 *count,
    /* [in] */ UINT32 unknown1,
    /* [in] */ UINT32 unknown2
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupNames2(IDL_handle,
                                hPolicy,
                                num_names,
                                names,
                                domains,
                                sids,
                                level,
                                count,
                                unknown1,
                                unknown2);
    return status;
}


NTSTATUS srv_lsa_Function3b(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function3c(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function3d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function3e(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function3f(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function40(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function41(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function42(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_lsa_Function43(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_LsaLookupNames3(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ UINT32 num_names,
    /* [in] */ UNICODE_STRING *names,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedSidArray3 *sids,
    /* [in] */ UINT16 level,
    /* [in, out] */ UINT32 *count,
    /* [in] */ UINT32 unknown1,
    /* [in] */ UINT32 unknown2
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvLookupNames3(IDL_handle,
                                hPolicy,
                                num_names,
                                names,
                                domains,
                                sids,
                                level,
                                count,
                                unknown1,
                                unknown2);
    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
