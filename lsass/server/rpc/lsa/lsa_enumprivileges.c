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
 *        lsa_enumprivileges.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaEnumPrivileges function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvEnumPrivileges(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in, out] */ PDWORD pResume,
    /* [in] */ DWORD PreferredMaxSize,
    /* [out] */ PLSA_PRIVILEGE_ENUM_BUFFER pBuffer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    DWORD enumerationStatus = ERROR_SUCCESS;
    PPOLICY_CONTEXT pPolicyContext = (PPOLICY_CONTEXT)hPolicy;
    DWORD resume = *pResume;
    PWSTR *pPrivilegeNames = NULL;
    PLUID pPrivilegeValues = NULL;
    DWORD numPrivileges = 0;
    DWORD i = 0;

    err = LsaSrvPrivsEnumPrivileges(
                        NULL,
                        pPolicyContext->pUserToken,
                        &resume,
                        PreferredMaxSize,
                        &pPrivilegeNames,
                        &pPrivilegeValues,
                        &numPrivileges);
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
                        OUT_PPVOID(&pBuffer->pPrivilege),
                        sizeof(pBuffer->pPrivilege[0]) * numPrivileges);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < numPrivileges; i++)
    {
        pBuffer->pPrivilege[i].Value = pPrivilegeValues[i];

        ntStatus = LsaSrvInitUnicodeString(
                            &pBuffer->pPrivilege[i].Name,
                            pPrivilegeNames[i]);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer->NumPrivileges = numPrivileges;
    *pResume = resume;

error:
    if (err || ntStatus)
    {
        if (pBuffer->pPrivilege)
        {
            for (i = 0; i < numPrivileges; i++)
            {
                LsaSrvFreeUnicodeString(&pBuffer->pPrivilege[i].Name);
            }
            LsaSrvFreeMemory(pBuffer->pPrivilege);
        }

        pBuffer->pPrivilege    = NULL;
        pBuffer->NumPrivileges = 0;
        *pResume = 0;
    }

    for (i = 0; i < numPrivileges; i++)
    {
        LW_SAFE_FREE_MEMORY(pPrivilegeNames[i]);
    }
    LW_SAFE_FREE_MEMORY(pPrivilegeNames);
    LW_SAFE_FREE_MEMORY(pPrivilegeValues);

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
