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
 *        net_groupinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI group info buffer handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
DWORD
NetAllocateLocalGroupInfo0(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


static
DWORD
NetAllocateLocalGroupInfo1(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocateLocalGroupInfo(
    PVOID                 pInfoBuffer,
    PDWORD                pdwSpaceLeft, 
    DWORD                 dwLevel,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = pInfoBuffer;

    switch (dwLevel)
    {
    case 0:
        err = NetAllocateLocalGroupInfo0(&pCursor,
                                         pdwSpaceLeft,
                                         pSource,
                                         pdwSize,
                                         eValidation);
        break;

    case 1:
        err = NetAllocateLocalGroupInfo1(&pCursor,
                                         pdwSpaceLeft,
                                         pSource,
                                         pdwSize,
                                         eValidation);
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        break;
    }
    BAIL_ON_WIN_ERROR(err);

cleanup:
    return err;

error:
    goto cleanup;
}


static
DWORD
NetAllocateLocalGroupInfo0(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWSTR pwszName = (PWSTR)pSource;

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

    /* lgrpi0_name */
    err = NetAllocBufferWC16String(&pCursor,
                                   &dwSpaceLeft,
                                   pwszName,
                                   &dwSize,
                                   eValidation);
    BAIL_ON_WIN_ERROR(err);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}
    

static
DWORD
NetAllocateLocalGroupInfo1(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    AliasInfo *pInfo = (AliasInfo*)pSource;
    AliasInfoAll *pInfoAll = &pInfo->all;

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

    /* lgrpi1_name */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pInfoAll->name,
                                   &dwSize,
                                   eValidation);
    BAIL_ON_WIN_ERROR(err);

    /* lgrpi1_comment */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pInfoAll->description,
                                   &dwSize,
                                   eValidation);
    BAIL_ON_WIN_ERROR(err);

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
    return err;

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
