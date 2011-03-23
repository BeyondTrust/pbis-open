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
 *        lsa_addaccountrights.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaAddAccountRights function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaAddAccountRights(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN PSID            pAccountSid,
    IN PWSTR          *pAccountRights,
    IN DWORD           NumAccountRights
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    LSA_ACCOUNT_RIGHTS accountRights = {0};
    DWORD i = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(pAccountSid, ntStatus);
    BAIL_ON_INVALID_PTR(pAccountRights, ntStatus);

    err = LwAllocateMemory(
                     sizeof(accountRights.pAccountRight[0]) * NumAccountRights,
                     OUT_PPVOID(&accountRights.pAccountRight));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < NumAccountRights; i++)
    {
        err = LwAllocateUnicodeStringFromWc16String(
                                  &accountRights.pAccountRight[i],
                                  pAccountRights[i]);
        BAIL_ON_LSA_ERROR(err);
    }

    accountRights.NumAccountRights = NumAccountRights;

    DCERPC_CALL(ntStatus, cli_LsaAddAccountRights(
                              (handle_t)hBinding,
                              hPolicy,
                              pAccountSid,
                              &accountRights));
    BAIL_ON_NT_STATUS(ntStatus);

error:
    if (accountRights.pAccountRight)
    {
        for (i = 0; i < accountRights.NumAccountRights; i++)
        {
            LwFreeUnicodeString(&accountRights.pAccountRight[i]);
        }
        LW_SAFE_FREE_MEMORY(accountRights.pAccountRight);
    }

    if (ntStatus == STATUS_SUCCESS &&
        err != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(err);
    }

    return ntStatus;
}
