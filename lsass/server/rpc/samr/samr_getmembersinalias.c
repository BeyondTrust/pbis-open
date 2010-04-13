/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        samr_getmembersinalias.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrGetMembersInAlias function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvGetMembersInAlias(
    IN  handle_t        hBinding,
    IN  ACCOUNT_HANDLE  hAlias,
    OUT SID_ARRAY      *pSids
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszAliasDn = NULL;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    PDIRECTORY_ENTRY pMemberEntries;
    DWORD dwMembersNum = 0;
    DWORD i = 0;
    PWSTR pwszMemberSid = NULL;
    PSID pMemberSid = NULL;
    SID_ARRAY Sids = {0};

    PWSTR wszAttributes[] = {
        wszAttrObjectSid,
        NULL
    };

    pAcctCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pAcctCtx->dwAccessGranted & ALIAS_ACCESS_GET_MEMBERS))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pConnCtx    = (PCONNECT_CONTEXT)pAcctCtx->pDomCtx->pConnCtx;
    hDirectory  = pConnCtx->hDirectory;
    pwszAliasDn = pAcctCtx->pwszDn;

    dwError = DirectoryGetGroupMembers(hDirectory,
                                       pwszAliasDn,
                                       wszAttributes,
                                       &pMemberEntries,
                                       &dwMembersNum);
    BAIL_ON_LSA_ERROR(dwError);

    Sids.dwNumSids = dwMembersNum;
    ntStatus = SamrSrvAllocateMemory((PVOID*)&Sids.pSids,
                                   sizeof(*Sids.pSids) * Sids.dwNumSids);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < dwMembersNum; i++)
    {
        PDIRECTORY_ENTRY pEntry = &(pMemberEntries[i]);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pEntry,
                                    wszAttrObjectSid,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszMemberSid);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = SamrSrvAllocateSidFromWC16String(
                                    &pMemberSid,
                                    pwszMemberSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        Sids.pSids[i].pSid = pMemberSid;
    }

    pSids->dwNumSids = Sids.dwNumSids;
    pSids->pSids     = Sids.pSids;

cleanup:
    if (pMemberEntries)
    {
        DirectoryFreeEntries(pMemberEntries, dwMembersNum);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    for (i = 0; i < dwMembersNum; i++)
    {
        SamrSrvFreeMemory(Sids.pSids[i].pSid);
    }
    SamrSrvFreeMemory(Sids.pSids);

    memset(pSids, 0, sizeof(*pSids));
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
