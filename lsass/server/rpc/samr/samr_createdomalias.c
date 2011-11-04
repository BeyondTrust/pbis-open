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
 *        samr_createdomalias2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrCreateDomAlias function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvCreateDomAlias(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UNICODE_STRING *pAliasName,
    /* [in] */ UINT32 dwAccessMask,
    /* [out] */ ACCOUNT_HANDLE *phAlias,
    /* [out] */ UINT32 *pdwRid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PWSTR pwszAliasName = NULL;
    UNICODE_STRING Name = {0};
    UINT32 ulAccessGranted = 0;

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pDomCtx->dwAccessGranted & DOMAIN_ACCESS_CREATE_ALIAS))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = SamrSrvGetFromUnicodeString(&pwszAliasName,
                                           pAliasName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvInitUnicodeStringEx(&Name,
                                          pwszAliasName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvCreateAccount(hBinding,
                                    hDomain,
                                    &Name,
                                    DS_OBJECT_CLASS_LOCAL_GROUP,
                                    0,
                                    dwAccessMask,
                                    phAlias,
                                    &ulAccessGranted,
                                    pdwRid);
    if (ntStatus == STATUS_USER_EXISTS)
    {
        ntStatus = STATUS_ALIAS_EXISTS;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

cleanup:
    if (pwszAliasName)
    {
        SamrSrvFreeMemory(pwszAliasName);
    }

    SamrSrvFreeUnicodeStringEx(&Name);

    return ntStatus;

error:
    *phAlias = NULL;
    *pdwRid  = 0;
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
