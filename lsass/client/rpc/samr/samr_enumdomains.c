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
 *        samr_enumdomains.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrEnumDomains function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrEnumDomains(
    IN  SAMR_BINDING     hBinding,
    IN  CONNECT_HANDLE   hConn,
    IN OUT UINT32       *pResume,
    IN  UINT32           Size,
    OUT PWSTR          **pppwszNames,
    OUT UINT32          *pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntRetStatus = STATUS_SUCCESS;
    PENTRY_ARRAY pDomains = NULL;
    UINT32 Resume = 0;
    UINT32 Count = 0;
    PWSTR *ppwszNames = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hConn, ntStatus);
    BAIL_ON_INVALID_PTR(pResume, ntStatus);
    BAIL_ON_INVALID_PTR(pppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    Resume = *pResume;

    DCERPC_CALL(ntStatus, cli_SamrEnumDomains((handle_t)hBinding,
                                              hConn,
                                              &Resume,
                                              Size,
                                              &pDomains,
                                              &Count));

    /* Preserve returned status code */
    ntRetStatus = ntStatus;

    /* Status other than success doesn't have to mean failure here */
    if (ntRetStatus != STATUS_SUCCESS &&
        ntRetStatus != STATUS_MORE_ENTRIES)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pDomains)
    {
        ntStatus = SamrAllocateNames(NULL,
                                     &dwOffset,
                                     NULL,
                                     pDomains,
                                     &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&ppwszNames),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateNames(ppwszNames,
                                     &dwOffset,
                                     &dwSpaceLeft,
                                     pDomains,
                                     &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pResume     = Resume;
    *pCount      = Count;
    *pppwszNames = ppwszNames;

cleanup:
    if (pDomains)
    {
        SamrFreeStubEntryArray(pDomains);
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

    goto cleanup;
}
