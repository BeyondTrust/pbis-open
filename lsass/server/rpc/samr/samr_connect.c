/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
