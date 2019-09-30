/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwntreg.h
 *
 * Abstract:
 *
 *        NtRegistry
 *
 *        Public NT Client API
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __LWNTREG_H__
#define __LWNTREG_H__

#include <reg/reg.h>

NTSTATUS
LwNtRegOpenServer(
    OUT PHANDLE phConnection
    );

VOID
LwNtRegCloseServer(
    IN HANDLE hConnection
    );

NTSTATUS
LwNtRegEnumRootKeysA(
    IN HANDLE hNtRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

NTSTATUS
LwNtRegEnumRootKeysW(
    IN HANDLE hNtRegConnection,
    OUT PWSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

NTSTATUS
LwNtRegCreateKeyExA(
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

NTSTATUS
LwNtRegCreateKeyExW(
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

NTSTATUS
LwNtRegCloseKey(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey
    );

NTSTATUS
LwNtRegDeleteKeyA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey
    );

NTSTATUS
LwNtRegDeleteKeyW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    );

NTSTATUS
LwNtRegDeleteKeyValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName
    );

NTSTATUS
LwNtRegDeleteKeyValueW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    );

NTSTATUS
LwNtRegDeleteTreeA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey
    );

NTSTATUS
LwNtRegDeleteTreeW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    );

NTSTATUS
LwNtRegDeleteValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCSTR pszValueName
    );

NTSTATUS
LwNtRegDeleteValueW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    );

NTSTATUS
LwNtRegEnumKeyExA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PSTR pszName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT OPTIONAL PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

NTSTATUS
LwNtRegEnumKeyExW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

NTSTATUS
LwNtRegEnumValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName,
    IN OUT PDWORD pcchValueName,
    IN OPTIONAL PDWORD pReserved,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegEnumValueW(
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

NTSTATUS
LwNtRegGetValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pSubKey,
    IN OPTIONAL PCSTR pszValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegGetValueW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegOpenKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

NTSTATUS
LwNtRegOpenKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

NTSTATUS
LwNtRegQueryInfoKeyA(
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

NTSTATUS
LwNtRegQueryInfoKeyW(
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

NTSTATUS
LwNtRegQueryValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegQueryValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegSetValueExA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    );

NTSTATUS
LwNtRegSetValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    );

NTSTATUS
LwNtRegQueryMultipleValues(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD dwTotsize
    );

//
// Registry ACL APIs
//
NTSTATUS
LwNtRegSetKeySecurity(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    );

NTSTATUS
LwNtRegGetKeySecurity(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN OUT PULONG lpcbSecurityDescriptor
    );


//
// NtRegistry multi-str data type conversion functions
//
NTSTATUS
LwNtRegMultiStrsToByteArrayW(
    IN PWSTR*   ppwszInMultiSz,
    OUT PBYTE*   ppOutBuf,
    OUT SSIZE_T* pOutBufLen
    );

NTSTATUS
LwNtRegMultiStrsToByteArrayA(
    IN PSTR*    ppszInMultiSz,
    OUT PBYTE*   ppOutBuf,
    OUT SSIZE_T* pOutBufLen
    );

NTSTATUS
LwNtRegByteArrayToMultiStrsW(
    IN PBYTE   pInBuf,
    IN SSIZE_T bufLen,
    OUT PWSTR** pppwszStrings
    );

NTSTATUS
LwNtRegByteArrayToMultiStrsA(
    IN PBYTE   pInBuf,
    IN SSIZE_T bufLen,
    OUT PSTR**  pppszStrings
    );

NTSTATUS
LwNtRegConvertByteStreamA2W(
    IN const PBYTE pData,
    IN DWORD       cbData,
    OUT PBYTE*      ppOutData,
    OUT PDWORD      pcbOutDataLen
    );

NTSTATUS
LwNtRegConvertByteStreamW2A(
    IN const PBYTE pData,
    IN DWORD       cbData,
    OUT PBYTE*      ppOutData,
    OUT PDWORD      pcbOutDataLen
    );


//
// Registry value attributes APIs
//
NTSTATUS
LwNtRegSetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    IN PLWREG_VALUE_ATTRIBUTES_A pValueAttributes
    );

NTSTATUS
LwNtRegSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );

NTSTATUS
LwNtRegGetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES_A* ppValueAttributes
    );

NTSTATUS
LwNtRegGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );

NTSTATUS
LwNtRegDeleteValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName
    );

