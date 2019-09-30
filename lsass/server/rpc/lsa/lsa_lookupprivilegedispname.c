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
 *        lsa_lookupprivilegedispname.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupPrivilegeDisplayName function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvLookupPrivilegeDisplayName(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ PUNICODE_STRING pName,
    /* [in] */ INT16 ClientLanguage,
    /* [in] */ INT16 ClientSystemLanguage,
    /* [out] */ PUNICODE_STRING *ppDisplayName,
    /* [out] */ UINT16 *pLanguage
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    PPOLICY_CONTEXT pPolicyCtx = (PPOLICY_CONTEXT)hPolicy;
    PWSTR name = NULL;
    PWSTR displayName = NULL;
    UINT16 language = 0;
    PUNICODE_STRING pDisplayName = NULL;

    BAIL_ON_INVALID_PTR(hPolicy);
    BAIL_ON_INVALID_PTR(pName);
    BAIL_ON_INVALID_PTR(ppDisplayName);
    BAIL_ON_INVALID_PTR(pLanguage);

    if (pPolicyCtx->Type != LsaContextPolicy)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    err = LwAllocateWc16StringFromUnicodeString(
                        &name,
                        pName);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvPrivsLookupPrivilegeDescription(
                        NULL,
                        pPolicyCtx->pUserToken,
                        name,
                        ClientLanguage,
                        ClientSystemLanguage,
                        &displayName,
                        &language);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaSrvAllocateMemory(
                        OUT_PPVOID(&pDisplayName),
                        sizeof(*pDisplayName));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaSrvInitUnicodeString(
                        pDisplayName,
                        displayName);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppDisplayName = pDisplayName;
    *pLanguage     = language;

error:
    if (err)
    {
        if (pDisplayName)
        {
            LsaSrvFreeUnicodeString(pDisplayName);
            LsaSrvFreeMemory(pDisplayName);
        }

        if (ppDisplayName)
        {
            *ppDisplayName = NULL;
        }

        if (pLanguage)
        {
            *pLanguage = 0;
        }
    }

    LW_SAFE_FREE_MEMORY(name);
    LW_SAFE_FREE_MEMORY(displayName);

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
