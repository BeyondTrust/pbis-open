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
 *        lsa_enumprivilegesaccount.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaEnumPrivilegesAccount function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaEnumPrivilegesAccount(
    IN  LSA_BINDING          hBinding,
    IN  LSAR_ACCOUNT_HANDLE  hAccount,
    OUT PPRIVILEGE_SET      *ppPrivileges
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    size_t privilegesSize = 0;
    PPRIVILEGE_SET pPrivs = NULL;
    PPRIVILEGE_SET pPrivileges = NULL;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hAccount, ntStatus);
    BAIL_ON_INVALID_PTR(ppPrivileges, ntStatus);

    DCERPC_CALL(ntStatus, cli_LsaEnumPrivilegesAccount(
                              (handle_t)hBinding,
                              hAccount,
                              &pPrivs));
    BAIL_ON_NT_STATUS(ntStatus);

    privilegesSize = RtlLengthPrivilegeSet(pPrivs);

    ntStatus = LsaRpcAllocateMemory(
                        OUT_PPVOID(&pPrivileges),
                        privilegesSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCopyPrivilegeSet(
                        privilegesSize,
                        pPrivileges,
                        pPrivs);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppPrivileges = pPrivileges;

error:
    if (ntStatus)
    {
        if (ppPrivileges)
        {
            LW_SAFE_FREE_MEMORY(pPrivileges);
            *ppPrivileges = NULL;
        }
    }

    if (pPrivs)
    {
        LsaFreeStubPrivilegeSet(pPrivs);
    }

    return ntStatus;
}
