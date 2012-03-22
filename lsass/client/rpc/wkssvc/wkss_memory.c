/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2010
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
 *        wkss_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        WksSvc rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
WkssAllocateNetrWkstaUserInfo0(
    OUT PVOID                      pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN  PNETR_WKSTA_USER_INFO_0    pIn,
    IN OUT PDWORD                  pdwSize
    );


static
DWORD
WkssAllocateNetrWkstaUserInfo1(
    OUT PVOID                      pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN  PNETR_WKSTA_USER_INFO_1    pIn,
    IN OUT PDWORD                  pdwSize
    );


WINERROR
WkssAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t sSize
    )
{
    WINERROR winError = ERROR_SUCCESS;
    PVOID pMem = NULL;

    pMem = malloc(sSize);
    if (pMem == NULL)
    {
        winError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN_ERROR(winError);
    }

    memset(pMem, 0, sSize);
    *ppOut = pMem;

cleanup:
    return winError;

error:
    *ppOut = NULL;
    goto cleanup;
}


VOID
WkssFreeMemory(
    IN PVOID pPtr
    )
{
    free(pPtr);
}


DWORD
WkssAllocateNetrWkstaInfo(
    OUT PNETR_WKSTA_INFO  pOut,
    IN OUT PDWORD         pdwOffset,
    IN OUT PDWORD         pdwSpaceLeft,
    IN  DWORD             dwLevel,
    IN  PNETR_WKSTA_INFO  pIn,
    IN OUT PDWORD         pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = NULL;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    if (dwLevel >= 100 && dwLevel <= 102)
    {
        pBuffer = pOut->pInfo100;

        LWBUF_ALLOC_DWORD(pBuffer, pIn->pInfo100->wksta100_platform_id);
        LWBUF_ALLOC_WC16STR(pBuffer, pIn->pInfo100->wksta100_name);
        LWBUF_ALLOC_WC16STR(pBuffer, pIn->pInfo100->wksta100_domain);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->pInfo100->wksta100_version_major);
        LWBUF_ALLOC_DWORD(pBuffer, pIn->pInfo100->wksta100_version_minor);
    }

    if (dwLevel >= 101 && dwLevel <= 102)
    {
        /* Level 101 is an extension of level 100 and
           pBuffer points to the same place as pInfo101 */
        LWBUF_ALLOC_WC16STR(pBuffer, pIn->pInfo101->wksta101_domain);
    }

    if (dwLevel == 102)
    {
        /* Level 102 is an extension of level 101 and
           pBuffer points to the same place as pInfo102 */
        LWBUF_ALLOC_DWORD(pBuffer, pIn->pInfo102->wksta102_logged_users);
    }

    if (pBuffer == NULL &&
        pdwSpaceLeft != NULL)
    {
        /* No matching infolevel has been found */
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(dwError);
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
WkssAllocateNetrWkstaUserInfo(
    OUT PVOID                      pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN DWORD                       dwLevel,
    IN  PNETR_WKSTA_USER_INFO_CTR  pIn,
    IN OUT PDWORD                  pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD iUser = 0;
    DWORD dwCount = 0;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    switch (dwLevel)
    {
    case 0:
        dwCount = pIn->pInfo0->dwCount;
        break;

    case 1:
        dwCount = pIn->pInfo1->dwCount;
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(dwError);
    }

    for (iUser = 0; iUser < dwCount; iUser++)
    {
        switch (dwLevel)
        {
        case 0:
            dwError = WkssAllocateNetrWkstaUserInfo0(
                                           pBuffer,
                                           pdwOffset,
                                           pdwSpaceLeft,
                                           &(pIn->pInfo0->pInfo[iUser]),
                                           pdwSize);
            break;

        case 1:
            dwError = WkssAllocateNetrWkstaUserInfo1(
                                           pBuffer,
                                           pdwOffset,
                                           pdwSpaceLeft,
                                           &(pIn->pInfo1->pInfo[iUser]),
                                           pdwSize);
            break;
        }
        BAIL_ON_WIN_ERROR(dwError);
    }

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
WkssAllocateNetrWkstaUserInfo0(
    OUT PVOID                      pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN  PNETR_WKSTA_USER_INFO_0    pIn,
    IN OUT PDWORD                  pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALLOC_WC16STR(pBuffer, pIn->wkui0_username);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
WkssAllocateNetrWkstaUserInfo1(
    OUT PVOID                      pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN  PNETR_WKSTA_USER_INFO_1    pIn,
    IN OUT PDWORD                  pdwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pBuffer = pOut;

    BAIL_ON_INVALID_PTR(pdwOffset, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);
    BAIL_ON_INVALID_PTR(pdwSize, ntStatus);

    LWBUF_ALLOC_WC16STR(pBuffer, pIn->wkui1_username);
    LWBUF_ALLOC_WC16STR(pBuffer, pIn->wkui1_logon_domain);
    LWBUF_ALLOC_WC16STR(pBuffer, pIn->wkui1_oth_domains);
    LWBUF_ALLOC_WC16STR(pBuffer, pIn->wkui1_logon_server);

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return dwError;

error:
    goto cleanup;
}
