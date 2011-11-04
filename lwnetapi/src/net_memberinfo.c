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
