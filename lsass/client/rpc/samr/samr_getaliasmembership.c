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
 *        samr_getaliasmembership.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrGetAliasMembership function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrGetAliasMembership(
    IN  SAMR_BINDING   hBinding,
    IN  DOMAIN_HANDLE  hDomain,
    IN  PSID          *ppSids,
    IN  DWORD          dwNumSids,
    OUT PDWORD        *ppdwRids,
    OUT PDWORD         pdwCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 iSid = 0;
    SID_ARRAY Sids = {0};
    IDS Rids = {0};
    UINT32 *pRids = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(ppSids, ntStatus);
    BAIL_ON_INVALID_PTR(ppdwRids, ntStatus);
    BAIL_ON_INVALID_PTR(pdwCount, ntStatus);

    Sids.dwNumSids = dwNumSids;
    ntStatus = SamrAllocateMemory(OUT_PPVOID(&Sids.pSids),
                                  sizeof(Sids.pSids[0]) * dwNumSids);
    BAIL_ON_NT_STATUS(ntStatus);

    for (iSid = 0; iSid < dwNumSids; iSid++)
    {
        Sids.pSids[iSid].pSid = ppSids[iSid];
    }

    DCERPC_CALL(ntStatus, cli_SamrGetAliasMembership(
                                       (handle_t)hBinding,
                                       hDomain,
                                       &Sids,
                                       &Rids));
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = sizeof(pRids[0]) * Rids.dwCount;
    dwSize      = 0;

    ntStatus = SamrAllocateMemory(OUT_PPVOID(&pRids),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrAllocateIds(pRids,
                               &dwOffset,
                               &dwSpaceLeft,
                               &Rids,
                               &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppdwRids  = pRids;
    *pdwCount  = Rids.dwCount;

cleanup:
    SamrCleanStubIds(&Rids);

    return ntStatus;

error:
    if (pRids)
    {
        SamrFreeMemory(pRids);
    }

    if (ppdwRids)
    {
        *ppdwRids = NULL;
    }

    if (pdwCount)
    {
        *pdwCount = 0;
    }

    goto cleanup;
}
