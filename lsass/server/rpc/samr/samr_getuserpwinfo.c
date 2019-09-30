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
 *        samr_getuserpwinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrGetUserPwInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvGetUserPwInfo(
    IN  handle_t        hBinding,
    IN  ACCOUNT_HANDLE  hUser,
    OUT PwInfo         *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;

    BAIL_ON_INVALID_PTR(hBinding);
    BAIL_ON_INVALID_PTR(hUser);
    BAIL_ON_INVALID_PTR(pInfo);

    pAcctCtx = (PACCOUNT_CONTEXT)hUser;
    pDomCtx  = pAcctCtx->pDomCtx;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /* Check access rights required */
    if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_ATTRIBUTES))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * This is in fact returning domain- not user-specific
     * settings
     */
    pInfo->min_password_length = pDomCtx->dwMinPasswordLen;
    pInfo->password_properties = pDomCtx->dwPasswordProperties;

cleanup:
    return ntStatus;

error:
    pInfo->min_password_length = 0;
    pInfo->password_properties = 0;

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
