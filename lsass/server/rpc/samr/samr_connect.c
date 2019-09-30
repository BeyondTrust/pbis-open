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
 *        samr_connect.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrConnect function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvConnect(
    /* [in] */  handle_t        hBinding,
    /* [in] */  PCWSTR          pwszSystemName,
    /* [in] */  DWORD           dwAccessMask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    const DWORD dwConnectVersion = 2;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;

    ntStatus = SamrSrvConnectInternal(hBinding,
                                      pwszSystemName,
                                      dwAccessMask,
                                      dwConnectVersion,
                                      0,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &pConnCtx);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *hConn = (CONNECT_HANDLE)pConnCtx;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pConnCtx)
    {
        SamrSrvConnectContextFree(pConnCtx);
    }

    *hConn = NULL;
    goto cleanup;
}


NTSTATUS
SamrSrvConnectInternal(
    IN  handle_t            hBinding,
    IN  PCWSTR              pwszSystemName,
    IN  DWORD               dwAccessMask,
    IN  DWORD               dwConnectVersion,
    IN  DWORD               dwLevelIn,
    IN  PSAMR_CONNECT_INFO  pInfoIn,
    OUT PDWORD              pdwLevelOut,
    OUT PSAMR_CONNECT_INFO  pInfoOut,
    OUT PCONNECT_CONTEXT   *ppConnCtx
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = gpSamrSecDesc;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;

    BAIL_ON_INVALID_PTR(hBinding);
    BAIL_ON_INVALID_PTR(pwszSystemName);
    BAIL_ON_INVALID_PTR(ppConnCtx);

    dwError = LwAllocateMemory(sizeof(*pConnCtx),
                               OUT_PPVOID(&pConnCtx));
    BAIL_ON_LSA_ERROR(dwError);

    pConnCtx->Type     = SamrContextConnect;
    pConnCtx->refcount = 1;

    ntStatus = SamrSrvInitAuthInfo(hBinding, pConnCtx);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (!RtlAccessCheck(pSecDesc,
                        pConnCtx->pUserToken,
                        dwAccessMask,
                        pConnCtx->dwAccessGranted,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pConnCtx->dwAccessGranted = dwAccessGranted;

    dwError = DirectoryOpen(&pConnCtx->hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    pConnCtx->dwConnectVersion = dwConnectVersion;

    if (dwConnectVersion == 5)
    {
        BAIL_ON_INVALID_PTR(pInfoIn);
        BAIL_ON_INVALID_PTR(pInfoOut);
        BAIL_ON_INVALID_PTR(pdwLevelOut);

        pConnCtx->dwLevel = dwLevelIn;
        pConnCtx->Info    = *pInfoIn;
    }

    if (pdwLevelOut)
    {
        *pdwLevelOut = pConnCtx->dwLevel;
    }

    if (pInfoOut)
    {
        *pInfoOut = pConnCtx->Info;
    }

    *ppConnCtx = pConnCtx;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pdwLevelOut)
    {
        *pdwLevelOut = 0;
    }

    if (pInfoOut)
    {
        memset(pInfoOut, 0, sizeof(*pInfoOut));
    }

    if (pConnCtx)
    {
        SamrSrvConnectContextFree(pConnCtx);
    }

    *ppConnCtx = NULL;
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
