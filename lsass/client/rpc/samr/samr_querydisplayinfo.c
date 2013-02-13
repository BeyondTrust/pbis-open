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
 *        samr_querydisplayinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrQueryDisplayInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrQueryDisplayInfo(
    IN  SAMR_BINDING      hBinding,
    IN  DOMAIN_HANDLE     hDomain,
    IN  WORD              swLevel,
    IN  UINT32            StartIdx,
    IN  UINT32            MaxEntries,
    IN  UINT32            BufferSize,
    OUT UINT32           *pTotalSize,
    OUT UINT32           *pReturnedSize,
    OUT SamrDisplayInfo **ppInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntRetStatus = STATUS_SUCCESS;
    UINT32 TotalSize = 0;
    UINT32 ReturnedSize = 0;
    SamrDisplayInfo Info;
    SamrDisplayInfo *pDispInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pTotalSize, ntStatus);
    BAIL_ON_INVALID_PTR(pReturnedSize, ntStatus);
    BAIL_ON_INVALID_PTR(ppInfo, ntStatus);

    memset(&Info, 0, sizeof(Info));

    DCERPC_CALL(ntStatus, cli_SamrQueryDisplayInfo((handle_t)hBinding,
                                                   hDomain,
                                                   swLevel,
                                                   StartIdx,
                                                   MaxEntries,
                                                   BufferSize,
                                                   &TotalSize,
                                                   &ReturnedSize,
                                                   &Info));
    /* Preserve returned status code */
    ntRetStatus = ntStatus;

    /* Status other than success doesn't have to mean failure here */
    if (ntStatus != STATUS_SUCCESS &&
        ntStatus != STATUS_MORE_ENTRIES)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SamrAllocateDisplayInfo(NULL,
                                       &dwOffset,
                                       NULL,
                                       swLevel,
                                       &Info,
                                       &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = SamrAllocateMemory(OUT_PPVOID(&pDispInfo),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrAllocateDisplayInfo(pDispInfo,
                                       &dwOffset,
                                       &dwSpaceLeft,
                                       swLevel,
                                       &Info,
                                       &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *pTotalSize    = TotalSize;
    *pReturnedSize = ReturnedSize;
    *ppInfo        = pDispInfo;

cleanup:
    SamrCleanStubDisplayInfo(&Info, swLevel);

    if (ntStatus == STATUS_SUCCESS &&
        (ntRetStatus == STATUS_SUCCESS ||
         ntRetStatus == STATUS_MORE_ENTRIES))
    {
        ntStatus = ntRetStatus;
    }

    return ntStatus;

error:
    if (pDispInfo)
    {
        SamrFreeMemory(pDispInfo);
    }

    if (pTotalSize)
    {
        *pTotalSize = 0;
    }

    if (pReturnedSize)
    {
        *pReturnedSize = 0;
    }

    if (ppInfo)
    {
        *ppInfo = NULL;
    }

    goto cleanup;
}
