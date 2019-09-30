/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
