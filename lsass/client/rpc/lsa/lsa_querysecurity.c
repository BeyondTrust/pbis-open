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
 *        lsa_querysecurity.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaQuerySecurity function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaQuerySecurity(
    IN  LSA_BINDING                    hBinding,
    IN  void                          *hObject,
    IN  DWORD                          SecurityInfo,
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppSecDesc,
    OUT PDWORD                         pSecDescLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_SECURITY_DESCRIPTOR_BUFFER pSecurityDescBuffer = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDesc = NULL;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hObject, ntStatus);
    BAIL_ON_INVALID_PTR(ppSecDesc, ntStatus);
    BAIL_ON_INVALID_PTR(pSecDescLen, ntStatus);

    DCERPC_CALL(ntStatus, cli_LsaQuerySecurity(
                              (handle_t)hBinding,
                              hObject,
                              SecurityInfo,
                              &pSecurityDescBuffer));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaAllocateSecurityDescriptor(
                              &pSecurityDesc,
                              pSecurityDescBuffer);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSecDesc   = pSecurityDesc;
    *pSecDescLen = pSecurityDescBuffer->BufferLen;

cleanup:
    if (pSecurityDescBuffer)
    {
        LsaFreeStubSecurityDescriptorBuffer(pSecurityDescBuffer);
    }

    return ntStatus;

error:
    if (pSecurityDesc)
    {
        LsaRpcFreeMemory(pSecurityDesc);
    }

    if (ppSecDesc)
    {
        *ppSecDesc = NULL;
    }

    if (pSecDescLen)
    {
        *pSecDescLen = 0;
    }

    goto cleanup;
}
