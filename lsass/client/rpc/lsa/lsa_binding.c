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
 *        lsa_binding.c
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
LsaInitBindingDefaultA(
    OUT PLSA_BINDING   phBinding,
    IN  PCSTR          pszHostname,
    IN  PIO_CREDS      pCreds
    );


static
NTSTATUS
LsaInitBindingFullA(
    OUT PLSA_BINDING   phBinding,
    IN  PCSTR          pszProtSeq,
    IN  PCSTR          pszHostname,
    IN  PCSTR          pszEndpoint,
    IN  PCSTR          pszUuid,
    IN  PCSTR          pszOptions,
    IN  PIO_CREDS      pCreds
    );


NTSTATUS
LsaInitBindingDefault(
    OUT PLSA_BINDING   phBinding,
    IN  PCWSTR         pwszHostname,
    IN  PIO_CREDS      pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszHostname = NULL;

    if (pwszHostname)
    {
        dwError = LwWc16sToMbs(pwszHostname, &pszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    ntStatus = LsaInitBindingDefaultA(phBinding,
                                      pszHostname,
                                      pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LW_SAFE_FREE_MEMORY(pszHostname);

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaInitBindingDefaultA(
    OUT PLSA_BINDING   phBinding,
    IN  PCSTR          pszHostname,
    IN  PIO_CREDS      pCreds
    )
{
    NTSTATUS ntStatus = RPC_S_OK;
    PSTR pszProtSeq = (PSTR)LSA_DEFAULT_PROT_SEQ;
    PSTR pszLpcProtSeq = (PSTR)"ncalrpc";
    PSTR pszEndpoint = (PSTR)LSA_DEFAULT_ENDPOINT;
    PSTR pszLpcEndpoint = (PSTR)LSA_LOCAL_ENDPOINT;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    LSA_BINDING hBinding = NULL;

    BAIL_ON_INVALID_PTR(phBinding, ntStatus);

    ntStatus = LsaInitBindingFullA(
                    &hBinding,
                    (pszHostname) ? pszProtSeq : pszLpcProtSeq,
                    pszHostname,
                    (pszHostname) ? pszEndpoint : pszLpcEndpoint,
                    pszUuid,
                    pszOptions,
                    pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    *phBinding = hBinding;

cleanup:
    return ntStatus;

error:
    if (phBinding)
    {
        *phBinding = NULL;
    }

    goto cleanup;
}


NTSTATUS
LsaInitBindingFull(
    OUT PLSA_BINDING   phBinding,
    IN  PCWSTR         pwszProtSeq,
    IN  PCWSTR         pwszHostname,
    IN  PCWSTR         pwszEndpoint,
    IN  PCWSTR         pwszUuid,
    IN  PCWSTR         pwszOptions,
    IN  PIO_CREDS      pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszProtSeq = NULL;
    PSTR pszHostname = NULL;
    PSTR pszEndpoint = NULL;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    LSA_BINDING hBinding = NULL;

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

    ntStatus = LsaInitBindingFullA(&hBinding,
                                   pszProtSeq,
                                   pszHostname,
                                   pszEndpoint,
                                   pszUuid,
                                   pszOptions,
                                   pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    *phBinding = hBinding;

cleanup:
    LW_SAFE_FREE_MEMORY(pszProtSeq);
    LW_SAFE_FREE_MEMORY(pszHostname);
    LW_SAFE_FREE_MEMORY(pszEndpoint);
    LW_SAFE_FREE_MEMORY(pszUuid);
    LW_SAFE_FREE_MEMORY(pszOptions);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (phBinding)
    {
        *phBinding = NULL;
    }

    goto cleanup;
}


static
NTSTATUS
LsaInitBindingFullA(
    OUT PLSA_BINDING   phBinding,
    IN  PCSTR          pszProtSeq,
    IN  PCSTR          pszHostname,
    IN  PCSTR          pszEndpoint,
    IN  PCSTR          pszUuid,
    IN  PCSTR          pszOptions,
    IN  PIO_CREDS      pCreds
    )
{
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

    *phBinding = (LSA_BINDING)hBinding;

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

    return ntStatus;

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
LsaFreeBinding(
    IN OUT PLSA_BINDING  phBinding
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
