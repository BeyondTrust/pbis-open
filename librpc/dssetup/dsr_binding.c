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
 *        dsr_binding.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsSetup dcerpc binding functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


RPCSTATUS
InitDsrBindingDefault(
    handle_t *phBinding,
    PCSTR pszHostname,
    PIO_CREDS pIoAccessToken
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    PSTR pszProtSeq = (PSTR)DSR_DEFAULT_PROT_SEQ;
    PSTR pszEndpoint = (PSTR)DSR_DEFAULT_ENDPOINT;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    handle_t hBinding = NULL;

    rpcStatus = InitDsrBindingFull(
                    &hBinding,
                    pszProtSeq,
                    pszHostname,
                    pszEndpoint,
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
InitDsrBindingFull(
    handle_t *phBinding,
    PCSTR pszProtSeq,
    PCSTR pszHostname,
    PCSTR pszEndpoint,
    PCSTR pszUuid,
    PCSTR pszOptions,
    PIO_CREDS pIoAccessToken
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    RPCSTATUS st = RPC_S_OK;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pbBindingString = NULL;
    PBYTE pbProtSeq   = NULL;
    PBYTE pbEndpoint   = NULL;
    PBYTE pbUuid    = NULL;
    PBYTE pbOptions = NULL;
    PBYTE pbAddress = NULL;
    handle_t hBinding = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;

    BAIL_ON_INVALID_PTR_RPCSTATUS(phBinding, rpcStatus);
    BAIL_ON_INVALID_PTR_RPCSTATUS(pszProtSeq, rpcStatus);

    ntStatus = RtlCStringDuplicate((PSTR*)&pbProtSeq, pszProtSeq);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pszEndpoint != NULL)
    {
        ntStatus = RtlCStringDuplicate((PSTR*)&pbEndpoint, pszEndpoint);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pszUuid != NULL)
    {
        ntStatus = RtlCStringDuplicate((PSTR*)&pbUuid, pszUuid);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pszOptions != NULL)
    {
        ntStatus = RtlCStringDuplicate((PSTR*)&pbOptions, pszOptions);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RtlCStringDuplicate((PSTR*)&pbAddress, pszHostname);
    BAIL_ON_NT_STATUS(ntStatus);

    rpc_string_binding_compose(
        pbUuid,
        pbProtSeq,
        pbAddress,
        pbEndpoint,
        pbOptions,
        &pbBindingString,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_binding_from_string_binding(
        pbBindingString,
        &hBinding,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_smb_transport_info_from_lwio_creds(
        pIoAccessToken,
        FALSE,
        &hTransportInfo,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_binding_set_transport_info(
        hBinding,
        hTransportInfo,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    hTransportInfo = NULL;

    rpc_mgmt_set_com_timeout(hBinding, 6, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    *phBinding = hBinding;

cleanup:
    RtlCStringFree((PSTR*)&pbProtSeq);
    RtlCStringFree((PSTR*)&pbEndpoint);
    RtlCStringFree((PSTR*)&pbUuid);
    RtlCStringFree((PSTR*)&pbOptions);
    RtlCStringFree((PSTR*)&pbAddress);

    if (pbBindingString)
    {
        rpc_string_free(&pbBindingString, &st);
    }

    if ((rpcStatus == RPC_S_OK) && (st != RPC_S_OK))
    {
        rpcStatus = st;
    }

    if (hTransportInfo)
    {
        rpc_smb_transport_info_free(hTransportInfo);
    }

    return rpcStatus;

error:
    if (ntStatus != STATUS_SUCCESS)
    {
        /* We really need an NTSTATUS -> RPCSTATUS conversion
           function here, but since the only NTSTATUS that can
           occur is a memory related one, weare ok for the moment */

        switch(ntStatus)
        {
        case STATUS_INVALID_PARAMETER:
            rpcStatus = RPC_S_INVALID_ARG;
            break;
        case STATUS_INSUFFICIENT_RESOURCES:
            rpcStatus = RPC_S_OUT_OF_MEMORY;
            break;
        default:
            rpcStatus = RPC_S_UNKNOWN_STATUS_CODE;
            break;
        }
    }

    if (hBinding)
    {
        rpc_binding_free(&hBinding, &st);
    }

    goto cleanup;
}


RPCSTATUS
FreeDsrBinding(
    handle_t *phBinding
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