NTSTATUS
LwNtRegDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    );

/**
 * @brief Update config item ranges with registry attributes.
 *
 * Update the ranges for each config item with the ranges 
 * defined in the registry for that value. Only applies to 
 * integer ranges.
 *
 * This is typically used in conjunction with 
 * LwNtRegProcessConfig()
 *
 * @param pszConfigKey the path to the registry key
 * @param pConfig an array of config item, registry values 
 *  under the config key
 * @param dwConfigEntries the number of config entries
 *
 * @return STATUS_SUCCESS, or appropriate error.
 */
NTSTATUS
LwNtRegUpdateConfigItemRange(
    IN PCSTR pszConfigKey,
    IN OUT PLWREG_CONFIG_ITEM pConfig,
    IN DWORD dwConfigEntries
    );

/**
 * Read configuration values from the registry
 *
 * This function loops through a configuration table reading all given values
 * from a registry key.  If an entry is not found in the registry
 * @dwConfigEntries determines whether an error is returned.
 *
 * @deprecated new code should use LwNtRegProcessConfigUsingAttributeRanges()
 *  as this function does NOT restrict config values based on 
 *  acceptable ranges defined in the registry
 *
 * @param[in] pszConfigKey Registry key path
 * @param[in] pszPolicyKey Registry policy key path
 * @param[in] pConfig Configuration table specifying parameter names
 * @param[in] dwConfigEntries Number of table entries
 *
 * @return STATUS_SUCCESS, or appropriate error.
 */
NTSTATUS
LwNtRegProcessConfig(
    IN PCSTR pszConfigKey,
    IN PCSTR pszPolicyKey,
    IN OUT PLWREG_CONFIG_ITEM pConfig,
    IN DWORD dwConfigEntries
    );

/**
 * Read configuration values from the registry
 *
 * This function loops through a configuration table reading all given values
 * from a registry key.  If an entry is not found in the registry 
 * no error is returned. 
 *
 * For REG_DWORD values with defined integer ranges, the returned
 * value will be clamped to the range.
 *
 * This assumes ranges are defined on the pszConfigKey value and NOT
 * on the pszPolicyKey value.
 *
 * @param[in] pszConfigKey Registry key path
 * @param[in] pszPolicyKey Registry policy key path
 * @param[in] pConfig Configuration table specifying parameter names
 * @param[in] dwConfigEntries Number of table entries
 *
 * @return STATUS_SUCCESS, or appropriate error.
 */
NTSTATUS
LwNtRegProcessConfigUsingAttributeRanges(
    IN PCSTR pszConfigKey,
    IN PCSTR pszPolicyKey,
    IN OUT PLWREG_CONFIG_ITEM pConfig,
    IN DWORD dwConfigEntries
    );

