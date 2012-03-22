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
