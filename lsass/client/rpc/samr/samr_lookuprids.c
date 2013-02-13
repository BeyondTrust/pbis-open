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
 *        samr_lookuprids.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrLookupRids function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrLookupRids(
    IN  SAMR_BINDING    hBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT32          NumRids,
    IN  UINT32         *pRids,
    OUT PWSTR         **pppwszNames,
    OUT UINT32        **ppTypes
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntLookupStatus = STATUS_SUCCESS;
    UNICODE_STRING_ARRAY Names = {0};
    IDS Types = {0};
    PWSTR *ppwszNames = NULL;
    UINT32 *pTypes = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pRids, ntStatus);
    BAIL_ON_INVALID_PTR(pppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppTypes, ntStatus);

    DCERPC_CALL(ntStatus, cli_SamrLookupRids((handle_t)hBinding,
                                             hDomain,
                                             NumRids,
                                             pRids,
                                             &Names,
                                             &Types));
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != STATUS_SOME_NOT_MAPPED)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntLookupStatus = ntStatus;

    if (Names.dwCount > 0)
    {
        ntStatus = SamrAllocateNamesFromUnicodeStringArray(
                                      NULL,
                                      &dwOffset,
                                      NULL,
                                      &Names,
                                      &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&ppwszNames),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateNamesFromUnicodeStringArray(
                                      ppwszNames,
                                      &dwOffset,
                                      &dwSpaceLeft,
                                      &Names,
                                      &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = sizeof(pTypes[0]) * Types.dwCount;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&pTypes),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateIds(pTypes,
                                   &dwOffset,
                                   &dwSpaceLeft,
                                   &Types,
                                   &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pppwszNames = ppwszNames;
    *ppTypes     = pTypes;

cleanup:
    SamrCleanStubIds(&Types);
    SamrCleanStubUnicodeStringArray(&Names);

    if (ntStatus == STATUS_SUCCESS &&
        ntLookupStatus != STATUS_SUCCESS)
    {
        ntStatus = ntLookupStatus;
    }

    return ntStatus;

error:
    if (ppwszNames)
    {
        SamrFreeMemory(ppwszNames);
    }

    if (pTypes)
    {
        SamrFreeMemory(pTypes);
    }

    if (pppwszNames)
    {
        *pppwszNames = NULL;
    }

    if (ppTypes)
    {
        *ppTypes = NULL;
    }

    goto cleanup;
}
