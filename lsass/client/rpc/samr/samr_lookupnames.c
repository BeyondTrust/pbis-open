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
 *        samr_lookupnames.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrLookupNames function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrLookupNames(
    IN  SAMR_BINDING    hBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  DWORD           dwNumNames,
    IN  PWSTR          *ppwszNames,
    OUT UINT32        **ppRids,
    OUT UINT32        **ppTypes,
    OUT UINT32         *pRidsCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntLookupStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PUNICODE_STRING pNames = NULL;
    DWORD iName = 0;
    IDS Rids = {0};
    IDS Types = {0};
    UINT32 *pRids = NULL;
    UINT32 *pTypes = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(ppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppRids, ntStatus);
    BAIL_ON_INVALID_PTR(ppTypes, ntStatus);
    /* pRidsCount can be NULL, in which case the number of returned rids must
       match num_names. */

    dwError = LwAllocateMemory(sizeof(pNames[0]) * dwNumNames,
                               OUT_PPVOID(&pNames));
    BAIL_ON_WIN_ERROR(dwError);

    for (iName = 0; iName < dwNumNames; iName++)
    {
        dwError = LwAllocateUnicodeStringFromWc16String(
                                      &pNames[iName],
                                      ppwszNames[iName]);
        BAIL_ON_WIN_ERROR(dwError);
    }
    
    DCERPC_CALL(ntStatus, cli_SamrLookupNames((handle_t)hBinding,
                                              hDomain,
                                              dwNumNames,
                                              pNames,
                                              &Rids,
                                              &Types));
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != STATUS_SOME_NOT_MAPPED)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntLookupStatus = ntStatus;

    if (Rids.dwCount != Types.dwCount)
    {
        ntStatus = STATUS_REPLY_MESSAGE_MISMATCH;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SamrAllocateIds(NULL,
                               &dwOffset,
                               NULL,
                               &Rids,
                               &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = sizeof(pRids[0]) * Rids.dwCount;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = SamrAllocateMemory(OUT_PPVOID(&pRids),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrAllocateIds(pRids,
                               &dwOffset,
                               &dwSpaceLeft,
                               &Rids,
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

    if (pRidsCount)
    {
        *pRidsCount = Rids.dwCount;
    }
    else if (Rids.dwCount != dwNumNames)
    {
        ntStatus = STATUS_REPLY_MESSAGE_MISMATCH;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRids  = pRids;
    *ppTypes = pTypes;

cleanup:
    SamrCleanStubIds(&Rids);
    SamrCleanStubIds(&Types);

    if (pNames)
    {
        for (iName = 0; iName < dwNumNames; iName++)
        {
            LwFreeUnicodeString(&(pNames[iName]));
        }

        LW_SAFE_FREE_MEMORY(pNames);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    if (ntStatus == STATUS_SUCCESS &&
        ntLookupStatus != STATUS_SUCCESS)
    {
        ntStatus = ntLookupStatus;
    }

    return ntStatus;

error:
    if (pRids)
    {
        SamrFreeMemory(pRids);
    }

    if (pTypes)
    {
        SamrFreeMemory(pTypes);
    }

    if (pRidsCount)
    {
        *pRidsCount = 0;
    }

    if (ppRids)
    {
        *ppRids = NULL;
    }

    if (ppTypes)
    {
        *ppTypes = NULL;
    }

    goto cleanup;
}
