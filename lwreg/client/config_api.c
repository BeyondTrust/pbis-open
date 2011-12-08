/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        nt_config_api.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        NT Client wrapper API for configuration processing
 *
 * Authors: Scott Salley <ssalley@likewise.com>
 */
#include "client.h"

typedef struct __LWREG_CONFIG_REG LWREG_CONFIG_REG, *PLWREG_CONFIG_REG;

struct __LWREG_CONFIG_REG
{
    HANDLE hConnection;
    HKEY hKey;
    PSTR pszConfigKey;
    PSTR pszPolicyKey;
};

static
NTSTATUS
NtRegOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWREG_CONFIG_REG *ppReg
    );

static
VOID
NtRegCloseConfig(
    PLWREG_CONFIG_REG pReg
    );

static
NTSTATUS
NtRegReadConfigString(
    PLWREG_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
    );

static
NTSTATUS
NtRegReadConfigMultiString(
    PLWREG_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
    );

static
NTSTATUS
NtRegReadConfigDword(
    PLWREG_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    );

static
NTSTATUS
NtRegReadConfigBoolean(
    PLWREG_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

static
NTSTATUS
NtRegReadConfigEnum(
    PLWREG_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    const PCSTR   *ppszEnumNames,
    PDWORD  pdwValue
    );

/**
 * Read configuration values from the registry
 *
 * This function loops through a configuration table reading all given values
 * from a registry key.  If an entry is not found in the registry
 * @dwConfigEntries determines whether an error is returned.
 *
 * @param[in] pszConfigKey Registry key path
 * @param[in] pszPolicyKey Registry policy key path
 * @param[in] pConfig Configuration table specifying parameter names
 * @param[in] dwConfigEntries Number of table entries
 *
 * @return STATUS_SUCCESS, or appropriate error.
 */
NTSTATUS
NtRegProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWREG_CONFIG_ITEM pConfig,
    DWORD dwConfigEntries
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwEntry = 0;
    PLWREG_CONFIG_REG pReg = NULL;

    ntStatus = NtRegOpenConfig(pszConfigKey, pszPolicyKey, &pReg);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pReg == NULL)
    {
        goto error;
    }

    for (dwEntry = 0; dwEntry < dwConfigEntries; dwEntry++)
    {
        ntStatus = STATUS_SUCCESS;
        switch (pConfig[dwEntry].Type)
        {
            case LwRegTypeString:
                ntStatus = NtRegReadConfigString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue,
                            pConfig[dwEntry].pdwSize);
                break;

            case LwRegTypeMultiString:
                ntStatus = NtRegReadConfigMultiString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue,
                            pConfig[dwEntry].pdwSize);
                break;

            case LwRegTypeDword:
                ntStatus = NtRegReadConfigDword(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].pValue);
                break;

            case LwRegTypeBoolean:
                ntStatus = NtRegReadConfigBoolean(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue);
                break;

            case LwRegTypeEnum:
                ntStatus = NtRegReadConfigEnum(
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
        if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    NtRegCloseConfig(pReg);
    pReg = NULL;

    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
NtRegOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWREG_CONFIG_REG *ppReg
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWREG_CONFIG_REG pReg = NULL;

    ntStatus = LW_RTL_ALLOCATE(
                   (PVOID*)&pReg, 
                   LWREG_CONFIG_REG, 
                   sizeof(LWREG_CONFIG_REG));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCStringDuplicate(&pReg->pszConfigKey, pszConfigKey);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pszPolicyKey)
    {
        ntStatus = LwRtlCStringDuplicate(&pReg->pszPolicyKey, pszPolicyKey);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NtRegOpenServer(&pReg->hConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExA(
            pReg->hConnection,
            NULL,
            HKEY_THIS_MACHINE,
            0,
            KEY_READ,
            &(pReg->hKey));
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    *ppReg = pReg;

    return ntStatus;

error:

    NtRegCloseConfig(pReg);
    pReg = NULL;

    goto cleanup;
}

VOID
NtRegCloseConfig(
    PLWREG_CONFIG_REG pReg
    )
{
    if (pReg)
    {
        LwRtlCStringFree(&pReg->pszConfigKey);

        LwRtlCStringFree(&pReg->pszPolicyKey);

        if (pReg->hConnection)
        {
            if ( pReg->hKey )
            {
                NtRegCloseKey(pReg->hConnection, pReg->hKey);
                pReg->hKey = NULL;
            }
            NtRegCloseServer(pReg->hConnection);
            pReg->hConnection = NULL;
        }

        RTL_FREE(&pReg);
    }
}

NTSTATUS
NtRegReadConfigString(
    PLWREG_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH];
    DWORD dwType;
    DWORD dwSize;

    if ( bUsePolicy )
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue )
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        ntStatus = LwRtlCStringDuplicate(&pszValue, szValue);
        BAIL_ON_NT_STATUS(ntStatus);

        LwRtlCStringFree(ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;

        if (pdwSize)
        {
            *pdwSize = dwSize;
        }
    }

    ntStatus = 0;

cleanup:
    LwRtlCStringFree(&pszValue);
    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
NtRegReadConfigMultiString(
    PLWREG_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH];
    DWORD dwType = 0;
    DWORD dwSize = 0;

    if (bUsePolicy)
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue )
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        ntStatus = LW_RTL_ALLOCATE(&pszValue, char,
                                        dwSize);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pszValue, szValue, dwSize);

        LwRtlCStringFree(ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;

        if (pdwSize)
        {
            *pdwSize = dwSize;
        }
    }

    ntStatus = 0;

cleanup:

    LwRtlCStringFree(&pszValue);

    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
NtRegReadConfigDword(
    PLWREG_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    DWORD dwValue = 0;
    DWORD dwSize =0;
    DWORD dwType = 0;

    if (bUsePolicy)
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwSize = sizeof(dwValue);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(dwValue);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        if (dwMin <= dwValue && dwValue <= dwMax)
        {
            *pdwValue = dwValue;
        }
        else
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
NtRegReadConfigBoolean(
    PLWREG_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    )
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwValue = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    ntStatus = NtRegReadConfigDword(
                pReg,
                pszName,
                bUsePolicy,
                0,
                -1,
                &dwValue);
    BAIL_ON_NT_STATUS(ntStatus);

    *pbValue = dwValue ? TRUE : FALSE;

cleanup:

    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
NtRegReadConfigEnum(
    PLWREG_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    const PCSTR   *ppszEnumNames,
    PDWORD  pdwValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszValue = NULL;
    DWORD dwEnumIndex = 0;

    ntStatus = NtRegReadConfigString(
                pReg,
                pszName,
                bUsePolicy,
                &pszValue,
                NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pszValue != NULL )
    {
        for (dwEnumIndex = 0;
             dwEnumIndex <= dwMax - dwMin;
             dwEnumIndex++)
        {
            if(LwRtlCStringCompare(
                   pszValue,
                   ppszEnumNames[dwEnumIndex], FALSE) == 0)
            {
                *pdwValue = dwEnumIndex + dwMin;
                break;
            }
        }
    }

cleanup:
    LwRtlCStringFree(&pszValue);

    return ntStatus;

error:
    goto cleanup;
}


DWORD
RegProcessConfig(
    IN PCSTR pszConfigKey,
    IN PCSTR pszPolicyKey,
    IN OUT PLWREG_CONFIG_ITEM pConfig,
    IN DWORD dwConfigEntries
    )
{
    return RegNtStatusToWin32Error(
            NtRegProcessConfig(
                pszConfigKey,
                pszPolicyKey,
                pConfig,
                dwConfigEntries));
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
