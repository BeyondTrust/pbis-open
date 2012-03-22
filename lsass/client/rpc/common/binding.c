/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
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
