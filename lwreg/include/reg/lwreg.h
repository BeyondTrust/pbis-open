/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwreg.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Public Win32 Client API
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 */


#ifndef __LWREG_H__
#define __LWREG_H__

#include <reg/reg.h>
#include <reg/lwntreg.h>

size_t
LwRegGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    );

// Registry Client Side APIs
DWORD
LwRegOpenServer(
    OUT PHANDLE phConnection
    );

VOID
LwRegCloseServer(
    IN HANDLE hConnection
    );

DWORD
LwRegEnumRootKeysA(
    IN HANDLE hRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

DWORD
LwRegEnumRootKeysW(
    IN HANDLE hRegConnection,
    OUT PWSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

DWORD
LwRegCreateKeyExA(
	IN HANDLE hRegConnection,
	IN HKEY hKey,
	IN PCSTR pszSubKey,
	IN DWORD Reserved,
	IN OPTIONAL PWSTR pClass,
	IN DWORD dwOptions,
	IN ACCESS_MASK AccessDesired,
	IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
	OUT PHKEY phkResult,
	OUT OPTIONAL PDWORD pdwDisposition
	);

DWORD
LwRegCreateKeyExW(
	IN HANDLE hRegConnection,
	IN HKEY hKey,
	IN PCWSTR pSubKey,
	IN DWORD Reserved,
	IN OPTIONAL PWSTR pClass,
	IN DWORD dwOptions,
	IN ACCESS_MASK AccessDesired,
	IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
	OUT PHKEY phkResult,
	OUT OPTIONAL PDWORD pdwDisposition
	);

DWORD
LwRegCloseKey(
    IN HANDLE hRegConnection,
    IN HKEY hKey
	);

DWORD
LwRegDeleteKeyA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey
    );

DWORD
LwRegDeleteKeyW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    );

DWORD
LwRegDeleteKeyValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName
    );

DWORD
LwRegDeleteKeyValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    );

DWORD
LwRegDeleteTreeA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey
    );

DWORD
LwRegDeleteTreeW(
	IN HANDLE hRegConnection,
	IN HKEY hKey,
	IN OPTIONAL PCWSTR pSubKey
	);

DWORD
LwRegDeleteValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszValueName
    );

DWORD
LwRegDeleteValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    );

DWORD
LwRegEnumKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PSTR pszName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT OPTIONAL PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

DWORD
LwRegEnumKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

DWORD
LwRegEnumValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName,
    IN OUT PDWORD pcchValueName,
    IN OPTIONAL PDWORD pReserved,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
LwRegEnumValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
LwRegGetValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pSubKey,
    IN OPTIONAL PCSTR pszValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
LwRegGetValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
LwRegOpenKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

DWORD
LwRegOpenKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

DWORD
LwRegQueryInfoKeyA(
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
    );

DWORD
LwRegQueryInfoKeyW(
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
    );

DWORD
LwRegQueryValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
LwRegQueryValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
LwRegSetValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    );

DWORD
LwRegSetValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    );

DWORD
LwRegQueryMultipleValues(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD dwTotsize
    );

/* registry multi-str data type conversion functions */
DWORD
LwRegMultiStrsToByteArrayW(
    IN PWSTR*   ppwszInMultiSz,
    OUT PBYTE*   ppOutBuf,
    OUT SSIZE_T* pOutBufLen
    );

DWORD
LwRegMultiStrsToByteArrayA(
    IN PSTR*    ppszInMultiSz,
    OUT PBYTE*   ppOutBuf,
    OUT SSIZE_T* pOutBufLen
    );

DWORD
LwRegByteArrayToMultiStrsW(
    IN PBYTE   pInBuf,
    IN SSIZE_T bufLen,
    OUT PWSTR** pppwszStrings
    );

DWORD
LwRegByteArrayToMultiStrsA(
    IN PBYTE   pInBuf,
    IN SSIZE_T bufLen,
    OUT PSTR**  pppszStrings
    );

DWORD
LwRegConvertByteStreamA2W(
    IN const PBYTE pData,
    IN DWORD       cbData,
    OUT PBYTE*      ppOutData,
    OUT PDWORD      pcbOutDataLen
    );

DWORD
LwRegConvertByteStreamW2A(
    IN const PBYTE pData,
    IN DWORD       cbData,
    OUT PBYTE*      ppOutData,
    OUT PDWORD      pcbOutDataLen
    );

// Registry ACL APIs
DWORD
LwRegSetKeySecurity(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    );

DWORD
LwRegGetKeySecurity(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN OUT PULONG lpcbSecurityDescriptor
    );

DWORD
LwRegFindHintByName(
    PSTR pszHint
    );

PSTR
LwRegFindHintByValue(
    DWORD dwHint
    );


