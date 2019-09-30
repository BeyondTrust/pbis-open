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
 *        regserver.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#ifndef __REGSERVER_H_
#define __REGSERVER_H_

typedef struct __REG_SRV_API_STATE
{
    uid_t  peerUID;
    gid_t  peerGID;
    PACCESS_TOKEN pToken;
    HANDLE hEventLog;
} REG_SRV_API_STATE, *PREG_SRV_API_STATE;

// Incomplete type here, as the context implementation is provider specific
typedef struct __REG_KEY_CONTEXT *PREG_KEY_CONTEXT;

typedef struct __REG_KEY_HANDLE
{
	ACCESS_MASK AccessGranted;
	PREG_KEY_CONTEXT pKey;

} REG_KEY_HANDLE, *PREG_KEY_HANDLE;


#define LWREG_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           REG_LOG_ERROR("Failed to lock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWREG_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           REG_LOG_ERROR("Failed to unlock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

//reader
#define LWREG_LOCK_RWMUTEX_SHARED(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_rdlock(mutex); \
       if (thr_err) { \
           REG_LOG_ERROR("Failed to acquire shared lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

 //writer
#define LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_wrlock(mutex); \
       if (thr_err) { \
           REG_LOG_ERROR("Failed to acquire exclusive lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWREG_UNLOCK_RWMUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_rwlock_unlock(mutex); \
       if (thr_err) { \
           REG_LOG_ERROR("Failed to unlock rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

void
RegSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    );

LWMsgStatus
RegSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    );

DWORD
RegSrvApiInit(
    VOID
    );

DWORD
RegSrvApiShutdown(
    VOID
    );

LWMsgDispatchSpec*
RegSrvGetDispatchSpec(
    void
    );

NTSTATUS
RegSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    );

void
RegSrvCloseServer(
    HANDLE hServer
    );

NTSTATUS
RegSrvEnumRootKeysW(
    IN HANDLE Handle,
    OUT PWSTR** pppszRootKeys,
    OUT PDWORD pdwNumRootKeys
    );

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
    );

NTSTATUS
RegSrvOpenKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

VOID
RegSrvCloseKey(
    HKEY hKey
    );

NTSTATUS
RegSrvDeleteKey(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    );

NTSTATUS
RegSrvDeleteKeyValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    );

NTSTATUS
RegSrvDeleteTree(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpSubKey
    );

NTSTATUS
RegSrvDeleteValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpValueName
    );

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
    );

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
    );

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
    );

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
    );

NTSTATUS
RegSrvQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    );

NTSTATUS
RegSrvSetValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );

NTSTATUS
RegSrvSetKeySecurity(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescLength
    );

NTSTATUS
RegSrvGetKeySecurity(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN OUT PULONG pulSecDescLength
    );
//
// Registry value attribute APIs
//
NTSTATUS
RegSrvSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );

NTSTATUS
RegSrvGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );

NTSTATUS
RegSrvDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    );

// Key context (key handle) utility functions
BOOLEAN
RegSrvIsValidKeyName(
    PCWSTR pwszKeyName
    );

//Registry ACL check

NTSTATUS
RegSrvCreateAccessToken(
    uid_t uid,
    gid_t gid,
    PACCESS_TOKEN* ppToken
    );

NTSTATUS
RegSrvAccessCheckKey(
	IN PACCESS_TOKEN pToken,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescRelLen,
    IN ACCESS_MASK AccessDesired,
    OUT ACCESS_MASK *psamGranted
    );

NTSTATUS
RegSrvAccessCheckKeyHandle(
    IN PREG_KEY_HANDLE pKeyHandle,
    IN ACCESS_MASK AccessRequired
    );

NTSTATUS
RegSrvCreateDefaultSecDescRel(
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    IN OUT PULONG pulSecDescLength
    );

VOID
RegSrvFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );

#endif // __REGSERVER_H_
