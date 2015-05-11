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
 *        wkss_binding.c
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
WINERROR
WkssInitBindingDefaultA(
    OUT PWKSS_BINDING  phBinding,
    IN  PCSTR          pszHostname,
    IN  PIO_CREDS      pCreds
    );


static
WINERROR
WkssInitBindingFullA(
    OUT PWKSS_BINDING  phBinding,
    IN  PCSTR          pszProtSeq,
    IN  PCSTR          pszHostname,
    IN  PCSTR          pszEndpoint,
    IN  PCSTR          pszUuid,
    IN  PCSTR          pszOptions,
    IN  PIO_CREDS      pCreds
    );


WINERROR
WkssInitBindingDefault(
    OUT PWKSS_BINDING   phBinding,
    IN  PCWSTR          pwszHostname,
    IN  PIO_CREDS       pCreds
    )
{
    WINERROR winError = ERROR_SUCCESS;
    PSTR pszHostname = NULL;

    if (pwszHostname)
    {
        winError = LwWc16sToMbs(pwszHostname, &pszHostname);
        BAIL_ON_WIN_ERROR(winError);
    }

    winError = WkssInitBindingDefaultA(phBinding,
                                       pszHostname,
                                       pCreds);
    BAIL_ON_NT_STATUS(winError);

cleanup:
    LW_SAFE_FREE_MEMORY(pszHostname);

    return winError;

error:
    goto cleanup;
}


static
WINERROR
WkssInitBindingDefaultA(
    OUT PWKSS_BINDING   phBinding,
    IN  PCSTR           pszHostname,
    IN  PIO_CREDS       pCreds
    )
{
    WINERROR winError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszProtSeq = (PSTR)WKSS_DEFAULT_PROT_SEQ;
    PSTR pszLpcProtSeq = (PSTR)"ncalrpc";
    PSTR pszEndpoint = (PSTR)WKSS_DEFAULT_ENDPOINT;
    PSTR pszLpcEndpoint = (PSTR)WKSS_LOCAL_ENDPOINT;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    WKSS_BINDING hBinding = NULL;

    BAIL_ON_INVALID_PTR(phBinding, ntStatus);

    winError = WkssInitBindingFullA(
                    &hBinding,
                    (pszHostname) ? pszProtSeq : pszLpcProtSeq,
                    pszHostname,
                    (pszHostname) ? pszEndpoint : pszLpcEndpoint,
                    pszUuid,
                    pszOptions,
                    pCreds);
    BAIL_ON_WIN_ERROR(winError);

    *phBinding = hBinding;

cleanup:
    return winError;

error:
    if (phBinding)
    {
        *phBinding = NULL;
    }

    goto cleanup;
}


