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
 *        lsa_enumaccounts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaEnumAccounts function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvEnumAccounts(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in, out] */ PDWORD pResume,
    /* [out] */ PLSA_ACCOUNT_ENUM_BUFFER pBuffer,
    /* [in] */ DWORD PreferredMaxSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    DWORD enumerationStatus = ERROR_SUCCESS;
    PPOLICY_CONTEXT pPolicyContext = (PPOLICY_CONTEXT)hPolicy;
    DWORD resume = *pResume;
    PSID *pAccountSids = NULL;
    DWORD numAccounts = 0;
    DWORD i = 0;

    if (pPolicyContext == NULL || pPolicyContext->Type != LsaContextPolicy)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    err = LsaSrvPrivsEnumAccounts(
                        NULL,
                        pPolicyContext->pUserToken,
                        &resume,
                        PreferredMaxSize,
                        &pAccountSids,
                        &numAccounts);
    if (err == ERROR_MORE_DATA ||
        err == ERROR_NO_MORE_ITEMS)
    {
        enumerationStatus = err;
        err = ERROR_SUCCESS;
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    ntStatus = LsaSrvAllocateMemory(
                        OUT_PPVOID(&pBuffer->pAccount),
                        sizeof(pBuffer->pAccount[0]) * numAccounts);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < numAccounts; i++)
    {
        ntStatus = LsaSrvDuplicateSid(
                            &pBuffer->pAccount[i].pSid,
                            pAccountSids[i]);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer->NumAccounts = numAccounts;
    *pResume = resume;

error:
    if (err || ntStatus)
    {
        if (pBuffer->pAccount)
        {
            for (i = 0; i < numAccounts; i++)
            {
                LsaSrvFreeMemory(pBuffer->pAccount[i].pSid);
            }
            LsaSrvFreeMemory(pBuffer->pAccount);
        }

        pBuffer->pAccount    = NULL;
        pBuffer->NumAccounts = 0;
        *pResume = 0;
    }

    for (i = 0; i < numAccounts; i++)
    {
        LW_SAFE_FREE_MEMORY(pAccountSids[i]);
    }
    LW_SAFE_FREE_MEMORY(pAccountSids);

    if (err == ERROR_SUCCESS &&
        enumerationStatus != ERROR_SUCCESS)
    {
        switch (enumerationStatus)
        {
        case ERROR_MORE_DATA:
            ntStatus = STATUS_MORE_ENTRIES;
            break;

        case ERROR_NO_MORE_ITEMS:
            ntStatus = STATUS_NO_MORE_ENTRIES;
            break;

        default:
            err = enumerationStatus;
            break;
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
