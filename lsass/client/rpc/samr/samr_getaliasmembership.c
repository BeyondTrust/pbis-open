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
