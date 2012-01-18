/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
