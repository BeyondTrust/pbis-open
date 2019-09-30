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
 *        samr_getusergroups.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrGetUserGroups function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrGetUserGroups(
    IN  SAMR_BINDING     hBinding,
    IN  ACCOUNT_HANDLE   hUser,
    OUT UINT32         **ppRids,
    OUT UINT32         **ppAttributes,
    OUT UINT32          *pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 *pRids = NULL;
    UINT32 *pAttributes = NULL;
    PRID_WITH_ATTRIBUTE_ARRAY pRidWithAttr = NULL;
    DWORD dwRidCount = 0;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hUser, ntStatus);
    BAIL_ON_INVALID_PTR(ppRids, ntStatus);
    BAIL_ON_INVALID_PTR(ppAttributes, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    DCERPC_CALL(ntStatus, cli_SamrGetUserGroups((handle_t)hBinding,
                                                hUser,
                                                &pRidWithAttr));
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRidWithAttr)
    {
        dwRidCount = pRidWithAttr->dwCount;
        dwSpaceLeft = sizeof(pRids[0]) * dwRidCount;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&pRids),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateRidsFromRidWithAttributeArray(
                                      pRids,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      pRidWithAttr,
                                      &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = sizeof(pAttributes[0]) * dwRidCount;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&pAttributes),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateAttributesFromRidWithAttributeArray(
                                      pAttributes,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      pRidWithAttr,
                                      &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRids       = pRids;
    *ppAttributes = pAttributes;
    *pCount       = dwRidCount;

cleanup:
    if (pRidWithAttr)
    {
        SamrFreeStubRidWithAttributeArray(pRidWithAttr);
    }

    return ntStatus;

error:
    if (pRids)
    {
        SamrFreeMemory(pRids);
    }

    if (pAttributes)
    {
        SamrFreeMemory(pAttributes);
    }

    if (ppRids)
    {
        *ppRids = NULL;
    }

    if (ppAttributes)
    {
        *ppAttributes = NULL;
    }

    if (pCount)
    {
        *pCount = 0;
    }

    goto cleanup;
}
