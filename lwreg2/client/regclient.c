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
 *        registry.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 *          Marc Guy (mguy@likewisesoftware.com)            
 */
#include "client.h"

REG_API
DWORD
RegEnumRootKeysA(
    IN HANDLE hRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    )
{
    return RegNtStatusToWin32Error(
    		NtRegEnumRootKeysA(hRegConnection,
    				         pppszRootKeyNames,
    				         pdwNumRootKeys)
    				         );
}

REG_API
DWORD
RegEnumRootKeysW(
    IN HANDLE hRegConnection,
    OUT PWSTR** pppwszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    )
{
    return RegNtStatusToWin32Error(
    		NtRegEnumRootKeysW(hRegConnection,
    				         pppwszRootKeyNames,
    				         pdwNumRootKeys)
    				         );
}

REG_API
DWORD
RegCreateKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    return RegNtStatusToWin32Error(
    		NtRegCreateKeyExA(
    		    hRegConnection,
    		    hKey,
    		    pszSubKey,
    		    Reserved,
    		    pClass,
    		    dwOptions,
    		    AccessDesired,
    		    pSecDescAbs,
    		    phkResult,
    		    pdwDisposition
    		    )
    		    );
}

REG_API
DWORD
RegCreateKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    return RegNtStatusToWin32Error(
    		NtRegCreateKeyExW(
    		    hRegConnection,
    		    hKey,
    		    pSubKey,
    		    Reserved,
    		    pClass,
    		    dwOptions,
    		    AccessDesired,
    		    pSecDescAbs,
    		    phkResult,
    		    pdwDisposition
    		    )
    		    );
}

REG_API
DWORD
RegCloseKey(
    IN HANDLE hRegConnection,
    IN HKEY hKey
    )
{
    if (!hRegConnection)
    {
        return RegNtStatusToWin32Error(STATUS_INVALID_PARAMETER);
    }
    return RegNtStatusToWin32Error(
    		NtRegCloseKey(
    		    hRegConnection,
    		    hKey
    		    )
    		    );
}

REG_API
DWORD
RegDeleteKeyA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteKeyA(
    		    hRegConnection,
    		    hKey,
    		    pszSubKey
    		    )
    		    );
}

REG_API
DWORD
RegDeleteKeyW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteKeyW(
    		    hRegConnection,
    		    hKey,
    		    pSubKey
    		    )
    		    );
}

REG_API
DWORD
RegDeleteKeyValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteKeyValueA(
    		    hRegConnection,
    		    hKey,
    		    pszSubKey,
    		    pszValueName
    		    )
    		    );
}

REG_API
DWORD
RegDeleteKeyValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteKeyValueW(
    		    hRegConnection,
    		    hKey,
    		    pSubKey,
    		    pValueName
    		    )
    		    );
}

REG_API
DWORD
RegDeleteTreeA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteTreeA(
    		    hRegConnection,
    		    hKey,
    		    pszSubKey
    		    )
    		    );
}

REG_API
DWORD
RegDeleteTreeW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteTreeW(
    		    hRegConnection,
    		    hKey,
    		    pSubKey
    		    )
    		    );
}

REG_API
DWORD
RegDeleteValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszValueName
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteValueA(
    		    hRegConnection,
    		    hKey,
    		    pszValueName
    		    )
    		    );
}

REG_API
DWORD
RegDeleteValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    )
{
    return RegNtStatusToWin32Error(
    		NtRegDeleteValueW(
    		    hRegConnection,
    		    hKey,
    		    pValueName
    		    )
    		    );
}

REG_API
DWORD
RegEnumKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PSTR pszName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT OPTIONAL PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return RegNtStatusToWin32Error(
    		NtRegEnumKeyExA(
    		    hRegConnection,
    		    hKey,
    		    dwIndex,
    		    pszName,
    		    pcName,
    		    pReserved,
    		    pszClass,
    		    pcClass,
    		    pftLastWriteTime
    		    )
    		    );
}

REG_API
DWORD
RegEnumKeyExW(
    IN HANDLE hRegConnection,
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
    return RegNtStatusToWin32Error(
    		NtRegEnumKeyExW(
		        hRegConnection,
				hKey,
				dwIndex,
				pName,
				pcName,
				pReserved,
				pClass,
				pcClass,
				pftLastWriteTime
				)
    		    );
}


REG_API
DWORD
RegEnumValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName,
    IN OUT PDWORD pcchValueName,
    IN OPTIONAL PDWORD pReserved,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegEnumValueA(
    	         hRegConnection,
    			 hKey,
    			 dwIndex,
    			 pszValueName,
    			 pcchValueName,
    			 pReserved,
    			 pdwType,
    			 pData,
    			 pcbData)
    		    );
}

REG_API
DWORD
RegEnumValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegEnumValueW(
    	         hRegConnection,
    			 hKey,
    			 dwIndex,
    			 pValueName,
    			 pcchValueName,
    			 pReserved,
    			 pType,
    			 pData,
    			 pcbData)
    		    );
}

REG_API
DWORD
RegGetValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegGetValueA(
    	         hRegConnection,
    			 hKey,
    			 pszSubKey,
    			 pszValueName,
    			 Flags,
    			 pdwType,
    			 pvData,
    			 pcbData)
    		    );
}

