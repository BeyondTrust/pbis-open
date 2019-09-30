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
 *        samr_openuser.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrOpenUser function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvOpenUser(
    IN  handle_t         hBinding,
    IN  DOMAIN_HANDLE    hDomain,
    IN  DWORD            dwAccessMask,
    IN  DWORD            dwRid,
    OUT ACCOUNT_HANDLE  *phUser
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ACCOUNT_HANDLE hUser = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;

    ntStatus = SamrSrvOpenAccount(hBinding,
                                  hDomain,
                                  dwAccessMask,
                                  dwRid,
                                  DS_OBJECT_CLASS_USER,
                                  &hUser);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pAcctCtx = (PACCOUNT_CONTEXT)hUser;
    pAcctCtx->dwAccountType = SID_TYPE_USER;

    *phUser = (ACCOUNT_HANDLE)pAcctCtx;

cleanup:
    pAcctCtx = NULL;
    hUser    = NULL;

    return ntStatus;

error:
    *phUser = NULL;

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
