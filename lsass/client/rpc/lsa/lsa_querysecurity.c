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
