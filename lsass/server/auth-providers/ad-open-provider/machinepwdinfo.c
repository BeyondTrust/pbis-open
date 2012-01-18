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
 *        machinepwdinfo.c
 *
 * Abstract:
 *
 *        LSASS AD Provider
 *
 *        Machine Account/Password Info
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#include "adprovider.h"
#include <lsa/lsapstore-api.h>

DWORD
AD_GetMachineAccountInfoA(
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pCachedAccountInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pUncachedPasswordInfo = NULL;
    PLSA_AD_PROVIDER_STATE pState = NULL;
    BOOLEAN IsLocked = FALSE;

    dwError = AD_GetStateWithReference(
                  DnsDomainName,
                  &pState);
    if (dwError == LW_ERROR_NOT_HANDLED)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pState)
    {
        LsaAdProviderStateAcquireRead(pState);
        IsLocked = TRUE;

        if (pState->joinState == LSA_AD_JOINED)
        {
            dwError = LsaPcacheGetMachineAccountInfoA(
                            pState->pPcache,
                            &pCachedAccountInfo);
            if (dwError == NERR_SetupNotJoined)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (!pCachedAccountInfo)
    {
        dwError = LsaPstoreGetPasswordInfoA(
                        DnsDomainName,
                        &pUncachedPasswordInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvDuplicateMachineAccountInfoA(
                pCachedAccountInfo ? pCachedAccountInfo : &pUncachedPasswordInfo->Account,
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

    if (IsLocked)
    {
        LsaAdProviderStateRelease(pState);
    }
    AD_DereferenceProviderState(pState);

    if (pCachedAccountInfo)
    {
        LsaPcacheReleaseMachineAccountInfoA(pCachedAccountInfo);
    }

    if (pUncachedPasswordInfo)
    {
        LsaPstoreFreePasswordInfoA(pUncachedPasswordInfo);
    }

    *ppAccountInfo = pAccountInfo;

    return dwError;
}

DWORD
AD_GetMachineAccountInfoW(
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_W pCachedAccountInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pUncachedPasswordInfo = NULL;
    PLSA_AD_PROVIDER_STATE pState = NULL;
    BOOLEAN IsLocked = FALSE;
    PWSTR DnsDomainNameW = NULL;

    dwError = AD_GetStateWithReference(
                  DnsDomainName,
                  &pState);
    if (dwError == LW_ERROR_NOT_HANDLED)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pState)
    {
        LsaAdProviderStateAcquireRead(pState);
        IsLocked = TRUE;

        if (pState->joinState == LSA_AD_JOINED)
        {
            dwError = LsaPcacheGetMachineAccountInfoW(
                            pState->pPcache,
                            &pCachedAccountInfo);
            if (dwError == NERR_SetupNotJoined)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (!pCachedAccountInfo)
    {
        dwError = LwMbsToWc16s(DnsDomainName, &DnsDomainNameW);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaPstoreGetPasswordInfoW(
                        DnsDomainNameW,
                        &pUncachedPasswordInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvDuplicateMachineAccountInfoW(
                pCachedAccountInfo ? pCachedAccountInfo : &pUncachedPasswordInfo->Account,
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

    if (IsLocked)
    {
        LsaAdProviderStateRelease(pState);
    }
    AD_DereferenceProviderState(pState);

    if (pCachedAccountInfo)
    {
        LsaPcacheReleaseMachineAccountInfoW(pCachedAccountInfo);
    }

    if (pUncachedPasswordInfo)
    {
        LsaPstoreFreePasswordInfoW(pUncachedPasswordInfo);
    }

    LW_SAFE_FREE_MEMORY(DnsDomainNameW);

    *ppAccountInfo = pAccountInfo;

    return dwError;
}

DWORD
AD_GetMachinePasswordInfoA(
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pCachedPasswordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pUncachedPasswordInfo = NULL;
    PLSA_AD_PROVIDER_STATE pState = NULL;
    BOOLEAN IsLocked = FALSE;

    dwError = AD_GetStateWithReference(
                  DnsDomainName,
                  &pState);
    if (dwError == LW_ERROR_NOT_HANDLED)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pState)
    {
        LsaAdProviderStateAcquireRead(pState);
        IsLocked = TRUE;

        if (pState->joinState == LSA_AD_JOINED)
        {
            dwError = LsaPcacheGetMachinePasswordInfoA(
                            pState->pPcache,
                            &pCachedPasswordInfo);
            if (dwError == NERR_SetupNotJoined)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (!pCachedPasswordInfo)
    {
        dwError = LsaPstoreGetPasswordInfoA(
                        DnsDomainName,
                        &pUncachedPasswordInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvDuplicateMachinePasswordInfoA(
                pCachedPasswordInfo ? pCachedPasswordInfo : pUncachedPasswordInfo,
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

    if (IsLocked)
    {
        LsaAdProviderStateRelease(pState);
    }
    AD_DereferenceProviderState(pState);

    if (pCachedPasswordInfo)
    {
        LsaPcacheReleaseMachinePasswordInfoA(pCachedPasswordInfo);
    }

    if (pUncachedPasswordInfo)
    {
        LsaPstoreFreePasswordInfoA(pUncachedPasswordInfo);
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;
}

DWORD
AD_GetMachinePasswordInfoW(
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pCachedPasswordInfo = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pUncachedPasswordInfo = NULL;
    PLSA_AD_PROVIDER_STATE pState = NULL;
    BOOLEAN IsLocked = FALSE;
    PWSTR DnsDomainNameW = NULL;

    dwError = AD_GetStateWithReference(
                  DnsDomainName,
                  &pState);
    if (dwError == LW_ERROR_NOT_HANDLED)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pState)
    {
        LsaAdProviderStateAcquireRead(pState);
        IsLocked = TRUE;

        if (pState->joinState == LSA_AD_JOINED)
        {
            dwError = LsaPcacheGetMachinePasswordInfoW(
                            pState->pPcache,
                            &pCachedPasswordInfo);
            if (dwError == NERR_SetupNotJoined)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (!pCachedPasswordInfo)
    {
        dwError = LwMbsToWc16s(DnsDomainName, &DnsDomainNameW);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaPstoreGetPasswordInfoW(
                        DnsDomainNameW,
                        &pUncachedPasswordInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvDuplicateMachinePasswordInfoW(
                pCachedPasswordInfo ? pCachedPasswordInfo : pUncachedPasswordInfo,
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

    if (IsLocked)
    {
        LsaAdProviderStateRelease(pState);
    }
    AD_DereferenceProviderState(pState);

    if (pCachedPasswordInfo)
    {
        LsaPcacheReleaseMachinePasswordInfoW(pCachedPasswordInfo);
    }

    if (pUncachedPasswordInfo)
    {
        LsaPstoreFreePasswordInfoW(pUncachedPasswordInfo);
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;}
