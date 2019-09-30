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