WINERROR
WkssInitBindingFull(
    OUT PWKSS_BINDING   phBinding,
    IN  PCWSTR          pwszProtSeq,
    IN  PCWSTR          pwszHostname,
    IN  PCWSTR          pwszEndpoint,
    IN  PCWSTR          pwszUuid,
    IN  PCWSTR          pwszOptions,
    IN  PIO_CREDS       pCreds
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszProtSeq = NULL;
    PSTR pszHostname = NULL;
    PSTR pszEndpoint = NULL;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    WKSS_BINDING hBinding = NULL;

    BAIL_ON_INVALID_PTR(phBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszProtSeq, ntStatus);

    dwError = LwWc16sToMbs(pwszProtSeq, &pszProtSeq);
    BAIL_ON_WIN_ERROR(dwError);

    if (pwszHostname)
    {
        dwError = LwWc16sToMbs(pwszHostname, &pszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(pwszEndpoint, &pszEndpoint);
    BAIL_ON_WIN_ERROR(dwError);

    if (pwszUuid)
    {
        dwError = LwWc16sToMbs(pwszUuid, &pszUuid);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (pwszOptions)
    {
        dwError = LwWc16sToMbs(pwszOptions, &pszOptions);
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = WkssInitBindingFullA(&hBinding,
                                   pszProtSeq,
                                   pszHostname,
                                   pszEndpoint,
                                   pszUuid,
                                   pszOptions,
                                   pCreds);
    BAIL_ON_WIN_ERROR(dwError);

    *phBinding = hBinding;

cleanup:
    LW_SAFE_FREE_MEMORY(pszProtSeq);
    LW_SAFE_FREE_MEMORY(pszHostname);
    LW_SAFE_FREE_MEMORY(pszEndpoint);
    LW_SAFE_FREE_MEMORY(pszUuid);
    LW_SAFE_FREE_MEMORY(pszOptions);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;

error:
    if (phBinding)
    {
        *phBinding = NULL;
    }

    goto cleanup;
}


static
WINERROR
WkssInitBindingFullA(
    OUT PWKSS_BINDING  phBinding,
    IN  PCSTR          pszProtSeq,
    IN  PCSTR          pszHostname,
    IN  PCSTR          pszEndpoint,
    IN  PCSTR          pszUuid,
    IN  PCSTR          pszOptions,
    IN  PIO_CREDS      pCreds
    )
{
    WINERROR winError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    unsigned32 rpcStatus2 = RPC_S_OK;
    PBYTE pbBindingString = NULL;
    PBYTE pbProtSeq = NULL;
    PBYTE pbEndpoint = NULL;
    PBYTE pbUuid = NULL;
    PBYTE pbOpts = NULL;
    PBYTE pbAddr = NULL;
    handle_t hBinding = NULL;
    rpc_transport_info_handle_t hInfo = NULL;

    BAIL_ON_INVALID_PTR(phBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pszProtSeq, ntStatus);

    pbProtSeq = (PBYTE)strdup(pszProtSeq);
    BAIL_ON_NULL_PTR(pbProtSeq, ntStatus);

    if (pszEndpoint != NULL)
    {
        pbEndpoint = (PBYTE) strdup(pszEndpoint);
        BAIL_ON_NULL_PTR(pbEndpoint, ntStatus);
    }

    if (pszUuid != NULL)
    {
        pbUuid = (PBYTE)strdup(pszUuid);
        BAIL_ON_NULL_PTR(pbUuid, ntStatus);
    }

    if (pszOptions != NULL)
    {
        pbOpts = (PBYTE)strdup(pszOptions);
        BAIL_ON_NULL_PTR(pbOpts, ntStatus);
    }

    if (pszHostname)
    {
        pbAddr = (PBYTE)strdup(pszHostname);
        BAIL_ON_NULL_PTR(pbAddr, ntStatus);
    }

    rpc_string_binding_compose(
        pbUuid,
        pbProtSeq,
        pbAddr,
        pbEndpoint,
        pbOpts,
        &pbBindingString,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_binding_from_string_binding(
        pbBindingString,
        &hBinding,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    if (strcmp(pszProtSeq, "ncacn_np") == 0)
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

    *phBinding = (WKSS_BINDING)hBinding;

cleanup:
    LW_SAFE_FREE_MEMORY(pbProtSeq);
    LW_SAFE_FREE_MEMORY(pbEndpoint);
    LW_SAFE_FREE_MEMORY(pbUuid);
    LW_SAFE_FREE_MEMORY(pbOpts);
    LW_SAFE_FREE_MEMORY(pbAddr);

    if (pbBindingString)
    {
        rpc_string_free(&pbBindingString, &rpcStatus2);
    }

    if ((rpcStatus == RPC_S_OK) && (rpcStatus2 != RPC_S_OK))
    {
        rpcStatus = rpcStatus2;
    }

    if (hInfo)
    {
        rpc_smb_transport_info_free(hInfo);
    }

    if (ntStatus == STATUS_SUCCESS &&
        rpcStatus != RPC_S_OK)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
    }

    if (ntStatus != STATUS_SUCCESS)
    {
        winError = LwNtStatusToWin32Error(ntStatus);
    }

    return winError;

error:
    if (hBinding)
    {
        rpc_binding_free(&hBinding, &rpcStatus2);
    }

    if (phBinding)
    {
        *phBinding = NULL;
    }

    goto cleanup;
}


VOID
WkssFreeBinding(
    IN OUT PWKSS_BINDING  phBinding
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    /* Free the binding itself */

    if (phBinding && *phBinding)
    {
	    rpc_binding_free((handle_t*)phBinding, &rpcStatus);
        BAIL_ON_RPC_STATUS(rpcStatus);

        *phBinding = NULL;
    }

cleanup:
    return;

error:
    goto cleanup;
}
