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
 *        samr_queryaliasinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrQueryAliasInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrQueryAliasInfo(
    IN  SAMR_BINDING     hBinding,
    IN  ACCOUNT_HANDLE   hAlias,
    IN  WORD             swLevel,
    OUT AliasInfo      **ppInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    AliasInfo *pInfo = NULL;
    AliasInfo *pOutInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hAlias, ntStatus);
    BAIL_ON_INVALID_PTR(ppInfo, ntStatus);

    DCERPC_CALL(ntStatus, cli_SamrQueryAliasInfo((handle_t)hBinding,
                                                 hAlias,
                                                 swLevel,
                                                 &pInfo));
    BAIL_ON_NT_STATUS(ntStatus);

    if (pInfo)
    {
        ntStatus = SamrAllocateAliasInfo(NULL,
                                         &dwOffset,
                                         NULL,
                                         swLevel,
                                         pInfo,
                                         &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = SamrAllocateMemory(OUT_PPVOID(&pOutInfo),
                                      dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateAliasInfo(pOutInfo,
                                         &dwOffset,
                                         &dwSpaceLeft,
                                         swLevel,
                                         pInfo,
                                         &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppInfo = pOutInfo;

cleanup:
    if (pInfo)
    {
        SamrFreeStubAliasInfo(pInfo, swLevel);
    }

    return ntStatus;

error:
    if (pOutInfo)
    {
        SamrFreeMemory(pOutInfo);
    }

    if (ppInfo)
    {
        *ppInfo = NULL;
    }

    goto cleanup;
}