REG_API
DWORD
RegGetValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegGetValueW(
    	         hRegConnection,
    			 hKey,
    			 pSubKey,
    			 pValueName,
    			 Flags,
    			 pdwType,
    			 pvData,
    			 pcbData)
    		    );
}

REG_API
DWORD
RegOpenKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
    return RegNtStatusToWin32Error(
    		NtRegOpenKeyExA(
    	         hRegConnection,
    			 hKey,
    			 pszSubKey,
    			 ulOptions,
    			 AccessDesired,
    			 phkResult)
    		    );
}

REG_API
DWORD
RegOpenKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
    return RegNtStatusToWin32Error(
    		NtRegOpenKeyExW(
    	         hRegConnection,
    			 hKey,
    			 pSubKey,
    			 ulOptions,
    			 AccessDesired,
    			 phkResult)
    		    );
}

REG_API
DWORD
RegQueryInfoKeyA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT OPTIONAL PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PULONG pulSecDescLen,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return RegNtStatusToWin32Error(
    		NtRegQueryInfoKeyA(
    	         hRegConnection,
    			 hKey,
    			 pszClass,
    			 pcClass,
    			 pReserved,
    			 pcSubKeys,
    			 pcMaxSubKeyLen,
    			 pcMaxClassLen,
    			 pcValues,
    			 pcMaxValueNameLen,
    			 pcMaxValueLen,
    			 pulSecDescLen,
    			 pftLastWriteTime)
    		    );
}

REG_API
DWORD
RegQueryInfoKeyW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PULONG pulSecDescLen,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return RegNtStatusToWin32Error(
    		NtRegQueryInfoKeyW(
    	         hRegConnection,
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
    			 pulSecDescLen,
    			 pftLastWriteTime)
    		    );
}

REG_API
DWORD
RegQueryMultipleValues(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD dwTotsize
    )
{
    return RegNtStatusToWin32Error(
    		NtRegQueryMultipleValues(
    	         hRegConnection,
    			 hKey,
    			 val_list,
    			 num_vals,
    			 pValueBuf,
    			 dwTotsize)
    		    );
}

REG_API
DWORD
RegQueryValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegQueryValueExA(
    	         hRegConnection,
    			 hKey,
    			 pszValueName,
    			 pReserved,
    			 pType,
    			 pData,
    			 pcbData)
    		    );
}

REG_API
DWORD
RegQueryValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegQueryValueExW(
    	         hRegConnection,
    			 hKey,
    			 pValueName,
    			 pReserved,
    			 pType,
    			 pData,
    			 pcbData)
    		    );
}

REG_API
DWORD
RegSetValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegSetValueExA(
    	         hRegConnection,
    			 hKey,
    			 pszValueName,
    			 Reserved,
    			 dwType,
    			 pData,
    			 cbData)
    		    );
}

REG_API
DWORD
RegSetValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
    return RegNtStatusToWin32Error(
    		NtRegSetValueExW(
    	         hRegConnection,
    			 hKey,
    			 pValueName,
    			 Reserved,
    			 dwType,
    			 pData,
    			 cbData)
    		    );
}

REG_API
DWORD
RegSetKeySecurity(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLen
    )
{
    return RegNtStatusToWin32Error(
    		NtRegSetKeySecurity(
    	         hRegConnection,
    			 hKey,
    			 SecurityInformation,
    			 pSecDescRel,
    			 ulSecDescLen)
    		    );
}

REG_API
DWORD
RegGetKeySecurity(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN OUT PULONG pulSecDescLen
    )
{
    return RegNtStatusToWin32Error(
    		NtRegGetKeySecurity(
    	         hRegConnection,
    			 hKey,
    			 SecurityInformation,
    			 pSecDescRel,
    			 pulSecDescLen)
    		    );
}



REG_API
DWORD
LwRegSetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    IN PLWREG_VALUE_ATTRIBUTES_A pValueAttributes
    )
{
    return RegNtStatusToWin32Error(
            NtRegSetValueAttributesA(
                hRegConnection,
                hKey,
                pszSubKey,
                pszValueName,
                pValueAttributes
                )
               );
}

REG_API
DWORD
LwRegSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    return RegNtStatusToWin32Error(
            NtRegSetValueAttributesW(
                hRegConnection,
                hKey,
                pwszSubKey,
                pwszValueName,
                pValueAttributes
                )
               );
}

REG_API
DWORD
LwRegGetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES_A* ppValueAttributes
    )
{
    return RegNtStatusToWin32Error(
            NtRegGetValueAttributesA(
                hRegConnection,
                hKey,
                pszSubKey,
                pszValueName,
                ppCurrentValue,
                ppValueAttributes
                )
                );
}

REG_API
DWORD
LwRegGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    return RegNtStatusToWin32Error(
            NtRegGetValueAttributesW(
                hRegConnection,
                hKey,
                pwszSubKey,
                pwszValueName,
                ppCurrentValue,
                ppValueAttributes
                )
                );
}

REG_API
DWORD
LwRegDeleteValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName
    )
{
    return RegNtStatusToWin32Error(
            NtRegDeleteValueAttributesA(
                hRegConnection,
                hKey,
                pszSubKey,
                pszValueName
                )
                );
}

REG_API
DWORD
LwRegDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    )
{
    return RegNtStatusToWin32Error(
            NtRegDeleteValueAttributesW(
                hRegConnection,
                hKey,
                pwszSubKey,
                pwszValueName
                )
                );
}
