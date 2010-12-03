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
VOID
AD_FreeMachinePassword(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        LW_SAFE_FREE_STRING(pPasswordInfo->Account.DnsDomainName);
        LW_SAFE_FREE_STRING(pPasswordInfo->Account.NetbiosDomainName);
        LW_SAFE_FREE_STRING(pPasswordInfo->Account.DomainSid);
        LW_SAFE_FREE_STRING(pPasswordInfo->Account.SamAccountName);
        LW_SAFE_FREE_STRING(pPasswordInfo->Account.Fqdn);
        LW_SECURE_FREE_STRING(pPasswordInfo->Password);
        LwFreeMemory(pPasswordInfo);
    }
}

static
DWORD
AD_ConvertPasswordInfo(
    IN PLWPS_PASSWORD_INFO_A pLegacyPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
///<
/// Convert legacy pstore password info to LSA pstore version.
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
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pPasswordInfo), OUT_PPVOID(&pPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszDnsDomainName,
                    &pPasswordInfo->Account.DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pPasswordInfo->Account.DnsDomainName);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszDomainName,
                    &pPasswordInfo->Account.NetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pPasswordInfo->Account.NetbiosDomainName);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszSID,
                    &pPasswordInfo->Account.DomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszMachineAccount,
                    &pPasswordInfo->Account.SamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pPasswordInfo->Account.SamAccountName);

    // TODO-2010/12/03-dalmeida - Hard-coded for now.
    pPasswordInfo->Account.Type = LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
    pPasswordInfo->Account.KeyVersionNumber = 1;

    dwError = LwAllocateStringPrintf(
                    &pPasswordInfo->Account.Fqdn,
                    "%s.%s",
                    pLegacyPasswordInfo->pszHostname,
                    pLegacyPasswordInfo->pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToLower(pPasswordInfo->Account.Fqdn);

    // TODO-2010/12/03-dalmeida - Incorrect conversion.
    pPasswordInfo->Account.LastChangeTime = pLegacyPasswordInfo->last_change_time;

    dwError = LwStrDupOrNull(
                    pLegacyPasswordInfo->pszMachinePassword,
                    &pPasswordInfo->Password);
    BAIL_ON_LSA_ERROR(dwError);
                        

error:
    if (dwError)
    {
        if (pPasswordInfo)
        {
            AD_FreeMachinePassword(pPasswordInfo);
        }
        pPasswordInfo = NULL;
    }

    *ppPasswordInfo = pPasswordInfo;

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
    PLWPS_PASSWORD_INFO_A pPasswordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pFinalPasswordInfo = NULL;

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

    dwError = AD_GetPasswordInfo(pszDnsDomainName, NULL, &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ConvertPasswordInfo(pPasswordInfo, &pFinalPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetMachinePasswordRespSpec(),
                                  pFinalPasswordInfo,
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
        LwFreePasswordInfoA(pPasswordInfo);
    }

    if (pFinalPasswordInfo)
    {
        AD_FreeMachinePassword(pFinalPasswordInfo);
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
