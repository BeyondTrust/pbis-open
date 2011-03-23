/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        lsa_lookupprivilegename.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaLookupPrivilegeName function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaLookupPrivilegeName(
    IN  LSA_BINDING      hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  PLUID            pValue,
    OUT PWSTR           *ppwszName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUNICODE_STRING pName = NULL;
    PWSTR pwszName = NULL;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hPolicy, ntStatus);
    BAIL_ON_INVALID_PTR(pValue, ntStatus);
    BAIL_ON_INVALID_PTR(ppwszName, ntStatus);

    DCERPC_CALL(ntStatus, cli_LsaLookupPrivilegeName(
                              (handle_t)hBinding,
                              hPolicy,
                              pValue,
                              &pName));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                     OUT_PPVOID(&pwszName),
                     pName->Length + sizeof(pName->Buffer[0]));
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pwszName, pName->Buffer, pName->Length);

    *ppwszName = pwszName;

error:
    if (ntStatus)
    {
        if (ppwszName)
        {
            *ppwszName = NULL;
        }
    }

    if (pName)
    {
        LsaFreeStubUnicodeString(pName);
    }

    return ntStatus;
}
