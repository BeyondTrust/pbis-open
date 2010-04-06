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
 * Abstract: Netlogon interface binding (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


RPCSTATUS
InitNetlogonBindingDefault(
    OUT handle_t            *phNetrBinding,
    IN  PCSTR                pszHostname,
    IN  LW_PIO_CREDS  pAccessToken,
    IN  BOOLEAN              bIsSchannel
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    PSTR pszProtSeq = NETLOGON_DEFAULT_PROT_SEQ;
    PSTR pszEndpoint = NETLOGON_DEFAULT_ENDPOINT;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    handle_t hNetrBinding = NULL;

    rpcStatus = InitNetlogonBindingFull(&hNetrBinding,
                                        pszProtSeq,
                                        pszHostname,
                                        pszEndpoint,
                                        pszUuid,
                                        pszOptions,
                                        pAccessToken,
                                        bIsSchannel);
    BAIL_ON_RPC_STATUS(rpcStatus);

    *phNetrBinding = hNetrBinding;

cleanup:
    return rpcStatus;

error:
    *phNetrBinding = NULL;

    goto cleanup;
}


RPCSTATUS
InitNetlogonBindingFull(
    OUT handle_t             *phNetrBinding,
    IN  PCSTR                 pszProtSeq,
    IN  PCSTR                 pszHostname,
    IN  PCSTR                 pszEndpoint,
    IN  PCSTR                 pszUuid,
    IN  PCSTR                 pszOptions,
    IN  LW_PIO_CREDS   pAccessToken,
    IN  BOOLEAN               bIsSchannel
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    RPCSTATUS rpcRetStatus = RPC_S_OK;
 unsigned char *binding_string = NULL;
    unsigned char *prot_seq       = NULL;
    unsigned char *endpoint       = NULL;
    unsigned char *uuid           = NULL;
    unsigned char *options        = NULL;
    unsigned char *address        = NULL;
    handle_t hNetrBinding = NULL;
    rpc_transport_info_handle_t info = NULL;

    BAIL_ON_INVALID_PTR_RPCSTATUS(phNetrBinding, rpcStatus);
    BAIL_ON_INVALID_PTR_RPCSTATUS(pszHostname, rpcStatus);
    BAIL_ON_INVALID_PTR_RPCSTATUS(pszProtSeq, rpcStatus);

    prot_seq = (unsigned char*) strdup(pszProtSeq);
    BAIL_ON_NO_MEMORY_RPCSTATUS(prot_seq, rpcStatus);

    if (pszEndpoint != NULL) {
        endpoint = (unsigned char*) strdup(pszEndpoint);
        BAIL_ON_NO_MEMORY_RPCSTATUS(endpoint, rpcStatus);
    }

    if (pszUuid != NULL) {
        uuid = (unsigned char*) strdup(pszUuid);
        BAIL_ON_NO_MEMORY_RPCSTATUS(uuid, rpcStatus);
    }

    if (pszOptions != NULL) {
        options = (unsigned char*) strdup(pszOptions);
        BAIL_ON_NO_MEMORY_RPCSTATUS(options, rpcStatus);
    }

    address = (unsigned char*) strdup(pszHostname);
    BAIL_ON_NO_MEMORY_RPCSTATUS(address, rpcStatus);

    rpc_string_binding_compose(uuid,
                               prot_seq,
                               address,
                               endpoint,
                               options,
                               &binding_string,
                               &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_binding_from_string_binding(binding_string, &hNetrBinding, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_smb_transport_info_from_lwio_creds(pAccessToken, bIsSchannel, &info, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_binding_set_transport_info(hNetrBinding, info, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

	info = NULL;

    rpc_mgmt_set_com_timeout(hNetrBinding, 6, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    *phNetrBinding = hNetrBinding;

cleanup:
    SAFE_FREE(prot_seq);
    SAFE_FREE(endpoint);
    SAFE_FREE(uuid);
    SAFE_FREE(options);
    SAFE_FREE(address);

    if (binding_string) {
        rpc_string_free(&binding_string, &rpcRetStatus);
    }

    if (rpcStatus == RPC_S_OK &&
        rpcRetStatus != RPC_S_OK) {
        rpcStatus = rpcRetStatus;
    }

    if (info)
    {
        rpc_smb_transport_info_free(info);
    }

    return rpcStatus;

error:
    if (hNetrBinding) {
        rpc_binding_free(&hNetrBinding, &rpcStatus);
    }

    goto cleanup;
}


RPCSTATUS
FreeNetlogonBinding(
    IN OUT handle_t *phNetrBinding
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;

    /* Free the binding itself */
    if (phNetrBinding && *phNetrBinding) {
        rpc_binding_free(phNetrBinding, &rpcStatus);
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
