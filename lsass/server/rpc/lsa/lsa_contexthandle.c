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
 *        lsa_contexthandle.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa context handles
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvPolicyContextClose(
    PPOLICY_CONTEXT  pPolCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pPolCtx);

    InterlockedDecrement(&pPolCtx->refcount);
    if (pPolCtx->refcount)
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    /*
     * Close local SAM domain handle
     */
    if (pPolCtx->hSamrBinding &&
        pPolCtx->hLocalDomain)
    {
        ntStatus = SamrClose(pPolCtx->hSamrBinding,
                             pPolCtx->hLocalDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        pPolCtx->hLocalDomain = NULL;
    }

    /*
     * Close builtin SAM domain handle
     */
    if (pPolCtx->hSamrBinding &&
        pPolCtx->hBuiltinDomain)
    {
        ntStatus = SamrClose(pPolCtx->hSamrBinding,
                             pPolCtx->hBuiltinDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        pPolCtx->hBuiltinDomain = NULL;
    }

    /*
     * Close SAM connection handle
     */
    if (pPolCtx->hSamrBinding &&
        pPolCtx->hConn)
    {
        ntStatus = SamrClose(pPolCtx->hSamrBinding,
                             pPolCtx->hConn);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        pPolCtx->hConn = NULL;
    }

    /*
     * Close samr rpc server binding
     */
    if (pPolCtx->hSamrBinding)
    {
        SamrFreeBinding(&pPolCtx->hSamrBinding);
        pPolCtx->hSamrBinding = NULL;
    }

    /*
     * Free domain connections cache (clean free)
     */
    if (pPolCtx->pDomains)
    {
        LsaSrvDestroyDomainsTable(pPolCtx->pDomains, TRUE);
        pPolCtx->pDomains = NULL;
    }

    LsaSrvPolicyContextFree(pPolCtx);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


VOID
LsaSrvPolicyContextFree(
    PPOLICY_CONTEXT  pPolCtx
    )
{
    /*
     * Free access token
     */
    LsaSrvFreeAuthInfo(pPolCtx);

    RTL_FREE(&pPolCtx->pLocalDomainSid);
    LW_SAFE_FREE_MEMORY(pPolCtx->pwszLocalDomainName);
    LW_SAFE_FREE_MEMORY(pPolCtx->pwszDomainName);
    RTL_FREE(&pPolCtx->pDomainSid);
    LW_SAFE_FREE_MEMORY(pPolCtx->pwszDcName);

    LW_SAFE_FREE_MEMORY(pPolCtx);
}


VOID
LsaSrvAccountContextFree(
    PLSAR_ACCOUNT_CONTEXT pAccountCtx
    )
{
    InterlockedDecrement(&pAccountCtx->refcount);
    if (pAccountCtx->refcount) return;

    LsaSrvPrivsCloseAccount(&pAccountCtx->pAccountContext);

    LsaSrvPolicyContextClose(pAccountCtx->pPolicyCtx);

    LW_SAFE_FREE_MEMORY(pAccountCtx);
}


void
POLICY_HANDLE_rundown(
    void *hContext
    )
{
    PPOLICY_CONTEXT pPolCtx = (PPOLICY_CONTEXT)hContext;

    /*
     * Free domain connections cache
     */
    if (pPolCtx->pDomains)
    {
        LsaSrvDestroyDomainsTable(pPolCtx->pDomains, FALSE);
    }

    LsaSrvPolicyContextFree(pPolCtx);
}


void
LSAR_ACCOUNT_HANDLE_rundown(
    void *hContext
    )
{
    PLSAR_ACCOUNT_CONTEXT pAccountCtx = (PLSAR_ACCOUNT_CONTEXT)hContext;

    LsaSrvAccountContextFree(pAccountCtx);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
