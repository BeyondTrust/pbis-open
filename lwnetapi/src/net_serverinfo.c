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
 *        net_serverinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI server info buffer handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
NetAllocateServerInfo100(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


static
DWORD
NetAllocateServerInfo101(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocateServerInfo(
    PVOID                 pBuffer,
    PDWORD                pdwSpaceLeft,
    DWORD                 dwLevel,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pCursor = pBuffer;

    switch (dwLevel)
    {
    case 100:
        dwError = NetAllocateServerInfo100(&pCursor,
                                           pdwSpaceLeft,
                                           pSource,
                                           pdwSize,
                                           eValidation);
        break;

    case 101:
        dwError = NetAllocateServerInfo101(&pCursor,
                                           pdwSpaceLeft,
                                           pSource,
                                           pdwSize,
                                           eValidation);
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        break;
    }
    BAIL_ON_WIN_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
DWORD
NetAllocateServerInfo100(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PSERVER_INFO_100 pInfo = (PSERVER_INFO_100)pSource;

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pdwSize)
    {
        dwSize = *pdwSize;
    }

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    /* sv100_platform_id */
    dwError = NetAllocBufferDword(&pCursor,
                                  &dwSpaceLeft,
                                  pInfo->sv100_platform_id,
                                  &dwSize);
    BAIL_ON_WIN_ERROR(dwError);

    ALIGN_PTR_IN_BUFFER(SERVER_INFO_100, sv100_platform_id,
                        pCursor, dwSize, dwSpaceLeft);
    /* sv100_name */
    dwError = NetAllocBufferWC16String(&pCursor,
                                       &dwSpaceLeft,
                                       pInfo->sv100_name,
                                       &dwSize,
                                       eValidation);
    BAIL_ON_WIN_ERROR(dwError);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

    if (ppCursor)
    {
        *ppCursor = pCursor;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
DWORD
NetAllocateServerInfo101(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PSERVER_INFO_101 pInfo = (PSERVER_INFO_101)pSource;

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pdwSize)
    {
        dwSize = *pdwSize;
    }

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    dwError = NetAllocateServerInfo100(&pCursor,
                                       &dwSpaceLeft,
                                       pSource,
                                       &dwSize,
                                       eValidation);
    BAIL_ON_WIN_ERROR(dwError);

    /* sv101_version_major */
    dwError = NetAllocBufferDword(&pCursor,
                                  &dwSpaceLeft,
                                  pInfo->sv101_version_major,
                                  &dwSize);
    BAIL_ON_WIN_ERROR(dwError);

    /* sv101_version_minor */
    dwError = NetAllocBufferDword(&pCursor,
                                  &dwSpaceLeft,
                                  pInfo->sv101_version_minor,
                                  &dwSize);
    BAIL_ON_WIN_ERROR(dwError);

    /* sv101_type */
    dwError = NetAllocBufferDword(&pCursor,
                                  &dwSpaceLeft,
                                  pInfo->sv101_type,
                                  &dwSize);
    BAIL_ON_WIN_ERROR(dwError);

    ALIGN_PTR_IN_BUFFER(SERVER_INFO_101, sv101_type,
                        pCursor, dwSize, dwSpaceLeft);

    /* sv101_comment */
    dwError = NetAllocBufferWC16String(&pCursor,
                                       &dwSpaceLeft,
                                       pInfo->sv101_comment,
                                       &dwSize,
                                       eValidation);
    BAIL_ON_WIN_ERROR(dwError);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

    if (ppCursor)
    {
        *ppCursor = pCursor;
    }

cleanup:
    return dwError;

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
