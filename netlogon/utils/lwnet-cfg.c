/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 * Abstract:
 *
 */
#include "includes.h"

DWORD
LWNetProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWNET_CONFIG pConfig,
    DWORD dwConfigEntries
    )
{
    DWORD dwError = 0;
    DWORD dwEntry;

    PLWNET_CONFIG_REG pReg = NULL;

    dwError = LWNetOpenConfig(pszConfigKey, pszPolicyKey, &pReg);
    BAIL_ON_LWNET_ERROR(dwError);

    if ( pReg == NULL )
    {
        goto error;
    }

    for (dwEntry = 0; dwEntry < dwConfigEntries; dwEntry++)
    {
        dwError = 0;
        switch (pConfig[dwEntry].Type)
        {
            case LWNetTypeString:
                dwError = LWNetReadConfigString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue);
                break;

            case LWNetTypeDword:
                dwError = LWNetReadConfigDword(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].pValue);
                break;

            case LWNetTypeBoolean:
                dwError = LWNetReadConfigBoolean(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue);
                break;

            case LWNetTypeEnum:
                dwError = LWNetReadConfigEnum(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].ppszEnumNames,
                            pConfig[dwEntry].pValue);
                break;

            default:
                break;
        }
        BAIL_ON_NON_LWREG_ERROR(dwError);
        dwError = 0;
    }

cleanup:
    LWNetCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:
    goto cleanup;    
}

DWORD
LWNetOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWNET_CONFIG_REG *ppReg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PLWNET_CONFIG_REG pReg = NULL;

    LWNetAllocateMemory(sizeof(LWNET_CONFIG_REG), (PVOID*)&pReg);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwAllocateString(pszConfigKey, &(pReg->pszConfigKey));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwAllocateString(pszPolicyKey, &(pReg->pszPolicyKey));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = RegOpenServer(&(pReg->hConnection));
    if ( dwError )
    {
        dwError = 0;
        goto error;
    }

    dwError = RegOpenKeyExA(
            pReg->hConnection,
            NULL,
            HKEY_THIS_MACHINE,
            0,
            KEY_READ,
            &(pReg->hKey));
    if (dwError)
    {
        dwError = 0;
        goto error;
    }

cleanup:

    *ppReg = pReg;

    return dwError;

error:

    LWNetCloseConfig(pReg);
    pReg = NULL;

    goto cleanup;
}

VOID
LWNetCloseConfig(
    PLWNET_CONFIG_REG pReg
    )
{
    if ( pReg )
    {
        LWNET_SAFE_FREE_STRING(pReg->pszConfigKey);

        LWNET_SAFE_FREE_STRING(pReg->pszPolicyKey);
        if ( pReg->hConnection )
        {
            if ( pReg->hKey )
            {
                RegCloseKey(pReg->hConnection, pReg->hKey);
                pReg->hKey = NULL;
            }
            RegCloseServer(pReg->hConnection);
            pReg->hConnection = NULL;
        }

        LWNET_SAFE_FREE_MEMORY(pReg);
    }
}

DWORD
LWNetReadConfigString(
    PLWNET_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    )
{
    DWORD dwError = 0;
    
    BOOLEAN bGotValue = FALSE;
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH];
    DWORD dwType;
    DWORD dwSize;

    if ( bUsePolicy )
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        dwError = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!dwError)
            bGotValue = TRUE;
    }

    if (!bGotValue )
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        dwError = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!dwError)
            bGotValue = TRUE;
    }
 
    if (bGotValue)
    {
        dwError = LWNetAllocateString(szValue, &pszValue);
        BAIL_ON_LWNET_ERROR(dwError);

        LWNET_SAFE_FREE_STRING(*ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;
    }

    dwError = 0;
    
cleanup: 
    LWNET_SAFE_FREE_STRING(pszValue);

    return dwError;

error:
    goto cleanup;    
}

DWORD
LWNetReadConfigDword(
    PLWNET_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;

    BOOLEAN bGotValue = FALSE;
    DWORD dwValue;
    DWORD dwSize;
    DWORD dwType;

    if (bUsePolicy)
    {
        dwSize = sizeof(dwValue);
        dwError = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!dwError)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(dwValue);
        dwError = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!dwError)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        if ( dwMin <= dwValue && dwValue <= dwMax)
            *pdwValue = dwValue;
    }

    dwError = 0;

    return dwError;
}

DWORD
LWNetReadConfigBoolean(
    PLWNET_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    )
{

    DWORD dwError = 0;

    DWORD dwValue = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    dwError = LWNetReadConfigDword(
                pReg,
                pszName,
                bUsePolicy,
                0,
                -1,
                &dwValue);
    BAIL_ON_LWNET_ERROR(dwError);

    *pbValue = dwValue ? TRUE : FALSE;

cleanup:

    return dwError;

error:
    goto cleanup;
}

DWORD
LWNetReadConfigEnum(
    PLWNET_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    const PCSTR   *ppszEnumNames,
    PDWORD  pdwValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;

    DWORD dwEnumIndex;

    dwError = LWNetReadConfigString(
                pReg,
                pszName,
                bUsePolicy,
                &pszValue);
    BAIL_ON_LWNET_ERROR(dwError);

    if (pszValue != NULL )
    {
        for (dwEnumIndex = 0;
             dwEnumIndex <= dwMax - dwMin;
             dwEnumIndex++)
        {
            if(!strcasecmp(pszValue, ppszEnumNames[dwEnumIndex]))
            {
                *pdwValue = dwEnumIndex + dwMin;
                goto cleanup;
            }
        }
    }

cleanup:
    LWNET_SAFE_FREE_STRING(pszValue);
    return dwError;

error:
    goto cleanup;    
}

