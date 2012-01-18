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
 *        lsa_enumaccountrights.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaEnumAccountRights function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaEnumAccountRights(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN PSID            pAccountSid,
    OUT PWSTR        **ppAccountRights,
    OUT PDWORD         pNumAccountRights
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_ACCOUNT_RIGHTS accountRights = {0};
    PWSTR *pAccountRights = NULL;
    DWORD offset = 0;
    DWORD spaceLeft = 0;
    DWORD size = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(pAccountSid, ntStatus);
    BAIL_ON_INVALID_PTR(ppAccountRights, ntStatus);
    BAIL_ON_INVALID_PTR(pNumAccountRights, ntStatus);

    DCERPC_CALL(ntStatus, cli_LsaEnumAccountRights(
                              (handle_t)hBinding,
                              hPolicy,
                              pAccountSid,
                              &accountRights));
    BAIL_ON_NT_STATUS(ntStatus);

    offset    = 0;
    spaceLeft = 0;
    size      = 0;

    ntStatus = LsaAllocateAccountRightNames(
                          pAccountRights,
                          &offset,
                          &spaceLeft,
                          &accountRights,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    offset    = 0;
    spaceLeft = size;
    size      = 0;

    ntStatus = LsaRpcAllocateMemory(
                          OUT_PPVOID(&pAccountRights),
                          spaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaAllocateAccountRightNames(
                          pAccountRights,
                          &offset,
                          &spaceLeft,
                          &accountRights,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAccountRights   = pAccountRights;
    *pNumAccountRights = accountRights.NumAccountRights;

error:
    if (ntStatus)
    {
        if (pAccountRights)
        {
            LsaRpcFreeMemory(pAccountRights);
        }

        if (ppAccountRights)
        {
            *ppAccountRights = NULL;
        }

        if (pNumAccountRights)
        {
            *pNumAccountRights = 0;
        }
    }

    LsaCleanStubAccountRights(&accountRights);

    return ntStatus;
}
