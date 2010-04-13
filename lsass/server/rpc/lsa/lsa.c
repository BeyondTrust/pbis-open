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
 * Abstract: Lsa wrapper functions called from DCE/RPC stubs (rpc server library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS __LsaClose(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hIn,
    /* [out] */ POLICY_HANDLE *hOut
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaSrvClose(IDL_handle,
                         hIn,
                         hOut);

    return status;
}


NTSTATUS _lsa_Function01(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function02(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function03(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function04(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function05(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function06(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS __LsaQueryInfoPolicy(
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


NTSTATUS _lsa_Function08(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function09(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function0a(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function0b(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function0c(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function0d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS __LsaLookupNames(
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


NTSTATUS __LsaLookupSids(
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


NTSTATUS _lsa_Function10(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function11(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function12(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function13(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function14(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function15(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function16(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function17(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function18(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function19(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function1a(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function1b(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function1c(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function1d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function1e(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function1f(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function20(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function21(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function22(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function23(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function24(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function25(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function26(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function27(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function28(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function29(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function2a(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function2b(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS __LsaOpenPolicy2(
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


NTSTATUS _lsa_Function2d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}


NTSTATUS __LsaQueryInfoPolicy2(
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


NTSTATUS _lsa_Function2f(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function30(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function31(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function32(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function33(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function34(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function35(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function36(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function37(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function38(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS __LsaLookupSids2(
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


NTSTATUS __LsaLookupNames2(
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


NTSTATUS _lsa_Function3b(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function3c(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function3d(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function3e(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function3f(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function40(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function41(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function42(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _lsa_Function43(
    /* [in] */ handle_t IDL_handle
)
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS __LsaLookupNames3(
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
