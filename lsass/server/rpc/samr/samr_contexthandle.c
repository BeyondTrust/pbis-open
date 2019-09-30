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
 *        samr_contexthandle.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr context handles
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


VOID
SamrSrvConnectContextFree(
    PCONNECT_CONTEXT  pConnCtx
    )
{
    InterlockedDecrement(&pConnCtx->refcount);
    if (pConnCtx->refcount) return;

    if (pConnCtx->hDirectory)
    {
        DirectoryClose(pConnCtx->hDirectory);
    }

    SamrSrvFreeAuthInfo(pConnCtx);

    LW_SAFE_FREE_MEMORY(pConnCtx);
}


VOID
SamrSrvDomainContextFree(
    PDOMAIN_CONTEXT  pDomCtx
    )
{
    InterlockedDecrement(&pDomCtx->refcount);
    if (pDomCtx->refcount) return;

    RTL_FREE(&pDomCtx->pDomainSid);
    LW_SAFE_FREE_MEMORY(pDomCtx->pwszDomainName);
    LW_SAFE_FREE_MEMORY(pDomCtx->pwszDn);

    SamrSrvConnectContextFree(pDomCtx->pConnCtx);

    LW_SAFE_FREE_MEMORY(pDomCtx);
}


VOID
SamrSrvAccountContextFree(
    PACCOUNT_CONTEXT  pAcctCtx
    )
{
    InterlockedDecrement(&pAcctCtx->refcount);
    if (pAcctCtx->refcount) return;

    LW_SAFE_FREE_MEMORY(pAcctCtx->pwszDn);
    LW_SAFE_FREE_MEMORY(pAcctCtx->pwszName);
    RTL_FREE(&pAcctCtx->pSid);

    SamrSrvDomainContextFree(pAcctCtx->pDomCtx);

    LW_SAFE_FREE_MEMORY(pAcctCtx);
}


void
CONNECT_HANDLE_rundown(
    void *hContext
    )
{
    PCONNECT_CONTEXT pConnCtx = (PCONNECT_CONTEXT)hContext;
    SamrSrvConnectContextFree(pConnCtx);
}


void
DOMAIN_HANDLE_rundown(
    void *hContext
    )
{
    PDOMAIN_CONTEXT pDomCtx = (PDOMAIN_CONTEXT)hContext;
    SamrSrvDomainContextFree(pDomCtx);
}


void
ACCOUNT_HANDLE_rundown(
    void *hContext
    )
{
    PACCOUNT_CONTEXT pAcctCtx = (PACCOUNT_CONTEXT)hContext;
    SamrSrvAccountContextFree(pAcctCtx);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
