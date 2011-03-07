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
 *        lsa_enumaccountrights.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaEnumAccountRights function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvEnumAccountRights(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ PSID pAccountSid,
    /* [out] */ PLSA_ACCOUNT_RIGHTS pAccountRights
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    PPOLICY_CONTEXT pPolicyContext = (PPOLICY_CONTEXT)hPolicy;
    PWSTR *ppwszAccountRights = NULL;
    DWORD numAccountRights = 0;
    DWORD i = 0;

    if (pPolicyContext == NULL || pPolicyContext->Type != LsaContextPolicy)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    err = LsaSrvPrivsEnumAccountRights(
            NULL,
            pPolicyContext->pUserToken,
            pAccountSid,
            &ppwszAccountRights,
            &numAccountRights);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvAllocateMemory(
                    OUT_PPVOID(&pAccountRights->pAccountRight),
                    sizeof(pAccountRights->pAccountRight[0]) * numAccountRights);
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < numAccountRights; i++)
    {
        err = LsaSrvInitUnicodeString(
                            &pAccountRights->pAccountRight[i],
                            ppwszAccountRights[i]);
        BAIL_ON_LSA_ERROR(err);
    }

    pAccountRights->NumAccountRights = numAccountRights;

error:
    if (err || ntStatus)
    {
        for (i = 0; i < numAccountRights; i++)
        {
            LsaSrvFreeUnicodeString(&pAccountRights->pAccountRight[i]);
        }

        pAccountRights->pAccountRight    = NULL;
        pAccountRights->NumAccountRights = 0;
    }

    for (i = 0; i < numAccountRights; i++)
    {
        LW_SAFE_FREE_MEMORY(ppwszAccountRights[i]);
    }
    LW_SAFE_FREE_MEMORY(ppwszAccountRights);

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
