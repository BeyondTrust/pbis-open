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
