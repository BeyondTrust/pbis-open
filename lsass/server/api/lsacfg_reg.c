/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lsacfg_reg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 */
#include "api.h"

DWORD
LsaProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLSA_CONFIG pConfig,
    DWORD dwConfigEntries
    )
{
    DWORD dwError = 0;
    DWORD dwEntry;

    PLSA_CONFIG_REG pReg = NULL;

    dwError = LsaOpenConfig(pszConfigKey, pszPolicyKey, &pReg);
    BAIL_ON_LSA_ERROR(dwError);

    if ( pReg == NULL || pReg->hConnection == NULL )
    {
        goto error;
    }

    for (dwEntry = 0; dwEntry < dwConfigEntries; dwEntry++)
    {
        switch (pConfig[dwEntry].Type)
        {
            case LsaTypeString:
                dwError = LsaReadConfigString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue,
                            pConfig[dwEntry].pdwSize);
                break;

            case LsaTypeMultiString:
                dwError = LsaReadConfigMultiString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue,
                            pConfig[dwEntry].pdwSize);
                break;

            case LsaTypeDword:
                dwError = LsaReadConfigDword(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].pValue);
                break;

            case LsaTypeBoolean:
                dwError = LsaReadConfigBoolean(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue);
                break;

            case LsaTypeEnum:
                dwError = LsaReadConfigEnum(
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
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LsaCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:
    goto cleanup;    
}

DWORD
LsaOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLSA_CONFIG_REG *ppReg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PLSA_CONFIG_REG pReg = NULL;

    dwError = LwAllocateMemory(sizeof(LSA_CONFIG_REG), (PVOID*)&pReg);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszConfigKey, &(pReg->pszConfigKey));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszPolicyKey, &(pReg->pszPolicyKey));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegOpenServer(&(pReg->hConnection));
    if ( dwError || (pReg->hConnection == NULL))
    {
        LSA_LOG_WARNING("Couldn't open the registry handle. Error: %s (%u)",
                        LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                        dwError);
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
        LSA_LOG_WARNING("Couldn't open HKEY_THIS_MACHINE key. Error: %s (%u)",
                        LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                        dwError);
        dwError = 0;
        goto error;
    }

cleanup:

    *ppReg = pReg;

    return dwError;

error:

    LsaCloseConfig(pReg);
    pReg = NULL;

    goto cleanup;
}

VOID
LsaCloseConfig(
    PLSA_CONFIG_REG pReg
    )
{
    if ( pReg )
    {
        LW_SAFE_FREE_STRING(pReg->pszConfigKey);

        LW_SAFE_FREE_STRING(pReg->pszPolicyKey);
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

        LW_SAFE_FREE_MEMORY(pReg);
    }
}

DWORD
LsaReadConfigString(
    PLSA_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
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
        {
            bGotValue = TRUE;

            LSA_LOG_VERBOSE("String value [%s] = \"%s\" read from "
                            "registry key [%s].",
                            pszName,
                            szValue,
                            pReg->pszPolicyKey);
        }
        else
        {
            LSA_LOG_VERBOSE("Couldn't read string value [%s] from "
                            "registry key [%s]. Error: %s (%u)",
                            pszName,
                            pReg->pszPolicyKey,
                            LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                            dwError);
        }
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
        {
            bGotValue = TRUE;

            LSA_LOG_VERBOSE("String value [%s] = \"%s\" read from "
                            "registry key [%s].",
                            pszName,
                            szValue,
                            pReg->pszConfigKey);
        }
        else
        {
            LSA_LOG_WARNING("Couldn't read string value [%s] from "
                            "registry key [%s]. Error: %s (%u)",
                            pszName,
                            pReg->pszConfigKey,
                            LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                            dwError);
        }
    }

    if (bGotValue)
    {
        dwError = LwAllocateString(szValue, &pszValue);
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_STRING(*ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;

        if (pdwSize)
        {
            *pdwSize = dwSize;
        }
    }

    dwError = 0;

cleanup:

    LW_SAFE_FREE_STRING(pszValue);

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaReadConfigMultiString(
    PLSA_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
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
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!dwError)
        {
            bGotValue = TRUE;

            LSA_LOG_VERBOSE("Multistring value [%s] read from "
                            "registry key [%s].",
                            pszName,
                            pReg->pszPolicyKey);
        }
        else
        {
            LSA_LOG_VERBOSE("Couldn't read multistring value [%s] from "
                            "registry key [%s]. Error: %s (%d)",
                            pszName,
                            pReg->pszPolicyKey,
                            LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                            dwError);
        }
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
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!dwError)
        {
            bGotValue = TRUE;

            LSA_LOG_VERBOSE("Multistring value [%s] read from "
                            "registry key [%s].",
                            pszName,
                            pReg->pszConfigKey);
        }
        else
        {
            LSA_LOG_WARNING("Couldn't read multistring value [%s] from "
                            "registry key [%s]. Error: %s (%d)",
                            pszName,
                            pReg->pszConfigKey,
                            LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                            dwError);
        }
    }

    if (bGotValue)
    {
        dwError = LwAllocateMemory(dwSize, (PVOID)&pszValue);
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pszValue, szValue, dwSize);

        LW_SAFE_FREE_MEMORY(*ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;

        if (pdwSize)
        {
            *pdwSize = dwSize;
        }
    }

    dwError = 0;

cleanup:

    LW_SAFE_FREE_MEMORY(pszValue);

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaReadConfigDword(
    PLSA_CONFIG_REG pReg,
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

            LSA_LOG_VERBOSE("DWORD value [%s] = 0x%08x (%u) read from "
                            "registry key [%s].",
                            pszName,
                            dwValue,
                            dwValue,
                            pReg->pszPolicyKey);
        }
        else
        {
            LSA_LOG_VERBOSE("Couldn't read DWORD value [%s] from "
                            "registry key [%s]. Error: %s (%u)",
                            pszName,
                            pReg->pszPolicyKey,
                            LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                            dwError);
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

            LSA_LOG_VERBOSE("DWORD value [%s] = 0x%08x (%u) read from "
                            "registry key [%s].",
                            pszName,
                            dwValue,
                            dwValue,
                            pReg->pszConfigKey);
        }
        else
        {
            LSA_LOG_WARNING("Couldn't read DWORD value [%s] from "
                            "registry key [%s]. Error: %s (%u)",
                            pszName,
                            pReg->pszConfigKey,
                            LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)),
                            dwError);
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
LsaReadConfigBoolean(
    PLSA_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    )
{

    DWORD dwError = 0;

    DWORD dwValue = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    dwError = LsaReadConfigDword(
                pReg,
                pszName,
                bUsePolicy,
                0,
                -1,
                &dwValue);
    BAIL_ON_LSA_ERROR(dwError);

    *pbValue = dwValue ? TRUE : FALSE;

cleanup:

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaReadConfigEnum(
    PLSA_CONFIG_REG pReg,
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

    dwError = LsaReadConfigString(
                pReg,
                pszName,
                bUsePolicy,
                &pszValue,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

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
    LW_SAFE_FREE_STRING(pszValue);
    return dwError;

error:
    goto cleanup;    
}
