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
 *        samr_createuser2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrCreateUser2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrCreateUser2(
    IN  handle_t        hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  PWSTR           pwszAccountName,
    IN  UINT32          AccountFlags,
    IN  UINT32          AccessMask,
    OUT ACCOUNT_HANDLE *phUser,
    OUT PUINT32         pAccessGranted,
    OUT PUINT32         pRid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UnicodeStringEx AccountName = {0};
    ACCOUNT_HANDLE hUser = NULL;
    UINT32 AccessGranted = 0;
    UINT32 Rid = 0;

    BAIL_ON_INVALID_PTR(hSamrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccountName, ntStatus);
    BAIL_ON_INVALID_PTR(phUser, ntStatus);
    BAIL_ON_INVALID_PTR(pAccessGranted, ntStatus);
    BAIL_ON_INVALID_PTR(pRid, ntStatus);

    ntStatus = InitUnicodeStringEx(&AccountName, pwszAccountName);
    BAIL_ON_NT_STATUS(ntStatus);

    DCERPC_CALL(ntStatus, _SamrCreateUser2(hSamrBinding,
                                           hDomain,
                                           &AccountName,
                                           AccountFlags,
                                           AccessMask,
                                           &hUser,
                                           &AccessGranted,
                                           &Rid));
    BAIL_ON_NT_STATUS(ntStatus);

    *phUser         = hUser;
    *pAccessGranted = AccessGranted;
    *pRid           = Rid;

cleanup:
    FreeUnicodeStringEx(&AccountName);

    return ntStatus;

error:
    phUser          = NULL;
    *pAccessGranted = 0;
    *pRid           = 0;

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
