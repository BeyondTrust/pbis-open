/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
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
 *        samr_getmembersinalias.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrGetMembersInAlias function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrGetMembersInAlias(
    IN  SAMR_BINDING     hBinding,
    IN  ACCOUNT_HANDLE   hAlias,
    OUT PSID           **pppSids,
    OUT UINT32          *pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SID_ARRAY Sids = {0};
    PSID *ppSids = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hAlias, ntStatus);
    BAIL_ON_INVALID_PTR(pppSids, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    DCERPC_CALL(ntStatus, cli_SamrGetMembersInAlias((handle_t)hBinding,
                                                    hAlias,
                                                    &Sids));
    BAIL_ON_NT_STATUS(ntStatus);

    if (Sids.dwNumSids)
    {
        ntStatus = SamrAllocateSids(NULL,
                                    &dwOffset,
                                    NULL,
                                    &Sids,
                                    &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&ppSids),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateSids(ppSids,
                                    &dwOffset,
                                    &dwSpaceLeft,
                                    &Sids,
                                    &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pppSids = ppSids;
    *pCount  = Sids.dwNumSids;

cleanup:
    SamrCleanStubSidArray(&Sids);

    return ntStatus;

error:
    if (ppSids)
    {
        SamrFreeMemory(ppSids);
    }

    if (pppSids)
    {
        *pppSids = NULL;
    }

    if (pCount)
    {
        *pCount = 0;
    }

    goto cleanup;
}
