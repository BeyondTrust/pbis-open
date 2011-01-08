/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ioctl.c
 *
 * Abstract:
 *
 *        AD Provider IOCTL Handlers
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#include "adprovider.h"

DWORD
AD_IoctlGetMachineAccount(
    IN HANDLE hProvider,
    IN DWORD dwInputBufferSize,
    IN PVOID pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PVOID pOutputBuffer = NULL;
    size_t outputBufferSize = 0;
    PAD_PROVIDER_CONTEXT pProviderContext = (PAD_PROVIDER_CONTEXT)hProvider;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PSTR pszDnsDomainName = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    //
    // Do access check
    //

    if (pProviderContext->uid)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //
    // Do request
    //

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  pInputBuffer,
                                  dwInputBufferSize,
                                  OUT_PPVOID(&pszDnsDomainName)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetMachineAccountInfoA(pszDnsDomainName, &pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetMachineAccountInfoSpec(),
                                  pAccountInfo,
                                  &pOutputBuffer,
                                  &outputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pOutputBuffer)
        {
            LwFreeMemory(pOutputBuffer);
        }
        pOutputBuffer = NULL;
        outputBufferSize = 0;
    }

    LW_SAFE_FREE_STRING(pszDnsDomainName);

    if (pAccountInfo)
    {
        LsaSrvFreeMachineAccountInfoA(pAccountInfo);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *pdwOutputBufferSize = (DWORD) outputBufferSize;
    *ppOutputBuffer = pOutputBuffer;

    return dwError;
}

DWORD
AD_IoctlGetMachinePassword(
    IN HANDLE hProvider,
    IN DWORD dwInputBufferSize,
    IN PVOID pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PVOID pOutputBuffer = NULL;
    size_t outputBufferSize = 0;
    PAD_PROVIDER_CONTEXT pProviderContext = (PAD_PROVIDER_CONTEXT)hProvider;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PSTR pszDnsDomainName = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    //
    // Do access check
    //

    if (pProviderContext->uid)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //
    // Do request
    //

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  pInputBuffer,
                                  dwInputBufferSize,
                                  OUT_PPVOID(&pszDnsDomainName)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetMachinePasswordInfoA(pszDnsDomainName, &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetMachinePasswordInfoSpec(),
                                  pPasswordInfo,
                                  &pOutputBuffer,
                                  &outputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pOutputBuffer)
        {
            LwFreeMemory(pOutputBuffer);
        }
        pOutputBuffer = NULL;
        outputBufferSize = 0;
    }

    LW_SAFE_FREE_STRING(pszDnsDomainName);

    if (pPasswordInfo)
    {
        LsaSrvFreeMachinePasswordInfoA(pPasswordInfo);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *pdwOutputBufferSize = (DWORD) outputBufferSize;
    *ppOutputBuffer = pOutputBuffer;

    return dwError;
}
