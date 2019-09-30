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
 *        clientipc_p.h
 *
 * Abstract:
 *
 *        Registry Subsystem
 *
 *        Private Header (Library)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#ifndef __CLIENTIPC_P_H__
#define __CLIENTIPC_P_H__

#include <lwmsg/lwmsg.h>

typedef struct __REG_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgPeer* pClient;
    LWMsgSession* pSession;
} REG_CLIENT_CONNECTION_CONTEXT, *PREG_CLIENT_CONNECTION_CONTEXT;

NTSTATUS
RegTransactEnumRootKeysW(
    IN HANDLE hConnection,
    OUT PWSTR** pppwszRootKeyNames,
    OUT PDWORD pdwNumRootKey
    );

NTSTATUS
RegTransactOpenKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

NTSTATUS
RegTransactCreateKeyExW(
    IN HANDLE hConnection,
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
RegTransactCloseKey(
    IN HANDLE Handle,
    IN HKEY hKey
    );

NTSTATUS
RegTransactDeleteKeyW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    );

NTSTATUS
RegTransactQueryInfoKeyW(
    IN HANDLE Handle,
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
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

NTSTATUS
RegTransactEnumKeyExW(
    IN HANDLE hConnection,
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
RegTransactDeleteKeyValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    );

NTSTATUS
RegTransactDeleteTreeW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    );

NTSTATUS
RegTransactDeleteValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    );

NTSTATUS
RegTransactEnumValueW(
    IN HANDLE hConnection,
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
RegTransactGetValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
RegTransactSetValueExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    );

NTSTATUS
RegTransactSetKeySecurity(
	IN HANDLE hNtRegConnection,
	IN HKEY hKey,
	IN SECURITY_INFORMATION SecurityInformation,
	IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
	IN ULONG Length
	);

NTSTATUS
RegTransactGetKeySecurity(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN OUT PULONG lpcbSecurityDescriptor
    );

NTSTATUS
RegTransactQueryMultipleValues(
    IN HANDLE hConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD pdwTotsize
    );

NTSTATUS
RegTransactSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );

NTSTATUS
RegTransactGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );

NTSTATUS
RegTransactDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    );

#endif /* __CLIENTIPC_P_H__ */

