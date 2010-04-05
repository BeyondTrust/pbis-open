/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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


RPCSTATUS
InitLsaBindingDefault(
    OUT handle_t  *phBinding,
    IN  PCSTR      pszHostname,
    IN  PIO_CREDS  pIoAccessToken
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    PSTR pszProtSeq = (PSTR)LSA_DEFAULT_PROT_SEQ;
    PSTR pszLpcProtSeq = (PSTR)"ncalrpc";
    PSTR pszEndpoint = (PSTR)LSA_DEFAULT_ENDPOINT;
    PSTR pszLpcEndpoint = (PSTR)LSA_LOCAL_ENDPOINT;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    handle_t hBinding = NULL;

    rpcStatus = InitLsaBindingFull(
                    &hBinding,
                    (pszHostname) ? pszProtSeq : pszLpcProtSeq,
                    pszHostname,
                    (pszHostname) ? pszEndpoint : pszLpcEndpoint,
                    pszUuid,
                    pszOptions,
                    pIoAccessToken);
    BAIL_ON_RPC_STATUS(rpcStatus);

    *phBinding = hBinding;

cleanup:
    return rpcStatus;

error:
    *phBinding = NULL;
    goto cleanup;
}


RPCSTATUS
InitLsaBindingFull(
    OUT handle_t  *phBinding,
    IN  PCSTR      pszProtSeq,
    IN  PCSTR      pszHostname,
    IN  PCSTR      pszEndpoint,
    IN  PCSTR      pszUuid,
    IN  PCSTR      pszOptions,
    IN  PIO_CREDS  pIoAccessToken
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    RPCSTATUS rpcStatus2 = RPC_S_OK;
    PBYTE pbBindingString = NULL;
    PBYTE pbProtSeq = NULL;
    PBYTE pbEndpoint = NULL;
    PBYTE pbUuid = NULL;
    PBYTE pbOpts = NULL;
    PBYTE pbAddr = NULL;
    handle_t hBinding = NULL;
    rpc_transport_info_handle_t hInfo = NULL;

    BAIL_ON_INVALID_PTR_RPCSTATUS(phBinding, rpcStatus);
    BAIL_ON_INVALID_PTR_RPCSTATUS(pszProtSeq, rpcStatus);

    pbProtSeq = (PBYTE)strdup(pszProtSeq);
    BAIL_ON_NO_MEMORY_RPCSTATUS(pbProtSeq, rpcStatus);

    if (pszEndpoint != NULL) {
        pbEndpoint = (PBYTE) strdup(pszEndpoint);
        BAIL_ON_NO_MEMORY_RPCSTATUS(pbEndpoint, rpcStatus);
    }

    if (pszUuid != NULL) {
        pbUuid = (PBYTE)strdup(pszUuid);
        BAIL_ON_NO_MEMORY_RPCSTATUS(pbUuid, rpcStatus);
    }

    if (pszOptions != NULL) {
        pbOpts = (PBYTE)strdup(pszOptions);
        BAIL_ON_NO_MEMORY_RPCSTATUS(pbOpts, rpcStatus);
    }

    if (pszHostname) {
        pbAddr = (PBYTE)strdup(pszHostname);
        BAIL_ON_NO_MEMORY_RPCSTATUS(pbAddr, rpcStatus);
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
            pIoAccessToken,
            FALSE,
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
    SAFE_FREE(pbProtSeq);
    SAFE_FREE(pbEndpoint);
    SAFE_FREE(pbUuid);
    SAFE_FREE(pbOpts);
    SAFE_FREE(pbAddr);

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

    return rpcStatus;

error:
    if (hBinding)
    {
        rpc_binding_free(&hBinding, &rpcStatus2);
    }

    goto cleanup;
}


RPCSTATUS
FreeLsaBinding(
    IN  handle_t *phBinding
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;

    /* Free the binding itself */

    if (phBinding && *phBinding)
    {
	    rpc_binding_free(phBinding, &rpcStatus);
        BAIL_ON_RPC_STATUS(rpcStatus);
    }

cleanup:
    return rpcStatus;

error:
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
