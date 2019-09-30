/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsa_accesstoken.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Access token handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
LsaSrvInitNpAuthInfo(
    IN  rpc_transport_info_handle_t hTransportInfo,
    OUT PPOLICY_CONTEXT             pPolCtx
    );

NTSTATUS
LsaSrvInitAuthInfo(
    IN  handle_t          hBinding,
    OUT PPOLICY_CONTEXT   pPolCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    unsigned32 rpcStatus = 0;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    DWORD dwProtSeq = rpc_c_invalid_protseq_id;

    rpc_binding_inq_access_token_caller(
        hBinding,
        &pPolCtx->pUserToken,
        &rpcStatus);

    ntStatus = LwRpcStatusToNtStatus(rpcStatus);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);
     
    rpc_binding_inq_transport_info(hBinding,
                                   &hTransportInfo,
                                   &rpcStatus);

    ntStatus = LwRpcStatusToNtStatus(rpcStatus);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (hTransportInfo)
    {
        rpc_binding_inq_prot_seq(hBinding,
                                 (unsigned32*)&dwProtSeq,
                                 &rpcStatus);
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        switch (dwProtSeq)
        {
        case rpc_c_protseq_id_ncacn_np:
            ntStatus = LsaSrvInitNpAuthInfo(hTransportInfo,
                                             pPolCtx);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
            break;
        }
    }

cleanup:
    return ntStatus;

error:
    LsaSrvFreeAuthInfo(pPolCtx);

    goto cleanup;
}


static
NTSTATUS
LsaSrvInitNpAuthInfo(
    IN  rpc_transport_info_handle_t hTransportInfo,
    OUT PPOLICY_CONTEXT             pPolCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PUCHAR pucSessionKey = NULL;
    unsigned16 SessionKeyLen = 0;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLen = 0;

    rpc_smb_transport_info_inq_session_key(
                                   hTransportInfo,
                                   (unsigned char**)&pucSessionKey,
                                   &SessionKeyLen);

    dwSessionKeyLen = SessionKeyLen;
    if (dwSessionKeyLen)
    {
        dwError = LwAllocateMemory(dwSessionKeyLen,
                                   OUT_PPVOID(&pSessionKey));
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pSessionKey, pucSessionKey, dwSessionKeyLen);
    }

    pPolCtx->pSessionKey     = pSessionKey;
    pPolCtx->dwSessionKeyLen = dwSessionKeyLen;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}

VOID
LsaSrvFreeAuthInfo(
    IN  PPOLICY_CONTEXT pPolCtx
    )
{
    if (pPolCtx == NULL) return;

    if (pPolCtx->pUserToken)
    {
        RtlReleaseAccessToken(&pPolCtx->pUserToken);
        pPolCtx->pUserToken = NULL;
    }

    if (pPolCtx->pSessionKey)
    {
        LW_SECURE_FREE_MEMORY(pPolCtx->pSessionKey, pPolCtx->dwSessionKeyLen);
        pPolCtx->pSessionKey     = NULL;
        pPolCtx->dwSessionKeyLen = 0;
    }
}


NTSTATUS
LsaSrvGetSystemCreds(
    OUT LW_PIO_CREDS *ppCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;
    PSTR pszMachinePrincipal = NULL;
    PSTR pszCachePath = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaSrvProviderGetMachineAccountInfoA(
                  LSA_PROVIDER_TAG_AD,
                  NULL,
                  &pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszMachinePrincipal,
                    "%s@%s",
                    pAccountInfo->SamAccountName,
                    pAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszCachePath,
                    "%s.%s",
                    LSASS_KRB5_CACHE_PATH,
                    pAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoCreateKrb5CredsA(
                    pszMachinePrincipal,
                    pszCachePath,
                    &pCreds);
    BAIL_ON_LSA_ERROR(dwError);

    *ppCreds = pCreds;

cleanup:
    LW_SAFE_FREE_STRING(pszMachinePrincipal);
    LW_SAFE_FREE_STRING(pszCachePath);

    LsaSrvFreeMachineAccountInfoA(pAccountInfo);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    *ppCreds = NULL;
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
