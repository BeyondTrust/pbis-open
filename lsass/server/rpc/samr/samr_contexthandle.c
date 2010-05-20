/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
