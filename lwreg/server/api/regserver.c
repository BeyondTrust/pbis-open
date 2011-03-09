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
 *        regserver.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "api.h"

BOOLEAN
RegSrvIsValidKeyName(
    PCWSTR pwszKeyName
    )
{
	wchar16_t wch = '\\';
	int iIndex = 0;

	if (LW_IS_NULL_OR_EMPTY_STR(pwszKeyName))
		return FALSE;

	for (; iIndex < RtlWC16StringNumChars(pwszKeyName); iIndex++)
	{
		if (pwszKeyName[iIndex] == wch)
		{
			return FALSE;
		}
	}

	return TRUE;
}

NTSTATUS
RegSrvEnumRootKeysW(
    IN HANDLE Handle,
    OUT PWSTR** pppwszRootKeys,
    OUT PDWORD pdwNumRootKeys
    )
{
	NTSTATUS status = 0;
    PWSTR* ppwszRootKeys = NULL;
    int iCount = 0;

    status = LW_RTL_ALLOCATE((PVOID*)&ppwszRootKeys, PWSTR, sizeof(*ppwszRootKeys));
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount< NUM_ROOTKEY; iCount++)
    {
    	status = LwRtlWC16StringDuplicate(&ppwszRootKeys[iCount], ROOT_KEYS[iCount]);
    	BAIL_ON_NT_STATUS(status);
    }

    *pdwNumRootKeys = NUM_ROOTKEY;
    *pppwszRootKeys = ppwszRootKeys;

cleanup:
    return status;

error:
    if (ppwszRootKeys)
    {
        for (iCount=0; iCount<NUM_ROOTKEY; iCount++)
        {
            LWREG_SAFE_FREE_MEMORY(ppwszRootKeys[iCount]);
        }
        ppwszRootKeys = NULL;
    }
    *pdwNumRootKeys = 0;
    *pppwszRootKeys = NULL;

    goto cleanup;
}

NTSTATUS
RegSrvCreateKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescLen,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    return gpRegProvider->pfnRegSrvCreateKeyEx(
                                           Handle,
                                           hKey,
                                           pSubKey,
                                           Reserved,
                                           pClass,
                                           dwOptions,
                                           AccessDesired,
                                           pSecurityDescriptor,
                                           ulSecDescLen,
                                           phkResult,
                                           pdwDisposition);
}

NTSTATUS
RegSrvOpenKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
	return gpRegProvider->pfnRegSrvOpenKeyExW(
                                 Handle,
                                 hKey,
                                 pwszSubKey,
                                 ulOptions,
                                 AccessDesired,
                                 phkResult);
}

VOID
RegSrvCloseKey(
    HKEY hKey
    )
{
    return gpRegProvider->pfnRegSrvCloseKey(hKey);
}

NTSTATUS
RegSrvDeleteKey(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    )
{
	return gpRegProvider->pfnRegSrvDeleteKey(Handle,
											 hKey,
											 pSubKey);
}

NTSTATUS
RegSrvDeleteKeyValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    )
{
	return gpRegProvider->pfnRegSrvDeleteKeyValue(Handle,
												  hKey,
												  pSubKey,
												  pValueName);
}

NTSTATUS
RegSrvDeleteValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName
    )
{
	return gpRegProvider->pfnRegSrvDeleteValue(Handle,
                                               hKey,
                                               pValueName);
}

NTSTATUS
RegSrvEnumKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return gpRegProvider->pfnRegSrvEnumKeyExW(
            Handle,
            hKey,
            dwIndex,
            pName,
            pcName,
            pReserved,
            pClass,
            pcClass,
            pftLastWriteTime);
}

NTSTATUS
RegSrvSetValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
   return gpRegProvider->pfnRegSrvSetValueExW(
            Handle,
            hKey,
            pValueName,
            Reserved,
            dwType,
            pData,
            cbData);
}

NTSTATUS
RegSrvGetValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    )
{
    return gpRegProvider->pfnRegSrvGetValueW(
            Handle,
            hKey,
            pSubKey,
            pValue,
            Flags,
            pdwType,
            pData,
            pcbData);
}

NTSTATUS
RegSrvEnumValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    )
{
    return gpRegProvider->pfnRegSrvEnumValueW(
            Handle,
            hKey,
            dwIndex,
            pValueName,
            pcchValueName,
            pReserved,
            pType,
            pData,
            pcbData);
}

NTSTATUS
RegSrvQueryInfoKeyW(
    HANDLE Handle,
    HKEY hKey,
    PWSTR pClass,
    PDWORD pcClass,
    PDWORD pReserved,
    PDWORD pcSubKeys,
    PDWORD pcMaxSubKeyLen,
    PDWORD pcMaxClassLen,
    PDWORD pcValues,
    PDWORD pcMaxValueNameLen,
    PDWORD pcMaxValueLen,
    PDWORD pcbSecurityDescriptor,
    PFILETIME pftLastWriteTime
    )
{
    return gpRegProvider->pfnRegSrvQueryInfoKeyW(
            Handle,
            hKey,
            pClass,
            pcClass,
            pReserved,
            pcSubKeys,
            pcMaxSubKeyLen,
            pcMaxClassLen,
            pcValues,
            pcMaxValueNameLen,
            pcMaxValueLen,
            pcbSecurityDescriptor,
            pftLastWriteTime);
}

NTSTATUS
RegSrvDeleteTree(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    )
{
	return gpRegProvider->pfnRegSrvDeleteTree(
            Handle,
            hKey,
            pSubKey);
}

NTSTATUS
RegSrvQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    )
{
    return gpRegProvider->pfnRegSrvQueryMultipleValues(
            Handle,
            hKey,
            pVal_list,
            num_vals,
            pValue,
            pdwTotalsize);
}


NTSTATUS
RegSrvSetKeySecurity(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescLength
    )
{
    return gpRegProvider->pfnRegSrvSetKeySecurity(
    		Handle,
    		hKey,
    		SecurityInformation,
    		pSecurityDescriptor,
    		ulSecDescLength);
}

NTSTATUS
RegSrvGetKeySecurity(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN OUT PULONG pulSecDescLength
    )
{
    return gpRegProvider->pfnRegSrvGetKeySecurity(
    		Handle,
    		hKey,
    		SecurityInformation,
    		pSecurityDescriptor,
    		pulSecDescLength);
}

//
// Registry Value Attributes Server APIs
//
NTSTATUS
RegSrvSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
   return gpRegProvider->pfnRegSrvSetValueAttributes(
           hRegConnection,
            hKey,
            pSubKey,
            pValueName,
            pValueAttributes);
}

NTSTATUS
RegSrvGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
   return gpRegProvider->pfnRegSrvGetValueAttributes(
           hRegConnection,
            hKey,
            pwszSubKey,
            pwszValueName,
            ppCurrentValue,
            ppValueAttributes);
}

NTSTATUS
RegSrvDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    )
{
    return gpRegProvider->pfnRegSrvDeleteValueAttributes(
            hRegConnection,
             hKey,
             pwszSubKey,
             pwszValueName);
}

