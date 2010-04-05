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


void
POLICY_HANDLE_rundown(
    void *hContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPOLICY_CONTEXT pPolCtx = (PPOLICY_CONTEXT)hContext;

    InterlockedDecrement(&pPolCtx->refcount);
    if (pPolCtx->refcount > 1) return;

    /*
     * Close local SAM domain handle
     */
    if (pPolCtx->bCleanClose &&
        pPolCtx->hSamrBinding &&
        pPolCtx->hLocalDomain)
    {
        ntStatus = SamrClose(pPolCtx->hSamrBinding,
                             pPolCtx->hLocalDomain);

        pPolCtx->hLocalDomain = NULL;
    }

    /*
     * Close builtin SAM domain handle and close
     * the samr rpc server connection
     */
    if (pPolCtx->bCleanClose &&
        pPolCtx->hSamrBinding &&
        pPolCtx->hBuiltinDomain)
    {
        ntStatus = SamrClose(pPolCtx->hSamrBinding,
                             pPolCtx->hBuiltinDomain);

        pPolCtx->hBuiltinDomain = NULL;
    }

    if (pPolCtx->bCleanClose &&
        pPolCtx->hSamrBinding &&
        pPolCtx->hConn)
    {
        ntStatus = SamrClose(pPolCtx->hSamrBinding,
                             pPolCtx->hConn);

        pPolCtx->hConn = NULL;
    }

    if (pPolCtx->bCleanClose &&
        pPolCtx->hSamrBinding)
    {
        FreeSamrBinding(&pPolCtx->hSamrBinding);
        pPolCtx->hSamrBinding = NULL;
    }

    /*
     * Free domain connections cache
     */
    if (pPolCtx->pDomains)
    {
        LsaSrvDestroyDomainsTable(pPolCtx->pDomains,
                                  pPolCtx->bCleanClose);
        pPolCtx->pDomains = NULL;
    }

    pPolCtx->bCleanClose = FALSE;

    /*
     * Free access token
     */
    LsaSrvFreeAuthInfo(pPolCtx);

    if (pPolCtx->refcount) return;

    RTL_FREE(&pPolCtx->pLocalDomainSid);
    LW_SAFE_FREE_MEMORY(pPolCtx->pwszLocalDomainName);
    LW_SAFE_FREE_MEMORY(pPolCtx->pwszDomainName);
    RTL_FREE(&pPolCtx->pDomainSid);
    LW_SAFE_FREE_MEMORY(pPolCtx->pwszDcName);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
