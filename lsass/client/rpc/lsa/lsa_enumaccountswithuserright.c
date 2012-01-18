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
 *        lsa_enumaccountswithuserright.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaEnumAccountsWithUserRight function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaEnumAccountsWithUserRight(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN PCWSTR          UserRight,
    OUT PSID         **pppAccounts,
    OUT PDWORD         pNumAccounts
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UNICODE_STRING userRight = {0};
    LSA_ACCOUNT_ENUM_BUFFER accounts = {0};
    PSID *ppSids = NULL;
    DWORD offset = 0;
    DWORD spaceLeft = 0;
    DWORD size = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(pppAccounts, ntStatus);
    BAIL_ON_INVALID_PTR(pNumAccounts, ntStatus);

    ntStatus = RtlUnicodeStringAllocateFromWC16String(
                              &userRight,
                              UserRight);
    BAIL_ON_NT_STATUS(ntStatus);

    DCERPC_CALL(ntStatus, cli_LsaEnumAccountsWithUserRight(
                              (handle_t)hBinding,
                              hPolicy,
                              &userRight,
                              &accounts));
    BAIL_ON_NT_STATUS(ntStatus);

    offset    = 0;
    spaceLeft = 0;
    size      = 0;

    ntStatus = LsaAllocateSids(
                          ppSids,
                          &offset,
                          &spaceLeft,
                          &accounts,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    offset    = 0;
    spaceLeft = size;
    size      = 0;

    ntStatus = LsaRpcAllocateMemory(
                          OUT_PPVOID(&ppSids),
                          spaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaAllocateSids(
                          ppSids,
                          &offset,
                          &spaceLeft,
                          &accounts,
                          &size);
    BAIL_ON_NT_STATUS(ntStatus);

    *pppAccounts  = ppSids;
    *pNumAccounts = accounts.NumAccounts;

error:
    if (ntStatus)
    {
        if (ppSids)
        {
            LsaRpcFreeMemory(ppSids);
        }

        if (pppAccounts)
        {
            *pppAccounts = NULL;
        }

        if (pNumAccounts)
        {
            *pNumAccounts = 0;
        }
    }

    LsaCleanStubAccountBuffer(&accounts);
    RtlUnicodeStringFree(&userRight);

    return ntStatus;
}
