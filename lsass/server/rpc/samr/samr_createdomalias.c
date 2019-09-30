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
