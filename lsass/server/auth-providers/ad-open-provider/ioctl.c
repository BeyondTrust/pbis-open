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

static
inline
VOID
AD_FreeMachineAccountInfoContents(
    IN OUT PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    LW_SAFE_FREE_STRING(pAccountInfo->DnsDomainName);
    LW_SAFE_FREE_STRING(pAccountInfo->NetbiosDomainName);
    LW_SAFE_FREE_STRING(pAccountInfo->DomainSid);
    LW_SAFE_FREE_STRING(pAccountInfo->SamAccountName);
    LW_SAFE_FREE_STRING(pAccountInfo->Fqdn);
}

static
VOID
AD_FreeMachineAccountInfo(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    if (pAccountInfo)
    {
        AD_FreeMachineAccountInfoContents(pAccountInfo);
        LwFreeMemory(pAccountInfo);
    }
}

static
VOID
AD_FreeMachinePasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        AD_FreeMachineAccountInfoContents(&pPasswordInfo->Account);
        LW_SECURE_FREE_STRING(pPasswordInfo->Password);
        LwFreeMemory(pPasswordInfo);
    }
}

static
DWORD
AD_FillAccountInfo(
    IN PLWPS_PASSWORD_INFO_A pLegacyPasswordInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
///<
/// Fill LSA pstore machine account info from legacy pstore password info.
///
/// This also handles any conversions or "cons"-ing up of values that
/// are not stored in the old legacy format.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on failure
///
/// @note This function will go away once the new pstore changes
///       are complete.
///
{
    DWORD dwError = 0;

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszDnsDomainName,
                    &pAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pAccountInfo->DnsDomainName);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszDomainName,
                    &pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pAccountInfo->NetbiosDomainName);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszSID,
                    &pAccountInfo->DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszMachineAccount,
                    &pAccountInfo->SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pAccountInfo->SamAccountName);

    // TODO-2010/12/03-dalmeida - Hard-coded for now.
    pAccountInfo->Type = LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
    pAccountInfo->KeyVersionNumber = 1;

    dwError = LwAllocateStringPrintf(
                    &pAccountInfo->Fqdn,
                    "%s.%s",
                    pLegacyPasswordInfo->pszHostname,
                    pLegacyPasswordInfo->pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToLower(pAccountInfo->Fqdn);

    // TODO-2010/12/03-dalmeida - Incorrect conversion.
    pAccountInfo->LastChangeTime = pLegacyPasswordInfo->last_change_time;

error:
    if (dwError)
    {
        AD_FreeMachineAccountInfoContents(pAccountInfo);
    }

    return dwError;    

}

static
DWORD
AD_ConvertToAccountInfo(
    IN PLWPS_PASSWORD_INFO_A pLegacyPasswordInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pAccountInfo), OUT_PPVOID(&pAccountInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FillAccountInfo(pLegacyPasswordInfo, pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pAccountInfo)
        {
            AD_FreeMachineAccountInfo(pAccountInfo);
        }
        pAccountInfo = NULL;
    }

    *ppAccountInfo = pAccountInfo;

    return dwError;    
}

static
DWORD
AD_ConvertToPasswordInfo(
    IN PLWPS_PASSWORD_INFO_A pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pPasswordInfo), OUT_PPVOID(&pPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FillAccountInfo(pLegacyPasswordInfo, &pPasswordInfo->Account);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszMachinePassword,
                    &pPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);                 

error:
    if (dwError)
    {
        if (pPasswordInfo)
        {
            AD_FreeMachinePasswordInfo(pPasswordInfo);
        }
        pPasswordInfo = NULL;
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;    
}

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
    PLWPS_PASSWORD_INFO_A pInternalPasswordInfo = NULL;
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

    dwError = AD_GetPasswordInfo(pszDnsDomainName, NULL, &pInternalPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ConvertToAccountInfo(pInternalPasswordInfo, &pAccountInfo);
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

    if (pInternalPasswordInfo)
    {
        LwFreePasswordInfoA(pInternalPasswordInfo);
    }

    if (pAccountInfo)
    {
        AD_FreeMachineAccountInfo(pAccountInfo);
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
    PLWPS_PASSWORD_INFO_A pInternalPasswordInfo = NULL;
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

    dwError = AD_GetPasswordInfo(pszDnsDomainName, NULL, &pInternalPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ConvertToPasswordInfo(pInternalPasswordInfo, &pPasswordInfo);
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

    if (pInternalPasswordInfo)
    {
        LwFreePasswordInfoA(pInternalPasswordInfo);
    }

    if (pPasswordInfo)
    {
        AD_FreeMachinePasswordInfo(pPasswordInfo);
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
