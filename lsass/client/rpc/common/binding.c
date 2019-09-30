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
 *        binding.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DCE/RPC binding functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
RpcInitBindingFromBindingStringA(
    OUT handle_t      *phBinding,
    IN  PCSTR          pszBindingString,
    IN  LW_PIO_CREDS   pCreds
    );


NTSTATUS
RpcInitBindingFromBindingString(
    OUT handle_t      *phBinding,
    IN  PCWSTR         pwszBindingString,
    IN  LW_PIO_CREDS   pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszBindingString = NULL;

    if (pwszBindingString)
    {
        dwError = LwWc16sToMbs(pwszBindingString,
                               &pszBindingString);
        BAIL_ON_WIN_ERROR(dwError);
    }

    ntStatus = RpcInitBindingFromBindingStringA(
                                      phBinding,
                                      pszBindingString,
                                      pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LW_SAFE_FREE_MEMORY(pszBindingString);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
RpcInitBindingFromBindingStringA(
    OUT handle_t      *phBinding,
    IN  PCSTR          pszBindingString,
    IN  LW_PIO_CREDS   pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    unsigned32 rpcStatus2 = RPC_S_OK;
    handle_t hBinding = NULL;
    rpc_transport_info_handle_t hInfo = NULL;
    unsigned char *pbBindingString = NULL;

    BAIL_ON_INVALID_PTR(phBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pszBindingString, ntStatus);

    pbBindingString = (unsigned char*)strdup(pszBindingString);
    if (pbBindingString == NULL)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    rpc_binding_from_string_binding(
        pbBindingString,
        &hBinding,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    if (LwCaselessStringSearch(pszBindingString, "ncacn_np") &&
        pCreds != NULL)
    {
        rpc_smb_transport_info_from_lwio_creds(
            pCreds,
            &hInfo,
            &rpcStatus);
        BAIL_ON_RPC_STATUS(rpcStatus);

        rpc_binding_set_transport_info(
            hBinding,
            hInfo,
            &rpcStatus);
        BAIL_ON_RPC_STATUS(rpcStatus);

        hInfo = NULL;
    }

    rpc_mgmt_set_com_timeout(hBinding, 6, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    *phBinding = hBinding;

cleanup:
    LW_SAFE_FREE_MEMORY(pbBindingString);

    if (hInfo)
    {
        rpc_smb_transport_info_free(hInfo);
    }

    if (ntStatus == STATUS_SUCCESS &&
        rpcStatus != RPC_S_OK)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
    }

    return ntStatus;

error:
    if (hBinding)
    {
        rpc_binding_free(&hBinding, &rpcStatus2);
    }

    *phBinding = NULL;

    goto cleanup;
}
