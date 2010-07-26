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
 *        samr.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr rpc server stub functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS srv_SamrConnect(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ UINT32 access_mask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvConnect(IDL_handle,
                            system_name,
                            access_mask,
                            hConn);
    return status;
}


NTSTATUS srv_SamrClose(
    /* [in] */ handle_t IDL_handle,
    /* [in,context_handle] */ void **phInOut
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvClose(IDL_handle,
                          phInOut);
    return status;
}


NTSTATUS srv_SamrSetSecurity(
    /* [in] */ handle_t IDL_handle,
    /* [in,context_handle] */ void *hObject,
    /* [in] */ UINT32 security_info,
    /* [in,ref] */ PSAMR_SECURITY_DESCRIPTOR_BUFFER secdesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvSetSecurity(IDL_handle,
                                hObject,
                                security_info,
                                secdesc);
    return status;
}


NTSTATUS srv_SamrQuerySecurity(
    /* [in] */ handle_t IDL_handle,
    /* [in,context_handle] */ void *hObject,
    /* [in] */ UINT32 security_info,
    /* [out] */ PSAMR_SECURITY_DESCRIPTOR_BUFFER *secdesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvQuerySecurity(IDL_handle,
                                  hObject,
                                  security_info,
                                  secdesc);
    return status;
}


NTSTATUS srv_samr_Function04(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrLookupDomain(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in] */ UNICODE_STRING *domain_name,
    /* [out] */ SID **sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvLookupDomain(IDL_handle,
                                 hConn,
                                 domain_name,
                                 sid);
    return status;
}


NTSTATUS srv_SamrEnumDomains(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in, out] */ UINT32 *resume,
    /* [in] */ UINT32 size,
    /* [out] */ ENTRY_ARRAY **domains,
    /* [out] */ UINT32 *num_entries
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvEnumDomains(IDL_handle,
                                hConn,
                                resume,
                                size,
                                domains,
                                num_entries);
    return status;
}


NTSTATUS srv_SamrOpenDomain(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in] */ UINT32 access_mask,
    /* [in] */ SID *sid,
    /* [out] */ DOMAIN_HANDLE *hDomain
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvOpenDomain(IDL_handle,
                               hConn,
                               access_mask,
                               sid,
                               hDomain);
    return status;
}


NTSTATUS srv_SamrQueryDomainInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT16 level,
    /* [out] */ DomainInfo **info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvQueryDomainInfo(IDL_handle,
                                    hDomain,
                                    level,
                                    info);
    return status;
}


NTSTATUS srv_samr_Function09(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrCreateDomGroup(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UNICODE_STRING *group_name,
    /* [in] */ UINT32 access_mask,
    /* [out] */ ACCOUNT_HANDLE *hGroup,
    /* [out] */ UINT32 *rid
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function0b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrCreateUser(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UNICODE_STRING *account_name,
    /* [in] */ UINT32 access_mask,
    /* [out] */ ACCOUNT_HANDLE *hUser,
    /* [out] */ UINT32 *rid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvCreateUser(IDL_handle,
                               hDomain,
                               account_name,
                               access_mask,
                               hUser,
                               rid);
    return status;
}


NTSTATUS srv_SamrEnumDomainUsers(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in, out] */ UINT32 *resume,
    /* [in] */ UINT32 account_flags,
    /* [in] */ UINT32 max_size,
    /* [out] */ RID_NAME_ARRAY **names,
    /* [out] */ UINT32 *num_entries
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvEnumDomainUsers(IDL_handle,
                                    hDomain,
                                    resume,
                                    account_flags,
                                    max_size,
                                    names,
                                    num_entries);
    return status;
}


NTSTATUS srv_SamrCreateDomAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UNICODE_STRING *alias_name,
    /* [in] */ UINT32 access_mask,
    /* [out] */ ACCOUNT_HANDLE *hAlias,
    /* [out] */ UINT32 *rid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvCreateDomAlias(IDL_handle,
                                   hDomain,
                                   alias_name,
                                   access_mask,
                                   hAlias,
                                   rid);
    return status;
}


