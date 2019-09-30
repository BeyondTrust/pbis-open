/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
    IN  SAMR_BINDING    hBinding,
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
    DWORD dwError = ERROR_SUCCESS;
    UNICODE_STRING AccountName = {0};
    ACCOUNT_HANDLE hUser = NULL;
    UINT32 AccessGranted = 0;
    UINT32 Rid = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccountName, ntStatus);
    BAIL_ON_INVALID_PTR(phUser, ntStatus);
    BAIL_ON_INVALID_PTR(pAccessGranted, ntStatus);
    BAIL_ON_INVALID_PTR(pRid, ntStatus);

    dwError = LwAllocateUnicodeStringFromWc16String(
                                    &AccountName,
                                    pwszAccountName);
    BAIL_ON_WIN_ERROR(dwError);

    DCERPC_CALL(ntStatus, cli_SamrCreateUser2((handle_t)hBinding,
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
    LwFreeUnicodeString(&AccountName);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (phUser)
    {
        *phUser = NULL;
    }

    if (pAccessGranted)
    {
        *pAccessGranted = 0;
    }

    if (pRid)
    {
        *pRid = 0;
    }

    goto cleanup;
}
