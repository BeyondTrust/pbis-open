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
 *        netr_schannel.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Netlogon schannel functions.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Gerald Carter (gcarter@likewise.com)
 */


#include "includes.h"


NTSTATUS
NetrOpenSchannel(
    IN  NETR_BINDING      hNetrBinding,
    IN  PCWSTR            pwszMachineAccount,
    IN  PCWSTR            pwszHostname,
    IN  PCWSTR            pwszServer,
    IN  PCWSTR            pwszDomain,
    IN  PCWSTR            pwszFqdn,
    IN  PCWSTR            pwszComputer,
    IN  PCWSTR            pwszMachinePassword,
    IN  NetrCredentials  *pCreds,
    OUT PNETR_BINDING     phSchannelBinding
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    BYTE PassHash[16] = {0};
    BYTE CliChal[8] = {0};
    BYTE SrvChal[8] = {0};
    BYTE SrvCred[8] = {0};
    rpc_schannel_auth_info_t SchannelAuthInfo = {0};
    PIO_CREDS pIoCreds = NULL;
    NETR_BINDING hSchannelBinding = NULL;

    NetrGetNtHash(PassHash, pwszMachinePassword);

    if (!RAND_bytes((unsigned char*)CliChal, sizeof(CliChal)))
    {
        dwError = ERROR_ENCRYPTION_FAILED;
        BAIL_ON_WIN_ERROR(dwError);
    }

    ntStatus = NetrServerReqChallenge(hNetrBinding,
                                      pwszServer,
                                      pwszComputer,
                                      CliChal,
                                      SrvChal);
    BAIL_ON_NT_STATUS(ntStatus);

    NetrCredentialsInit(pCreds,
                        CliChal,
                        SrvChal,
                        PassHash,
                        NETLOGON_NET_ADS_FLAGS);

    ntStatus = NetrServerAuthenticate2(hNetrBinding,
                                       pwszServer,
                                       pwszMachineAccount,
                                       pCreds->channel_type,
                                       pwszComputer,
                                       pCreds->cli_chal.data,
                                       SrvCred,
                                       &pCreds->negotiate_flags);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!NetrCredentialsCorrect(pCreds, SrvCred))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(SchannelAuthInfo.session_key,
           pCreds->session_key,
           16);

    dwError = LwWc16sToMbs(pwszDomain,
                           (PSTR*)&SchannelAuthInfo.domain_name);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszFqdn,
                           (PSTR*)&SchannelAuthInfo.fqdn);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszComputer,
                           (PSTR*)&SchannelAuthInfo.machine_name);
    BAIL_ON_WIN_ERROR(dwError);

    SchannelAuthInfo.sender_flags = rpc_schn_initiator_flags;

    ntStatus = LwIoGetActiveCreds(NULL, &pIoCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrInitBindingDefault(&hSchannelBinding,
                                      pwszHostname,
                                      pIoCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    rpc_binding_set_auth_info(hSchannelBinding,
                              NULL,
#if 0
                              /* Helps to debug network traces */
                              rpc_c_authn_level_pkt_integrity,
#else
                              /* Secure */
                              rpc_c_authn_level_pkt_privacy,
#endif
                              rpc_c_authn_schannel,
                              (rpc_auth_identity_handle_t)&SchannelAuthInfo,
                              rpc_c_authz_name, /* authz_protocol */
                              &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_mgmt_set_com_timeout(hSchannelBinding, 5, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    *phSchannelBinding = hSchannelBinding;

cleanup:
    LW_SAFE_FREE_MEMORY(SchannelAuthInfo.domain_name);
    LW_SAFE_FREE_MEMORY(SchannelAuthInfo.machine_name);
    LW_SAFE_FREE_MEMORY(SchannelAuthInfo.fqdn);

    if (pIoCreds)
    {
        LwIoDeleteCreds(pIoCreds);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (hSchannelBinding)
    {
        NetrFreeBinding(&hSchannelBinding);
    }

    if (phSchannelBinding)
    {
        *phSchannelBinding = NULL;
    }

    goto cleanup;
}


VOID
NetrCloseSchannel(
    IN NETR_BINDING  hSchannelBinding
    )
{
    if (hSchannelBinding)
    {
        NetrFreeBinding(&hSchannelBinding);
    }
}
