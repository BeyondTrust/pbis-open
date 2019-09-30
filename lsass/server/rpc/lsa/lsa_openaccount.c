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