#ifndef LW_STRICT_NAMESPACE
#define NtRegOpenServer LwNtRegOpenServer
#define NtRegCloseServer LwNtRegCloseServer
#define NtRegEnumRootKeysA LwNtRegEnumRootKeysA
#define NtRegEnumRootKeysW LwNtRegEnumRootKeysW
#define NtRegEnumRootKeys LwNtRegEnumRootKeys
#define NtRegCreateKeyExA LwNtRegCreateKeyExA
#define NtRegCreateKeyExW LwNtRegCreateKeyExW
#define NtRegCreateKeyEx LwNtRegCreateKeyEx
#define NtRegCloseKey LwNtRegCloseKey
#define NtRegDeleteKeyA LwNtRegDeleteKeyA
#define NtRegDeleteKeyW LwNtRegDeleteKeyW
#define NtRegDeleteKey LwNtRegDeleteKey
#define NtRegDeleteKeyValueA LwNtRegDeleteKeyValueA
#define NtRegDeleteKeyValueW LwNtRegDeleteKeyValueW
#define NtRegDeleteKeyValue LwNtRegDeleteKeyValue
#define NtRegDeleteTreeA LwNtRegDeleteTreeA
#define NtRegDeleteTreeW LwNtRegDeleteTreeW
#define NtRegDeleteTree LwNtRegDeleteTree
#define NtRegDeleteValueA LwNtRegDeleteValueA
#define NtRegDeleteValueW LwNtRegDeleteValueW
#define NtRegDeleteValue LwNtRegDeleteValue
#define NtRegQueryValueExA LwNtRegQueryValueExA
#define NtRegQueryValueExW LwNtRegQueryValueExW
#define NtRegQueryValueEx LwNtRegQueryValueEx
#define NtRegEnumKeyExA LwNtRegEnumKeyExA
#define NtRegEnumKeyExW LwNtRegEnumKeyExW
#define NtRegEnumKeyEx LwNtRegEnumKeyEx
#define NtRegEnumValueA LwNtRegEnumValueA
#define NtRegEnumValueW LwNtRegEnumValueW
#define NtRegEnumValue LwNtRegEnumValue
#define NtRegGetValueA LwNtRegGetValueA
#define NtRegGetValueW LwNtRegGetValueW
#define NtRegGetValue LwNtRegGetValue
#define NtRegOpenKeyExA LwNtRegOpenKeyExA
#define NtRegOpenKeyExW LwNtRegOpenKeyExW
#define NtRegOpenKeyEx LwNtRegOpenKeyEx
#define NtRegQueryInfoKeyA LwNtRegQueryInfoKeyA
#define NtRegQueryInfoKeyW LwNtRegQueryInfoKeyW
#define NtRegQueryInfoKey LwNtRegQueryInfoKey
#define NtRegSetValueExA LwNtRegSetValueExA
#define NtRegSetValueExW LwNtRegSetValueExW
#define NtRegSetValueEx LwNtRegSetValueEx
#define NtRegMultiStrsToByteArrayA LwNtRegMultiStrsToByteArrayA
#define NtRegMultiStrsToByteArrayW LwNtRegMultiStrsToByteArrayW
#define NtRegMultiStrsToByteArray LwNtRegMultiStrsToByteArray
#define NtRegByteArrayToMultiStrsA LwNtRegByteArrayToMultiStrsA
#define NtRegByteArrayToMultiStrsW LwNtRegByteArrayToMultiStrsW
#define NtRegByteArrayToMultiStrs LwNtRegByteArrayToMultiStrs
#define NtRegConvertByteStreamA2W LwNtRegConvertByteStreamA2W
#define NtRegConvertByteStreamW2A LwNtRegConvertByteStreamW2A
#define NtRegQueryMultipleValues LwNtRegQueryMultipleValues
#define NtRegSetKeySecurity LwNtRegSetKeySecurity
#define NtRegGetKeySecurity LwNtRegGetKeySecurity


#define NtRegSetValueAttributesA LwNtRegSetValueAttributesA
#define NtRegSetValueAttributesW LwNtRegSetValueAttributesW
#define NtRegGetValueAttributesA LwNtRegGetValueAttributesA
#define NtRegGetValueAttributesW LwNtRegGetValueAttributesW
#define NtRegDeleteValueAttributesA LwNtRegDeleteValueAttributesA
#define NtRegDeleteValueAttributesW LwNtRegDeleteValueAttributesW


#define NtRegUpdateConfigItemRange LwNtRegUpdateConfigItemRange
#define NtRegProcessConfig LwNtRegProcessConfig
#define NtRegProcessConfigUsingAttributeRanges LwNtRegProcessConfigUsingAttributeRanges

#endif /* ! LW_STRICT_NAMESPACE */


#ifdef UNICODE

#define LwNtRegEnumRootKeys(hNtRegConnection, pppwszRootKeyNames, pdwNumRootKeys) \
    LwNtRegEnumRootKeysW(hNtRegConnection, pppwszRootKeyNames, pdwNumRootKeys)

#define LwNtRegCreateKeyEx(hNtRegConnection, hKey, pwszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    LwNtRegCreateKeyExW(hNtRegConnection, hKey, pwszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition)