NTSTATUS srv_SamrEnumDomainAliases(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in, out] */ UINT32 *resume,
    /* [in] */ UINT32 account_flags,
    /* [out] */ RID_NAME_ARRAY **names,
    /* [out] */ UINT32 *num_entries
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvEnumDomainAliases(IDL_handle,
                                      hDomain,
                                      resume,
                                      account_flags,
                                      names,
                                      num_entries);
    return status;
}


NTSTATUS srv_SamrGetAliasMembership(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ SID_ARRAY *sids,
    /* [out] */ IDS *rids
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvGetAliasMembership(IDL_handle,
                                       hDomain,
                                       sids,
                                       rids);
    return status;
}


NTSTATUS srv_SamrLookupNames(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT32 num_names,
    /* [in] */ UNICODE_STRING *names,
    /* [out] */ IDS *ids,
    /* [out] */ IDS *types
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvLookupNames(IDL_handle,
                                hDomain,
                                num_names,
                                names,
                                ids,
                                types);
    return status;
}


NTSTATUS srv_SamrLookupRids(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT32 num_rids,
    /* [in] */ UINT32 *rids,
    /* [out] */ UNICODE_STRING_ARRAY *names,
    /* [out] */ IDS *types
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvLookupRids(IDL_handle,
                               hDomain,
                               num_rids,
                               rids,
                               names,
                               types);
    return status;
}


NTSTATUS srv_SamrOpenGroup(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT32 access_mask,
    /* [in] */ UINT32 rid,
    /* [out] */ ACCOUNT_HANDLE *hGroup
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function14(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function15(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function16(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrDeleteDomGroup(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hGroupIn,
    /* [out] */ ACCOUNT_HANDLE *hGroupOut
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function18(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function19(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function1a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrOpenAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT32 access_mask,
    /* [in] */ UINT32 rid,
    /* [out] */ ACCOUNT_HANDLE *hAlias
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvOpenAlias(IDL_handle,
                              hDomain,
                              access_mask,
                              rid,
                              hAlias);
    return status;
}


NTSTATUS srv_SamrQueryAliasInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [in] */ UINT16 level,
    /* [out] */ AliasInfo **info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvQueryAliasInfo(IDL_handle,
                                   hAlias,
                                   level,
                                   info);
    return status;
}


NTSTATUS srv_SamrSetAliasInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [in] */ UINT16 level,
    /* [in] */ AliasInfo *info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvSetAliasInfo(IDL_handle,
                                 hAlias,
                                 level,
                                 info);
    return status;
}


NTSTATUS srv_SamrDeleteDomAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in,out] */ ACCOUNT_HANDLE *phAlias
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvDeleteDomAlias(IDL_handle,
                                   phAlias);
    return status;
}


NTSTATUS srv_SamrAddAliasMember(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [in] */ SID *sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvAddAliasMember(IDL_handle,
                                   hAlias,
                                   sid);
    return status;
}


NTSTATUS srv_SamrDeleteAliasMember(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [in] */ SID *sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvDeleteAliasMember(IDL_handle,
                                      hAlias,
                                      sid);
    return status;
}


NTSTATUS srv_SamrGetMembersInAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [out] */ SID_ARRAY *sids
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvGetMembersInAlias(IDL_handle,
                                      hAlias,
                                      sids);
    return status;
}


NTSTATUS srv_SamrOpenUser(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT32 access_mask,
    /* [in] */ UINT32 rid,
    /* [out] */ ACCOUNT_HANDLE *hUser
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvOpenUser(IDL_handle,
                             hDomain,
                             access_mask,
                             rid,
                             hUser);
    return status;
}


NTSTATUS srv_SamrDeleteUser(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE *phUser
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvDeleteUser(IDL_handle,
                               phUser);
    return status;
}


NTSTATUS srv_SamrQueryUserInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [in] */ UINT16 level,
    /* [out] */ UserInfo **info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvQueryUserInfo(IDL_handle,
                                  hUser,
                                  level,
                                  info);
    return status;
}


NTSTATUS srv_SamrSetUserInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [in] */ UINT16 level,
    /* [in] */ UserInfo *info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvSetUserInfo(IDL_handle,
                                hUser,
                                level,
                                info);
    return status;
}


