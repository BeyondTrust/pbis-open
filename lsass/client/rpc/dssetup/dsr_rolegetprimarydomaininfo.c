/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        dsr_rolegetprimarydomaininfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsrRoleGetPrimaryDomainInformation function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERR
DsrRoleGetPrimaryDomainInformation(
    handle_t hBinding,
    UINT16 uiLevel,
    PDS_ROLE_INFO *ppInfo
    )
{
    WINERR dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDS_ROLE_INFO pStubInfo = NULL;
    PDS_ROLE_INFO pInfo = NULL;
    DWORD dwOffset = 0;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;

    DCERPC_CALL(ntStatus, cli_DsrRoleGetPrimaryDomainInformation(
                              hBinding,
                              uiLevel,
                              &pStubInfo));
    BAIL_ON_NT_STATUS(ntStatus);

    if (pStubInfo)
    {
        ntStatus = DsrAllocateDsRoleInfo(NULL,
                                         &dwOffset,
                                         NULL,
                                         pStubInfo,
                                         uiLevel,
                                         &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        dwSpaceLeft = dwSize;
        dwSize      = 0;
        dwOffset    = 0;

        ntStatus = DsrAllocateMemory(OUT_PPVOID(&pInfo),
                                     dwSpaceLeft);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = DsrAllocateDsRoleInfo(pInfo,
                                         &dwOffset,
                                         &dwSpaceLeft,
                                         pStubInfo,
                                         uiLevel,
                                         &dwSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppInfo = pInfo;
    pInfo = NULL;

cleanup:
    if (pStubInfo)
    {
        DsrFreeStubDsRoleInfo(pStubInfo, uiLevel);
    }

    if ((dwError == ERROR_SUCCESS) &&
        (ntStatus != STATUS_SUCCESS))
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    if (pInfo)
    {
        DsrFreeMemory(pInfo);
    }

    *ppInfo = NULL;
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