#define LwNtRegEnumKeyEx(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \
    LwNtRegEnumKeyExW(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \

#define LwNtRegEnumValue(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    LwNtRegEnumValueW(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)

#define LwNtRegGetValue(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    LwNtRegGetValueW(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)

#define LwNtRegQueryValueEx(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    LwNtRegQueryValueExW(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)

#define LwNtRegSetValueEx(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    LwNtRegSetValueExW(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)

#define LwNtRegOpenKeyEx(hNtRegConnection, hKey, pwszSubKey, ulOptions, AccessDesired, phkResult) \
    LwNtRegOpenKeyExW(hNtRegConnection, hKey, pwszSubKey, ulOptions, AccessDesired, phkResult)

#define LwNtRegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    LwNtRegMultiStrsToByteArrayW(ppszInMultiSz, outBuf, outBufLen)

#define LwNtRegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    LwNtRegByteArrayToMultiStrsW(pInBuf, bufLen, pppszOutMultiSz)

#define LwNtRegQueryInfoKey(hNtRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    LwNtRegQueryInfoKeyW(hNtRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)

#define LwNtRegDeleteKey(hNtRegConnection, hKey, pwszSubKey) \
    LwNtRegDeleteKeyW(hNtRegConnection, hKey, pwszSubKey)

#define LwNtRegDeleteKeyValue(hNtRegConnection, hKey, pwszSubKey, pwszValueName) \
    LwNtRegDeleteKeyValueW(hNtRegConnection, hKNtRegCloseKeyey, pwszSubKey, pwszValueName)

#define LwNtRegDeleteTree(hNtRegConnection, hKey, pwszSubKey) \
    LwNtRegDeleteTreeW(hNtRegConnection, hKey, pwszSubKey)

#define LwNtRegDeleteValue(hNtRegConnection, hKey, pwszValueName) \
    LwNtRegDeleteValueW(hNtRegConnection, hKey, pwszValueName)


#define LwNtRegSetValueAttributes(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes) \
    LwNtRegSetValueAttributesW(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes)

#define LwNtRegGetValueAttributes(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes) \
    LwNtRegGetValueAttributesW(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes)

#define LwNtRegDeleteValueAttributes(hRegConnection, hKey, pszSubKey, pszValueName) \
    LwNtRegDeleteValueAttributesW(hRegConnection, hKey, pszSubKey, pszValueName)

#else

#define LwNtRegEnumRootKeys(hNtRegConnection, pppszRootKeyNames, pdwNumRootKeys) \
	LwNtRegEnumRootKeysA(hNtRegConnection, pppszRootKeyNames, pdwNumRootKeys)

#define LwNtRegCreateKeyEx(hNtRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    LwNtRegCreateKeyExA(hNtRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, AccessDesired, pSecurityAttributes, phkResult, pdwDisposition)

#define LwNtRegEnumKeyEx(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \
    LwNtRegEnumKeyExA(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \

#define LwNtRegEnumValue(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    LwNtRegEnumValueA(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)

#define LwNtRegGetValue(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    LwNtRegGetValueA(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)

#define LwNtRegQueryValueEx(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    LwNtRegQueryValueExA(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)

#define LwNtRegSetValueEx(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    LwNtRegSetValueExA(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)

#define LwNtRegOpenKeyEx(hNtRegConnection, hKey, pszSubKey, ulOptions, AccessDesired, phkResult) \
    LwNtRegOpenKeyExA(hNtRegConnection, hKey, pszSubKey, ulOptions, AccessDesired, phkResult)

#define LwNtRegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    LwNtRegMultiStrsToByteArrayA(ppszInMultiSz, outBuf, outBufLen)

#define LwNtRegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    LwNtRegByteArrayToMultiStrsA(pInBuf, bufLen, pppszOutMultiSz)

#define LwNtRegQueryInfoKey(hNtRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    LwNtRegQueryInfoKeyA(hNtRegConnection, hKey, pszClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)

#define LwNtRegDeleteKey(hNtRegConnection, hKey, pszSubKey) \
    LwNtRegDeleteKeyA(hNtRegConnection, hKey, pszSubKey)

#define LwNtRegDeleteKeyValue(hNtRegConnection, hKey, pszSubKey, pszValueName) \
    LwNtRegDeleteKeyValueA(hNtRegConnection, hKey, pszSubKey, pszValueName)

#define LwNtRegDeleteTree(hNtRegConnection, hKey, pszSubKey) \
    LwNtRegDeleteTreeA(hNtRegConnection, hKey, pszSubKey)

#define LwNtRegDeleteValue(hNtRegConnection, hKey, pszValueName) \
    LwNtRegDeleteValueA(hNtRegConnection, hKey, pszValueName)

#define LwNtRegSetValueAttributes(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes) \
    LwNtRegSetValueAttributesA(hRegConnection, hKey, pSubKey, pValueName, pValueAttributes)

#define LwNtRegGetValueAttributes(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes) \
    LwNtRegGetValueAttributesA(hRegConnection, hKey, pwszSubKey, pwszValueName, ppCurrentValue, ppValueAttributes)

#define LwNtRegDeleteValueAttributes(hRegConnection, hKey, pszSubKey, pszValueName) \
    LwNtRegDeleteValueAttributesA(hRegConnection, hKey, pszSubKey, pszValueName)


#endif


#endif /* __LWNTREG_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
