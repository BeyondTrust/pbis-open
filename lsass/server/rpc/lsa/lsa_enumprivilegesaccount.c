/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2011
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
