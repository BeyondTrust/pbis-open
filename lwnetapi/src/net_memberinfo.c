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
 *        net_memberinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI local group members info buffer handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
NetAllocateLocalGroupMembersInfo0(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


static
DWORD
NetAllocateLocalGroupMembersInfo3(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


static
DWORD
NetAllocateLocalGroupUsersInfo0(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocateLocalGroupMembersInfo(
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
        err = NetAllocateLocalGroupMembersInfo0(&pCursor,
                                                pdwSpaceLeft,
                                                pSource,
                                                pdwSize,
                                                eValidation);
        break;

    case 3:
        err = NetAllocateLocalGroupMembersInfo3(&pCursor,
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
NetAllocateLocalGroupMembersInfo0(
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
    PSID pSid = (PSID)pSource;
    DWORD dwSidLen = 0;

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

    dwSidLen = RtlLengthSid(pSid);

    /* lgrmi0_sid */
    err = NetAllocBufferSid(&pCursor,
                            &dwSpaceLeft,
                            pSid,
                            dwSidLen,
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
NetAllocateLocalGroupMembersInfo3(
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
    PNET_RESOLVED_NAME pName = (PNET_RESOLVED_NAME)pSource;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAccountName = NULL;

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

    err = LwAllocateWc16StringFromUnicodeString(
                             &pwszDomainName,
                             &pName->DomainName);
    BAIL_ON_WIN_ERROR(err);

    err = LwAllocateWc16StringFromUnicodeString(
                             &pwszAccountName,
                             &pName->AccountName);
    BAIL_ON_WIN_ERROR(err);

    /* lgrmi3_domainandname */
    err = NetAllocBufferNT4Name(&pCursor,
                                &dwSpaceLeft,
                                pwszDomainName,
                                pwszAccountName,
                                &dwSize);
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


DWORD
NetAllocateLocalGroupUsersInfo(
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
        err = NetAllocateLocalGroupUsersInfo0(&pCursor,
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
NetAllocateLocalGroupUsersInfo0(
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

    /* lgrui0_name */
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
