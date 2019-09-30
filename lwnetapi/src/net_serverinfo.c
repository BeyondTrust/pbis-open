/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
