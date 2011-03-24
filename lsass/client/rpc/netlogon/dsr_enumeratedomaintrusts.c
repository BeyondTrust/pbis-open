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
 *        dsr_enumeratedomaintrusts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsrEnumerateDomainTrusts function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
DsrEnumerateDomainTrusts(
    IN  NETR_BINDING       hBinding,
    IN  PCWSTR             pwszServer,
    IN  UINT32             Flags,
    OUT NetrDomainTrust  **ppTrusts,
    OUT PUINT32            pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszServerName = NULL;
    NetrDomainTrustList TrustList = {0};
    NetrDomainTrust *pTrusts = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(ppTrusts, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    dwError = LwAllocateWc16String(&pwszServerName, pwszServer);
    BAIL_ON_WIN_ERROR(dwError);

    DCERPC_CALL_WINERR(dwError, cli_DsrEnumerateDomainTrusts(
                                        (handle_t)hBinding,
                                        pwszServerName,
                                        Flags,
                                        &TrustList));
    BAIL_ON_WIN_ERROR(dwError);

    ntStatus = NetrAllocateDomainTrusts(NULL,
                                        &dwOffset,
                                        NULL,
                                        &TrustList,
                                        &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    dwSpaceLeft = dwSize;
    dwSize      = 0;
    dwOffset    = 0;

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pTrusts),
                                  dwSpaceLeft);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateDomainTrusts(pTrusts,
                                        &dwOffset,
                                        &dwSpaceLeft,
                                        &TrustList,
                                        &dwSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *pCount   = TrustList.count;
    *ppTrusts = pTrusts;

cleanup:
    NetrCleanStubDomainTrustList(&TrustList);
    LW_SAFE_FREE_MEMORY(pwszServerName);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;

error:
    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    if (pCount)
    {
        *pCount = 0;
    }

    if (ppTrusts)
    {
        *ppTrusts = NULL;
    }

    goto cleanup;
}
