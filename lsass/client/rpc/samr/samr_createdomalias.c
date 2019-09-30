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
 *        samr_createdomalias.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrCreateDomAlias function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrCreateDomAlias(
    IN  SAMR_BINDING    hBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  PWSTR           pwszAliasName,
    IN  UINT32          AccessMask,
    OUT ACCOUNT_HANDLE *phAlias,
    OUT PUINT32         pRid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    UNICODE_STRING AliasName = {0};
    ACCOUNT_HANDLE hAlias = NULL;
    UINT32 Rid = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAliasName, ntStatus);
    BAIL_ON_INVALID_PTR(phAlias, ntStatus);
    BAIL_ON_INVALID_PTR(pRid, ntStatus);

    dwError = LwAllocateUnicodeStringFromWc16String(
                                    &AliasName,
                                    pwszAliasName);
    BAIL_ON_WIN_ERROR(dwError);

    DCERPC_CALL(ntStatus, cli_SamrCreateDomAlias((handle_t)hBinding,
                                                 hDomain,
                                                 &AliasName,
                                                 AccessMask,
                                                 &hAlias,
                                                 &Rid));
    BAIL_ON_NT_STATUS(ntStatus);

    *phAlias = hAlias;
    *pRid    = Rid;

cleanup:
    LwFreeUnicodeString(&AliasName);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (phAlias)
    {
        *phAlias = NULL;
    }

    if (pRid)
    {
        *pRid = 0;
    }

    goto cleanup;
}
