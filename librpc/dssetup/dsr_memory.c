/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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


NTSTATUS
DsrAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t sSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pMem = NULL;

    pMem = malloc(sSize);
    if (pMem == NULL)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memset(pMem, 0, sSize);
    *ppOut = pMem;

cleanup:
    return ntStatus;

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


NTSTATUS
DsrAllocateDsRoleInfo(
    OUT PDS_ROLE_INFO   pOut,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN  PDS_ROLE_INFO   pIn,
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
        LWBUF_ALLOC_DWORD(pBuffer, pIn->basic.dwRole);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->basic.dwFlags);
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->basic.pwszDomain);
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->basic.pwszDnsDomain);
        LWBUF_ALLOC_PWSTR(pBuffer, pIn->basic.pwszForest);
        LWBUF_ALLOC_BLOB(pBuffer,
                         sizeof(pIn->basic.DomainGuid),
                         &pIn->basic.DomainGuid);
        break;

    case DS_ROLE_UPGRADE_STATUS:
        LWBUF_ALLOC_WORD(pBuffer, pIn->upgrade.swUpgradeStatus);
        LWBUF_ALIGN_TYPE(pdwOffset, pdwSize, pdwSpaceLeft, DWORD);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->upgrade.dwPrevious);
        break;

    case DS_ROLE_OP_STATUS:
        LWBUF_ALLOC_WORD(pBuffer, pIn->opstatus.swStatus);
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
    }

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
