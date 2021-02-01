/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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


NTSTATUS 
NtRegUpdateConfigItemRange(
    IN PCSTR pszConfigKey,
    IN OUT PLWREG_CONFIG_ITEM pConfig,
    IN DWORD dwConfigEntries
    )
{
    PLWREG_CURRENT_VALUEINFO* DONT_RETRIEVE_CURRENT_VALUE = NULL;
    PCSTR NO_POLICY_KEY = NULL;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWREG_CONFIG_REG pReg = NULL;
    DWORD dwEntry = 0;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszValueName = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;

    ntStatus = NtRegOpenConfig(pszConfigKey, NO_POLICY_KEY, &pReg);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pReg == NULL)
    {
        goto error;
    }

    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pReg->pszConfigKey);
    BAIL_ON_NT_STATUS(ntStatus);

    for (dwEntry = 0; dwEntry < dwConfigEntries; ntStatus = STATUS_SUCCESS, dwEntry++)
    {
        if (pwszValueName) 
        {
            free(pwszValueName);
            pwszValueName = NULL;
        }
    
        ntStatus = LwRtlWC16StringAllocateFromCString(&pwszValueName, pConfig[dwEntry].pszName);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NtRegGetValueAttributesW(
                pReg->hConnection,
                pReg->hKey,
                pwszSubKey,
                pwszValueName,
                DONT_RETRIEVE_CURRENT_VALUE,
                &pValueAttributes
                );

        if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            continue;
        }

        BAIL_ON_NT_STATUS(ntStatus);

        if (pValueAttributes 
                && pValueAttributes->RangeType == LWREG_VALUE_RANGE_TYPE_INTEGER) 
        {
            pConfig[dwEntry].dwMin = pValueAttributes->Range.RangeInteger.Min;
            pConfig[dwEntry].dwMax = pValueAttributes->Range.RangeInteger.Max;
        }
    }

cleanup:

    if (pwszValueName) 
    {
        free(pwszValueName);
        pwszValueName = NULL;
    }

    if (pwszSubKey) 
    {
        free(pwszSubKey);
        pwszSubKey = NULL;
    }

    if (pReg) 
    {
        NtRegCloseConfig(pReg);
        pReg = NULL;
    }

    return ntStatus;

