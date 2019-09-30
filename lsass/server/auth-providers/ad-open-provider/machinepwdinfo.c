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
