/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        provider.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        User Lookup and Management (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvProviderIoControl(
    IN HANDLE hServer,
    IN PCSTR  pszProvider,
    IN DWORD  dwIoControlCode,
    IN DWORD  dwInputBufferSize,
    IN PVOID  pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    HANDLE hProvider = (HANDLE)NULL;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;

    dwError = LsaSrvGetTargetElements(
                  pszProvider,
                  &pszTargetProviderName,
                  &pszTargetInstance);
    BAIL_ON_LSA_ERROR(dwError);


    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if ( !strcmp(pProvider->pszId, pszTargetProviderName) )
        {
            dwError = LsaSrvOpenProvider(
                          hServer,
                          pProvider,
                          pszTargetInstance,
                          &hProvider);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = pProvider->pFnTable->pfnProviderIoControl(
                                            hProvider,
                                            pServerState->peerUID,
                                            pServerState->peerGID,
                                            dwIoControlCode,
                                            dwInputBufferSize,
                                            pInputBuffer,
                                            pdwOutputBufferSize,
                                            ppOutputBuffer);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }
    }

    if (pProvider == NULL)
    {
       dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return(dwError);

error:

    LSA_LOG_ERROR_API_FAILED(hServer, dwError,
        "run provider specific request (request code = %u, provider = '%s')",
        dwIoControlCode, LSA_SAFE_LOG_STRING(pszTargetProviderName));

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    goto cleanup;
}

DWORD
LsaSrvProviderServicesDomain(
    IN PCSTR pszProvider,
    IN PCSTR pszDomainName,
    OUT PBOOLEAN pbServicesDomain
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bServicesDomain = FALSE;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LsaSrvFindProviderByName(pszProvider, &pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pProvider->pFnTable->pfnServicesDomain(
                  pszDomainName,
                  &bServicesDomain);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    *pbServicesDomain = bServicesDomain;

    return dwError;

error:
    bServicesDomain = FALSE;

    goto cleanup;
}

DWORD
LsaSrvProviderGetMachineAccountInfoA(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LsaSrvFindProviderByName(pszProvider, &pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pProvider->pFnTable->pfnGetMachineAccountInfoA)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = pProvider->pFnTable->pfnGetMachineAccountInfoA(
                    DnsDomainName,
                    &pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pAccountInfo)
        {
            LsaSrvFreeMachineAccountInfoA(pAccountInfo);
            pAccountInfo = NULL;
        }
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    *ppAccountInfo = pAccountInfo;

    return dwError;
}

DWORD
LsaSrvProviderGetMachineAccountInfoW(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = NULL;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LsaSrvFindProviderByName(pszProvider, &pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pProvider->pFnTable->pfnGetMachineAccountInfoW)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = pProvider->pFnTable->pfnGetMachineAccountInfoW(
                    DnsDomainName,
                    &pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pAccountInfo)
        {
            LsaSrvFreeMachineAccountInfoW(pAccountInfo);
            pAccountInfo = NULL;
        }
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    *ppAccountInfo = pAccountInfo;

    return dwError;
}

DWORD
LsaSrvProviderGetMachinePasswordInfoA(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LsaSrvFindProviderByName(pszProvider, &pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pProvider->pFnTable->pfnGetMachinePasswordInfoA)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = pProvider->pFnTable->pfnGetMachinePasswordInfoA(
                    DnsDomainName,
                    &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pPasswordInfo)
        {
            LsaSrvFreeMachinePasswordInfoA(pPasswordInfo);
            pPasswordInfo = NULL;
        }
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    *ppPasswordInfo = pPasswordInfo;

    return dwError;
}

DWORD
LsaSrvProviderGetMachinePasswordInfoW(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LsaSrvFindProviderByName(pszProvider, &pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pProvider->pFnTable->pfnGetMachinePasswordInfoW)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = pProvider->pFnTable->pfnGetMachinePasswordInfoW(
                    DnsDomainName,
                    &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pPasswordInfo)
        {
            LsaSrvFreeMachinePasswordInfoW(pPasswordInfo);
            pPasswordInfo = NULL;
        }
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    *ppPasswordInfo = pPasswordInfo;

    return dwError;
}
