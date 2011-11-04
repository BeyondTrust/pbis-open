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
 *        samr_deleteaccount.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrSrvDeleteAccount function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvDeleteAccount(
    /* [in] */ handle_t hBinding,
    /* [in] */ ACCOUNT_HANDLE hAccountIn,
    /* [out] */ ACCOUNT_HANDLE *hAccountOut
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszAccountDn = NULL;

    pAcctCtx = (PACCOUNT_CONTEXT)hAccountIn;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    if (!(pAcctCtx->dwAccessGranted & DELETE))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (SamrSrvIsBuiltinAccount(pDomCtx->pDomainSid,
                                pAcctCtx->pSid))
    {
        ntStatus = STATUS_SPECIAL_ACCOUNT;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    hDirectory    = pConnCtx->hDirectory;
    pwszAccountDn = pAcctCtx->pwszDn;

    dwError = DirectoryDeleteObject(hDirectory,
                                    pwszAccountDn);
    BAIL_ON_LSA_ERROR(dwError);

    *hAccountOut = NULL;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    *hAccountOut = hAccountIn;
    goto cleanup;
}


BOOLEAN
SamrSrvIsBuiltinAccount(
    IN  PSID pDomainSid,
    IN  PSID pAccountSid
    )
{
    BOOLEAN bBuiltin = FALSE;
    DWORD dwRid = 0;
    union {
        SID BuiltinSid;
        UCHAR Buffer[SID_MAX_SIZE];
    } sidBuffer = { .Buffer = { 0 } };
    ULONG ulSidSize = sizeof(sidBuffer);

    RtlCreateWellKnownSid(WinBuiltinDomainSid,
                          NULL,
                          &sidBuffer.BuiltinSid,
                          &ulSidSize);

    if (RtlIsPrefixSid(pDomainSid,
                       pAccountSid))
    {
        /*
         * We're only interested in subauthority immediately
         * following the domain prefix
         */
        dwRid = pAccountSid->SubAuthority[pDomainSid->SubAuthorityCount];
        bBuiltin = (dwRid <= DOMAIN_USER_RID_MAX);
    }
    else if (RtlIsPrefixSid(&sidBuffer.BuiltinSid,
                            pAccountSid))
    {
        /*
         * We're only interested in subauthority immediately
         * following the domain prefix
         */
        dwRid = pAccountSid->SubAuthority[sidBuffer.BuiltinSid.SubAuthorityCount];
        bBuiltin = (dwRid <= DOMAIN_USER_RID_MAX);
    }

    return bBuiltin;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
