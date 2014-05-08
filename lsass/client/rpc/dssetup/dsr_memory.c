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
 *        dsr_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsSetup rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
DsrInitMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    return ntStatus;
}


NTSTATUS
DsrDestroyMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    return ntStatus;
}


DWORD
DsrAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t sSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pMem = NULL;

    pMem = malloc(sSize);
    if (pMem == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN_ERROR(dwError);
    }

    memset(pMem, 0, sSize);
    *ppOut = pMem;

cleanup:
    return dwError;

error:
    *ppOut = NULL;
    goto cleanup;
}


VOID
DsrFreeMemory(
    IN OUT PVOID pPtr
    )
{
    free(pPtr);
}


DWORD
DsrAllocateDsRoleInfo(
    OUT PDSR_ROLE_INFO  pOut,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN  PDSR_ROLE_INFO  pIn,
    IN  WORD            swLevel,
    IN OUT PDWORD       pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    switch(swLevel)
    {
    case DS_ROLE_BASIC_INFORMATION:
        LWBUF_ALLOC_DWORD(pBuffer, pIn->Basic.dwRole);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->Basic.dwFlags);
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->Basic.pwszDomain);
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->Basic.pwszDnsDomain);
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->Basic.pwszForest);
        LWBUF_ALLOC_BLOB(pBuffer,
                         sizeof(pIn->Basic.DomainGuid),
                         &pIn->Basic.DomainGuid);
        break;

    case DS_ROLE_UPGRADE_STATUS:
        LWBUF_ALLOC_WORD(pBuffer, pIn->Upgrade.swUpgradeStatus);
        LWBUF_ALIGN_TYPE(pdwOffset, pdwSize, pdwSpaceLeft, DWORD);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->Upgrade.dwPrevious);
        break;

    case DS_ROLE_OP_STATUS:
        LWBUF_ALLOC_WORD(pBuffer, pIn->OpStatus.swStatus);
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
    }

    BAIL_ON_WIN_ERROR(dwError);

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(dwError);
    }

    return dwError;

error:
    goto cleanup;
}