NTSTATUS srv_samr_Function26(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrGetUserGroups(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [out] */ RID_WITH_ATTRIBUTE_ARRAY **rids
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvGetUserGroups(IDL_handle,
                                  hUser,
                                  rids);
    return status;
}


NTSTATUS srv_SamrQueryDisplayInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT16 level,
    /* [in] */ UINT32 start_idx,
    /* [in] */ UINT32 max_entries,
    /* [in] */ UINT32 buf_size,
    /* [out] */ UINT32 *total_size,
    /* [out] */ UINT32 *returned_size,
    /* [out] */ SamrDisplayInfo *info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvQueryDisplayInfo(IDL_handle,
                                     hDomain,
                                     level,
                                     start_idx,
                                     max_entries,
                                     buf_size,
                                     total_size,
                                     returned_size,
                                     info);
    return status;
}


NTSTATUS srv_samr_Function29(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function2a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function2b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrGetUserPwInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [out] */ PwInfo *info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvGetUserPwInfo(IDL_handle,
                                  hUser,
                                  info);
    return status;
}


NTSTATUS srv_SamrRemoveMemberFromForeignDomain(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ PSID sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvRemoveMemberFromForeignDomain(IDL_handle,
                                                  hDomain,
                                                  sid);
    return status;
}


NTSTATUS srv_samr_Function2e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function2f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function30(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function31(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrCreateUser2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UNICODE_STRING *account_name,
    /* [in] */ UINT32 account_flags,
    /* [in] */ UINT32 access_mask,
    /* [out] */ ACCOUNT_HANDLE *hUser,
    /* [out] */ UINT32 *access_granted,
    /* [out] */ UINT32 *rid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvCreateUser2(IDL_handle,
                                hDomain,
                                account_name,
                                account_flags,
                                access_mask,
                                hUser,
                                access_granted,
                                rid);
    return status;
}


NTSTATUS srv_samr_Function33(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function34(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function35(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function36(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrChangePasswordUser2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ UNICODE_STRING *server,
    /* [in] */ UNICODE_STRING *account_name,
    /* [in] */ CryptPassword *nt_password,
    /* [in] */ HashPassword *nt_verifier,
    /* [in] */ UINT8 lm_change,
    /* [in] */ CryptPassword *lm_password,
    /* [in] */ HashPassword *lm_verifier
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvChangePasswordUser2(IDL_handle,
                                        server,
                                        account_name,
                                        nt_password,
                                        nt_verifier,
                                        lm_change,
                                        lm_password,
                                        lm_verifier);
    return status;
}


NTSTATUS srv_samr_Function38(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrConnect2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ UINT32 access_mask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvConnect2(IDL_handle,
                             system_name,
                             access_mask,
                             hConn);
    return status;
}


NTSTATUS srv_SamrSetUserInfo2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [in] */ UINT16 level,
    /* [in] */ UserInfo *info
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvSetUserInfo2(IDL_handle,
                                 hUser,
                                 level,
                                 info);
    return status;
}


NTSTATUS srv_samr_Function3a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function3b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_samr_Function3c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrConnect3(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ UINT32 unknown,
    /* [in] */ UINT32 access_mask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvConnect3(IDL_handle,
                             system_name,
                             unknown,
                             access_mask,
                             hConn);
    return status;
}


NTSTATUS srv_SamrConnect4(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ UINT32 client_access,
    /* [in] */ UINT32 access_mask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvConnect4(IDL_handle,
                             system_name,
                             client_access,
                             access_mask,
                             hConn);
    return status;
}


NTSTATUS srv_samr_Function3f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS srv_SamrConnect5(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ UINT32 access_mask,
    /* [in] */ UINT32 level_in,
    /* [in] */ SamrConnectInfo *info_in,
    /* [out] */ UINT32 *level_out,
    /* [out] */ SamrConnectInfo *info_out,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvConnect5(IDL_handle,
                             system_name,
                             access_mask,
                             level_in,
                             info_in,
                             level_out,
                             info_out,
                             hConn);
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

