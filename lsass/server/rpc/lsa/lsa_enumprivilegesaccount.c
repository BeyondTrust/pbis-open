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
 *        lsa_enumprivilegesaccount.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaEnumPrivilegesAccount function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvEnumPrivilegesAccount(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LSAR_ACCOUNT_HANDLE hAccount,
    /* [out] */ PPRIVILEGE_SET *ppPrivileges
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    PLSAR_ACCOUNT_CONTEXT pAccountCtx = (PLSAR_ACCOUNT_CONTEXT)hAccount;
    DWORD privilegesSize = 0;
    PPRIVILEGE_SET pPrivileges = NULL;
    PPRIVILEGE_SET pRetPrivileges = NULL;

    BAIL_ON_INVALID_PTR(hAccount);
    BAIL_ON_INVALID_PTR(ppPrivileges);

    if (pAccountCtx->Type != LsaContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    err = LsaSrvPrivsEnumAccountPrivileges(
                        pAccountCtx->pAccountContext,
                        &pPrivileges);
    BAIL_ON_LSA_ERROR(err);

    privilegesSize = RtlLengthPrivilegeSet(pPrivileges);
    ntStatus = LsaSrvAllocateMemory(
                        OUT_PPVOID(&pRetPrivileges),
                        privilegesSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCopyPrivilegeSet(
                        privilegesSize,
                        pRetPrivileges,
                        pPrivileges);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppPrivileges = pRetPrivileges;
    
error:
    if (err || ntStatus)
    {
        if (pRetPrivileges)
        {
            LsaRpcFreeMemory(pRetPrivileges);
        }

        if (ppPrivileges)
        {
            *ppPrivileges = NULL;
        }
    }

    LW_SAFE_FREE_MEMORY(pPrivileges);

    if (ntStatus == STATUS_SUCCESS &&
        err != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(err);
    }

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
