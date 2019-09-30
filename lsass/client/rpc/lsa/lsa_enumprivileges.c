/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lsa_enumprivileges.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaEnumPrivileges function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaEnumPrivileges(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN OUT PDWORD      pResume,
    IN DWORD           PreferredMaxSize,
    OUT PWSTR        **ppNames,
    OUT PLUID         *ppValues,
    OUT PDWORD         pNumPrivileges
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntEnumStatus = STATUS_SUCCESS;
    LSA_PRIVILEGE_ENUM_BUFFER privileges = {0};
    DWORD offset = 0;
    DWORD spaceLeft = 0;
    DWORD size = 0;
    PWSTR *pNames = NULL;
    PLUID pValues = NULL;
    DWORD i = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(pResume, ntStatus);
    BAIL_ON_INVALID_PTR(ppNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppValues, ntStatus);
    BAIL_ON_INVALID_PTR(pNumPrivileges, ntStatus);

    DCERPC_CALL(ntStatus, cli_LsaEnumPrivileges(
                              (handle_t)hBinding,
                              hPolicy,
                              pResume,
                              PreferredMaxSize,
                              &privileges));
    if (ntStatus == STATUS_MORE_ENTRIES ||
        ntStatus == STATUS_NO_MORE_ENTRIES)
    {
        ntEnumStatus = ntStatus;
        ntStatus = STATUS_SUCCESS;
    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    offset    = 0;
    spaceLeft = 0;
    size      = 0;

    ntStatus = LsaAllocatePrivilegeNames(
                          pNames,
                          &offset,
                          &spaceLeft,
                          &privileges,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    offset    = 0;
    spaceLeft = size;
    size      = 0;

    ntStatus = LsaRpcAllocateMemory(
                          OUT_PPVOID(&pNames),
                          spaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaAllocatePrivilegeNames(
                          pNames,
                          &offset,
                          &spaceLeft,
                          &privileges,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                          OUT_PPVOID(&pValues),
                          sizeof(pValues[0]) * privileges.NumPrivileges);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < privileges.NumPrivileges; i++)
    {
        pValues[i] = privileges.pPrivilege[i].Value;
    }

    *ppNames        = pNames;
    *ppValues       = pValues;
    *pNumPrivileges = privileges.NumPrivileges;

error:
    if (ntStatus)
    {
        if (pNames)
        {
            LsaRpcFreeMemory(pNames);
        }

        if (ppNames)
        {
            *ppNames = NULL;
        }

        if (ppValues)
        {
            *ppValues = NULL;
        }

        if (pNumPrivileges)
        {
            *pNumPrivileges = 0;
        }
    }

    LsaCleanStubPrivilegeBuffer(&privileges);

    if (ntStatus == STATUS_SUCCESS)
    {
        ntStatus = ntEnumStatus;
    }

    return ntStatus;
}