//
// Registry value attributes APIs
//
DWORD
LwRegSetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    IN PLWREG_VALUE_ATTRIBUTES_A pValueAttributes
    );

DWORD
LwRegSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );

DWORD
LwRegGetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES_A* ppValueAttributes
    );

DWORD
LwRegGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );

DWORD
LwRegDeleteValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName
    );

DWORD
LwRegDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    );


#ifndef LW_STRICT_NAMESPACE
#define RegOpenServer LwRegOpenServer
#define RegCloseServer LwRegCloseServer
#define RegEnumRootKeysA LwRegEnumRootKeysA
#define RegEnumRootKeysW LwRegEnumRootKeysW
#define RegEnumRootKeys LwRegEnumRootKeys
#define RegCreateKeyExA LwRegCreateKeyExA
#define RegCreateKeyExW LwRegCreateKeyExW
#define RegCreateKeyEx LwRegCreateKeyEx
#define RegCloseKey LwRegCloseKey
#define RegDeleteKeyA LwRegDeleteKeyA
#define RegDeleteKeyW LwRegDeleteKeyW
#define RegDeleteKey LwRegDeleteKey
#define RegDeleteKeyValueA LwRegDeleteKeyValueA
#define RegDeleteKeyValueW LwRegDeleteKeyValueW
#define RegDeleteKeyValue LwRegDeleteKeyValue
#define RegDeleteTreeA LwRegDeleteTreeA
#define RegDeleteTreeW LwRegDeleteTreeW
#define RegDeleteTree LwRegDeleteTree
#define RegDeleteValueA LwRegDeleteValueA
#define RegDeleteValueW LwRegDeleteValueW
#define RegDeleteValue LwRegDeleteValue
#define RegQueryValueExA LwRegQueryValueExA
#define RegQueryValueExW LwRegQueryValueExW
#define RegQueryValueEx LwRegQueryValueEx
#define RegEnumKeyExA LwRegEnumKeyExA
#define RegEnumKeyExW LwRegEnumKeyExW
#define RegEnumKeyEx LwRegEnumKeyEx
#define RegEnumValueA LwRegEnumValueA
#define RegEnumValueW LwRegEnumValueW
#define RegEnumValue LwRegEnumValue
#define RegGetValueA LwRegGetValueA
#define RegGetValueW LwRegGetValueW
#define RegGetValue LwRegGetValue
#define RegOpenKeyExA LwRegOpenKeyExA
#define RegOpenKeyExW LwRegOpenKeyExW
#define RegOpenKeyEx LwRegOpenKeyEx
#define RegQueryInfoKeyA LwRegQueryInfoKeyA
#define RegQueryInfoKeyW LwRegQueryInfoKeyW
#define RegQueryInfoKey LwRegQueryInfoKey
#define RegSetValueExA LwRegSetValueExA
#define RegSetValueExW LwRegSetValueExW
#define RegSetValueEx LwRegSetValueEx
#define RegMultiStrsToByteArrayA LwRegMultiStrsToByteArrayA
#define RegMultiStrsToByteArrayW LwRegMultiStrsToByteArrayW
#define RegMultiStrsToByteArray LwRegMultiStrsToByteArray
#define RegByteArrayToMultiStrsA LwRegByteArrayToMultiStrsA
#define RegByteArrayToMultiStrsW LwRegByteArrayToMultiStrsW
#define RegByteArrayToMultiStrs LwRegByteArrayToMultiStrs
#define RegConvertByteStreamA2W LwRegConvertByteStreamA2W
#define RegConvertByteStreamW2A LwRegConvertByteStreamW2A
#define RegQueryMultipleValues LwRegQueryMultipleValues
#define RegSetKeySecurity LwRegSetKeySecurity
#define RegGetKeySecurity LwRegGetKeySecurity
#define RegFindHintByName LwRegFindHintByName
#define RegFindHintByValue LwRegFindHintByValue

#define RegSetValueAttributesA LwRegSetValueAttributesA
#define RegSetValueAttributesW LwRegSetValueAttributesW
#define RegGetValueAttributesA LwRegGetValueAttributesA
#define RegGetValueAttributesW LwRegGetValueAttributesW
#define RegDeleteValueAttributesA LwRegDeleteValueAttributesA
#define RegDeleteValueAttributesW LwRegDeleteValueAttributesW

#endif /* ! LW_STRICT_NAMESPACE */


#ifdef UNICODE

#define LwRegEnumRootKeys(hRegConnection, pppwszRootKeyNames, pdwNumRootKeys) \
    LwRegEnumRootKeysW(hRegConnection, pppwszRootKeyNames, pdwNumRootKeys)

