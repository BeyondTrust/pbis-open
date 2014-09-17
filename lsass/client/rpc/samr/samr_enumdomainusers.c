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
 *        samr_enumdomainusers.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrEnumDomainUsers function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrEnumDomainUsers(
    IN  SAMR_BINDING     hBinding,
    IN  ACCOUNT_HANDLE   hDomain,
    IN OUT UINT32       *pResume,
    IN  UINT32           AccountFlags,
    IN  UINT32           MaxSize,
    OUT PWSTR          **pppwszNames,
    OUT UINT32         **ppRids,
    OUT UINT32          *pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntRetStatus = STATUS_SUCCESS;
    UINT32 Count = 0;
    UINT32 Resume = 0;
    PRID_NAME_ARRAY pEntries = NULL;
    PWSTR *ppwszNames = NULL;
    UINT32 *pRids = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pResume, ntStatus);
    BAIL_ON_INVALID_PTR(pppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppRids, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    Resume = *pResume;

    DCERPC_CALL(ntStatus, cli_SamrEnumDomainUsers((handle_t)hBinding,
                                                  hDomain,
                                                  &Resume,
                                                  AccountFlags,
                                                  MaxSize,
                                                  &pEntries,
                                                  &Count));

    /* Preserve returned status code */
    ntRetStatus = ntStatus;

    /* Status other than success doesn't have to mean failure here */
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != STATUS_MORE_ENTRIES)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pEntries)
    {
        ntStatus = SamrAllocateNamesFromRidNameArray(
                                            NULL,
                                            &dwOffset,
                                            NULL,
                                            pEntries,
                                            &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&ppwszNames),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateNamesFromRidNameArray(
                                            ppwszNames,
                                            &dwOffset,
                                            &dwSpaceLeft,
                                            pEntries,
                                            &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwOffset = 0;
        dwSize   = 0;

        ntStatus = SamrAllocateRidsFromRidNameArray(
                                            NULL,
                                            &dwOffset,
                                            &dwSpaceLeft,
                                            pEntries,
                                            &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&pRids),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateRidsFromRidNameArray(
                                            pRids,
                                            &dwOffset,
                                            &dwSpaceLeft,
                                            pEntries,
                                            &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pResume     = Resume;
    *pCount      = Count;
    *pppwszNames = ppwszNames;
    *ppRids      = pRids;

cleanup:
    if (pEntries)
    {
        SamrFreeStubRidNameArray(pEntries);
    }

    if (ntStatus == STATUS_SUCCESS &&
        (ntRetStatus == STATUS_SUCCESS ||
         ntRetStatus == STATUS_MORE_ENTRIES))
    {
        ntStatus = ntRetStatus;
    }

    return ntStatus;

error:
    if (ppwszNames)
    {
        SamrFreeMemory(ppwszNames);
    }

    if (pRids)
    {
        SamrFreeMemory(pRids);
    }

    if (pResume)
    {
        *pResume = 0;
    }

    if (pCount)
    {
        *pCount = 0;
    }

    if (pppwszNames)
    {
        *pppwszNames = NULL;
    }

    if (ppRids)
    {
        *ppRids = NULL;
    }

    goto cleanup;
}
