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
 *        lsa_deleteobject.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaDeleteObject function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
LsaSrvAccountContextDelete(
    PLSAR_ACCOUNT_CONTEXT pAccountCtx
    );


NTSTATUS
LsaRpcSrvDeleteObject(
    /* [in] */ handle_t hBinding,
    /* [out, context_handle] */ void **phObject
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_GENERIC_CONTEXT pContext = NULL;

    BAIL_ON_INVALID_PTR(phObject);
    BAIL_ON_INVALID_PTR(*phObject);

    pContext = (PLSA_GENERIC_CONTEXT)(*phObject);

    switch (pContext->Type)
    {
    case LsaContextPolicy:
        // It should not be possible to delete policy server
        ntStatus = STATUS_ACCESS_DENIED;
        break;

    case LsaContextAccount:
        ntStatus = LsaSrvAccountContextDelete(
                        (PLSAR_ACCOUNT_CONTEXT)pContext);
        break;

    default:
        /* Something is seriously wrong if we get a context
           we haven't created */
        ntStatus = STATUS_INTERNAL_ERROR;
    }

    *phObject = NULL;

error:
    return ntStatus;
}


static
NTSTATUS
LsaSrvAccountContextDelete(
    PLSAR_ACCOUNT_CONTEXT pAccountCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;

    err = LsaSrvPrivsMarkAccountDeleted(pAccountCtx->pAccountContext);
    if (err)
    {
        ntStatus = LwWin32ErrorToNtStatus(err);
    }

    LsaSrvAccountContextFree(pAccountCtx);

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