#define LwRegCreateKeyEx(hRegConnection, hKey, pwszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    LwRegCreateKeyExW(hRegConnection, hKey, pwszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition)

#define LwRegEnumKeyEx(hRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \
    LwRegEnumKeyExW(hRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \

#define LwRegEnumValue(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    LwRegEnumValueW(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)

#define LwRegGetValue(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    LwRegGetValueW(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)

#define LwRegQueryValueEx(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    LwRegQueryValueExW(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)

#define LwRegSetValueEx(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    LwRegSetValueExW(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)

#define LwRegOpenKeyEx(hRegConnection, hKey, pwszSubKey, ulOptions, AccessDesired, phkResult) \
    LwRegOpenKeyExW(hRegConnection, hKey, pwszSubKey, ulOptions, AccessDesired, phkResult)

#define LwRegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    LwRegMultiStrsToByteArrayW(ppszInMultiSz, outBuf, outBufLen)

#define LwRegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    LwRegByteArrayToMultiStrsW(pInBuf, bufLen, pppszOutMultiSz)

#define LwRegQueryInfoKey(hRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    LwRegQueryInfoKeyW(hRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)

#define LwRegDeleteKey(hRegConnection, hKey, pwszSubKey) \
    LwRegDeleteKeyW(hRegConnection, hKey, pwszSubKey)

#define LwRegDeleteKeyValue(hRegConnection, hKey, pwszSubKey, pwszValueName) \
    LwRegDeleteKeyValueW(hRegConnection, hKRegCloseKeyey, pwszSubKey, pwszValueName)

#define LwRegDeleteTree(hRegConnection, hKey, pwszSubKey) \
    LwRegDeleteTreeW(hRegConnection, hKey, pwszSubKey)

#define LwRegDeleteValue(hRegConnection, hKey, pwszValueName) \
    LwRegDeleteValueW(hRegConnection, hKey, pwszValueName)

#define LwRegSetValueAttributes(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes) \
    LwRegSetValueAttributesW(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes)

#define LwRegGetValueAttributes(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes) \
    LwRegGetValueAttributesW(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes)

#define LwRegDeleteValueAttributes(hRegConnection, hKey, pszSubKey, pszValueName) \
    LwRegDeleteValueAttributesW(hRegConnection, hKey, pszSubKey, pszValueName)


#else

#define LwRegEnumRootKeys(hRegConnection, pppszRootKeyNames, pdwNumRootKeys) \
	LwRegEnumRootKeysA(hRegConnection, pppszRootKeyNames, pdwNumRootKeys)

#define LwRegCreateKeyEx(hRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    LwRegCreateKeyExA(hRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition)

#define LwRegEnumKeyEx(hRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \
    LwRegEnumKeyExA(hRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \

#define LwRegEnumValue(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    LwRegEnumValueA(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)

#define LwRegGetValue(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    LwRegGetValueA(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)

#define LwRegQueryValueEx(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    LwRegQueryValueExA(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)

#define LwRegSetValueEx(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    LwRegSetValueExA(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)

#define LwRegOpenKeyEx(hRegConnection, hKey, pszSubKey, ulOptions, AccessDesired, phkResult) \
    LwRegOpenKeyExA(hRegConnection, hKey, pszSubKey, ulOptions, AccessDesired, phkResult)

#define LwRegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    LwRegMultiStrsToByteArrayA(ppszInMultiSz, outBuf, outBufLen)

#define LwRegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    LwRegByteArrayToMultiStrsA(pInBuf, bufLen, pppszOutMultiSz)

#define LwRegQueryInfoKey(hRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    LwRegQueryInfoKeyA(hRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)

#define LwRegDeleteKey(hRegConnection, hKey, pszSubKey) \
    LwRegDeleteKeyA(hRegConnection, hKey, pszSubKey)

#define LwRegDeleteKeyValue(hRegConnection, hKey, pszSubKey, pszValueName) \
    LwRegDeleteKeyValueA(hRegConnection, hKey, pszSubKey, pszValueName)

#define LwRegDeleteTree(hRegConnection, hKey, pszSubKey) \
    LwRegDeleteTreeA(hRegConnection, hKey, pszSubKey)

#define LwRegDeleteValue(hRegConnection, hKey, pszValueName) \
    LwRegDeleteValueA(hRegConnection, hKey, pszValueName)

#define LwRegSetValueAttributes(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes) \
    LwRegSetValueAttributesA(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes)

#define LwRegGetValueAttributes(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes) \
    LwRegGetValueAttributesA(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes)

#define LwRegDeleteValueAttributes(hRegConnection, hKey, pszSubKey, pszValueName) \
    LwRegDeleteValueAttributesA(hRegConnection, hKey, pszSubKey, pszValueName)

#endif


#endif /* __LWREG_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
