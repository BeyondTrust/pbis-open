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
 *        lsa_openaccount.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaOpenAccount function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvOpenAccount(
    /* [in] */ handle_t hBinding,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ PSID pAccountSid,
    /* [in] */ DWORD AccessMask,
    /* [out] */ LSAR_ACCOUNT_HANDLE *phAccount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    PPOLICY_CONTEXT pPolicyCtx = (PPOLICY_CONTEXT)hPolicy;
    PLSAR_ACCOUNT_CONTEXT pAccountCtx = NULL;

    BAIL_ON_INVALID_PTR(hPolicy);
    BAIL_ON_INVALID_PTR(pAccountSid);
    BAIL_ON_INVALID_PTR(phAccount);

    err = LwAllocateMemory(
                        sizeof(*pAccountCtx),
                        OUT_PPVOID(&pAccountCtx));
    BAIL_ON_LSA_ERROR(err);

    pAccountCtx->Type     = LsaContextAccount;
    pAccountCtx->refcount = 1;

    pAccountCtx->pPolicyCtx = pPolicyCtx;
    InterlockedIncrement(&pPolicyCtx->refcount);


    err = LsaSrvPrivsOpenAccount(
                        NULL,
                        pPolicyCtx->pUserToken,
                        pAccountSid,
                        AccessMask,
                        &pAccountCtx->pAccountContext);
    if (err == ERROR_FILE_NOT_FOUND)
    {
        ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    *phAccount = (LSAR_ACCOUNT_HANDLE)pAccountCtx;

error:
    if (err || ntStatus)
    {
        LsaSrvAccountContextFree(pAccountCtx);

        if (phAccount)
        {
            *phAccount = NULL;
        }
    }

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