error:
    goto cleanup;
}

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
NtRegProcessConfigUsingAttributeRanges(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWREG_CONFIG_ITEM pConfig,
    DWORD dwConfigEntries
    )
{
    PLWREG_CURRENT_VALUEINFO* DONT_RETRIEVE_CURRENT_VALUE = NULL;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwEntry = 0;
    PLWREG_CONFIG_REG pReg = NULL;

    PWSTR pwszConfigKey = NULL;
    PWSTR pwszValueName = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;

    ntStatus = NtRegOpenConfig(pszConfigKey, pszPolicyKey, &pReg);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pReg == NULL)
    {
        goto error;
    }

    ntStatus = LwRtlWC16StringAllocateFromCString(&pwszConfigKey, pReg->pszConfigKey);
    BAIL_ON_NT_STATUS(ntStatus);

    for (dwEntry = 0; dwEntry < dwConfigEntries; dwEntry++)
    {
        ntStatus = STATUS_SUCCESS;
    
        pValueAttributes = NULL;
        if (pwszValueName) 
        {
            free(pwszValueName);
            pwszValueName = NULL;
        }
        
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
                ntStatus = LwRtlWC16StringAllocateFromCString(&pwszValueName, pConfig[dwEntry].pszName);
                if (ntStatus) 
                {
                    break;
                }

                ntStatus = NtRegGetValueAttributesW(
                        pReg->hConnection,
                        pReg->hKey,
                        pwszConfigKey,
                        pwszValueName,
                        DONT_RETRIEVE_CURRENT_VALUE,
                        &pValueAttributes
                        );

                if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
                {
                    ntStatus = STATUS_SUCCESS;
                } 

                if (ntStatus) 
                {
                    break;
                }

                ntStatus = NtRegReadConfigDword(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            ((pValueAttributes && pValueAttributes->RangeType == LWREG_VALUE_RANGE_TYPE_INTEGER) 
                                ? pValueAttributes->Range.RangeInteger.Min
                                : pConfig[dwEntry].dwMin),
                            ((pValueAttributes && pValueAttributes->RangeType == LWREG_VALUE_RANGE_TYPE_INTEGER) 
                                ? pValueAttributes->Range.RangeInteger.Max
                                : pConfig[dwEntry].dwMax),
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
    if (pwszValueName) 
    {
        free(pwszValueName);
        pwszValueName = NULL;
    }

    if (pwszConfigKey) 
    {
        free(pwszConfigKey);
        pwszConfigKey = NULL;
    }

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
    DWORD dwType;
    DWORD dwSize = 0;

    if ( bUsePolicy )
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    NULL,
                    &dwSize);
        if (!ntStatus)
        {
            if (dwSize > 0)
            {
                ntStatus = LW_RTL_ALLOCATE(&pszValue, char, dwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = NtRegGetValueA(
                            pReg->hConnection,
                            pReg->hKey,
                            pReg->pszPolicyKey,
                            pszName,
                            RRF_RT_REG_SZ,
                            &dwType,
                            pszValue,
                            &dwSize);
            }
        }

        if (!ntStatus && dwSize > 0)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue )
    {
        LW_RTL_FREE(&pszValue);
        dwSize = 0;
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    NULL,
                    &dwSize);
        if (!ntStatus)
        {
            if (dwSize > 0)
            {
                ntStatus = LW_RTL_ALLOCATE(&pszValue, char, dwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = NtRegGetValueA(
                            pReg->hConnection,
                            pReg->hKey,
                            pReg->pszConfigKey,
                            pszName,
                            RRF_RT_REG_SZ,
                            &dwType,
                            pszValue,
                            &dwSize);
            }
        }

        if (!ntStatus && dwSize > 0)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
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
    DWORD dwType = 0;
    DWORD dwSize = 0;

    if (bUsePolicy)
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    pszValue,
                    &dwSize);
        if (!ntStatus)
        {
            if (dwSize > 0)
            {
                ntStatus = LW_RTL_ALLOCATE(&pszValue, char, dwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = NtRegGetValueA(
                            pReg->hConnection,
                            pReg->hKey,
                            pReg->pszPolicyKey,
                            pszName,
                            RRF_RT_REG_MULTI_SZ,
                            &dwType,
                            pszValue,
                            &dwSize);
            }
        }

        if (!ntStatus && dwSize > 0)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue )
    {
        LW_RTL_FREE(&pszValue);
        dwSize = 0;
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    pszValue,
                    &dwSize);
        if (!ntStatus)
        {
            if (dwSize > 0)
            {
                ntStatus = LW_RTL_ALLOCATE(&pszValue, char, dwSize);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = NtRegGetValueA(
                            pReg->hConnection,
                            pReg->hKey,
                            pReg->pszConfigKey,
                            pszName,
                            RRF_RT_REG_MULTI_SZ,
                            &dwType,
                            pszValue,
                            &dwSize);
            }
        }

        if (!ntStatus && dwSize > 0)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
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


/**
 * \brief Reads the specified DWORD value from the 
 * registry and returns it in pdwValue.
 *
 * n.b. the value is clamped to the range specified
 * by dwMin .. dwMax.
 */
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
        /* clamp the value to the specified range */
        if (dwMin > dwValue) {
            dwValue = dwMin;
        }

        if (dwMax < dwValue) {
            dwValue = dwMax;
        }

        *pdwValue = dwValue;
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
RegUpdateConfigItemRange(
    IN PCSTR pszConfigKey,
    IN OUT PLWREG_CONFIG_ITEM pConfig,
    IN DWORD dwConfigEntries
    )
{
    return RegNtStatusToWin32Error(
            NtRegUpdateConfigItemRange(
                pszConfigKey,
                pConfig,
                dwConfigEntries));
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

DWORD
RegProcessConfigUsingAttributeRanges(
    IN PCSTR pszConfigKey,
    IN PCSTR pszPolicyKey,
    IN OUT PLWREG_CONFIG_ITEM pConfig,
    IN DWORD dwConfigEntries
    )
{
    return RegNtStatusToWin32Error(
            NtRegProcessConfigUsingAttributeRanges(
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
