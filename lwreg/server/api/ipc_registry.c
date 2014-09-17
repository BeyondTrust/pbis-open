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
 *        ipc_registry.c
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
 *          Wei Fu (wfu@likewise.com)
 */
#include "api.h"

static
NTSTATUS
RegSrvIpcCheckPermissions(
    LWMsgSecurityToken* token,
    uid_t* puid,
    gid_t* pgid
    )
{
	NTSTATUS status = 0;
    uid_t euid;
    gid_t egid;

    if (strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        REG_LOG_WARNING("Unsupported authentication type");
        status = STATUS_UNHANDLED_EXCEPTION;
        BAIL_ON_NT_STATUS(status);
    }

    status = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(token, &euid, &egid));
    BAIL_ON_NT_STATUS(status);

    REG_LOG_VERBOSE("Permission granted for (uid = %i, gid = %i) to open RegIpcServer",
                    (int) euid,
                    (int) egid);

    *puid = euid;
    *pgid = egid;

error:
    return status;
}

static
NTSTATUS
RegSrvIpcRegisterHandle(
    LWMsgCall* pCall,
    PCSTR pszHandleType,
    PVOID pData,
    LWMsgHandleCleanupFunction pfnCleanup,
    LWMsgHandle** ppHandle
    )
{
	NTSTATUS status = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    status = MAP_LWMSG_ERROR(lwmsg_session_register_handle(pSession, pszHandleType, pData, pfnCleanup, ppHandle));
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

static
VOID
RegSrvIpcRetainHandle(
    LWMsgCall* pCall,
    LWMsgHandle* pHandle
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    lwmsg_session_retain_handle(pSession, pHandle);
}

static
NTSTATUS
RegSrvIpcGetHandleData(
    LWMsgCall* pCall,
    LWMsgHandle* pHandle,
    HKEY* phKey
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    LWMsgStatus status = lwmsg_session_get_handle_data(pSession, pHandle, OUT_PPVOID(phKey));

    if (status == LWMSG_STATUS_INVALID_HANDLE)
    {
        return STATUS_INVALID_HANDLE;
    }
    else
    {
        return MAP_LWMSG_ERROR(status);
    }
}

static
VOID
RegSrvIpcCloseHandle(
    PVOID pHandle
    )
{
    return RegSrvCloseKey((HKEY)pHandle);
}

static
NTSTATUS
RegSrvIpcUnregisterHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
	NTSTATUS status = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    status = MAP_LWMSG_ERROR(lwmsg_session_unregister_handle(pSession, pHandle));
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}


static
HANDLE
RegSrvIpcGetSessionData(
    LWMsgCall* pCall
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    return lwmsg_session_get_data(pSession);
}

void
RegSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    )
{
    RegSrvCloseServer(pSessionData);
}

LWMsgStatus
RegSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    )
{
	NTSTATUS status = 0;
    HANDLE Handle = (HANDLE)NULL;
    uid_t UID;
    gid_t GID;

    status = RegSrvIpcCheckPermissions(pToken, &UID, &GID);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvOpenServer(UID, GID, &Handle);
    BAIL_ON_NT_STATUS(status);

    *ppSessionData = Handle;

cleanup:

    return MAP_REG_ERROR_IPC(status);

error:

    goto cleanup;
}

NTSTATUS
RegSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    )
{
	NTSTATUS status = 0;
    PREG_SRV_API_STATE pServerState = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pServerState, REG_SRV_API_STATE, sizeof(*pServerState));
    BAIL_ON_NT_STATUS(status);

    pServerState->peerUID = peerUID;
    pServerState->peerGID = peerGID;

    *phServer = (HANDLE)pServerState;

cleanup:

    return status;

error:

    *phServer = (HANDLE)NULL;

    if (pServerState) {
        RegSrvCloseServer((HANDLE)pServerState);
    }

    goto cleanup;
}

void
RegSrvCloseServer(
    HANDLE hServer
    )
{
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)hServer;

    if (pServerState->hEventLog != (HANDLE)NULL)
    {
       //RegSrvCloseEventLog(pServerState->hEventLog);
    }

    if (pServerState->pToken)
    {
        RtlReleaseAccessToken(&pServerState->pToken);
    }

    LwRtlMemoryFree(pServerState);
}

NTSTATUS
RegSrvIpcCreateError(
    DWORD statusCode,
    PREG_IPC_STATUS* ppStatus
    )
{
	NTSTATUS status = 0;
    PREG_IPC_STATUS pStatus = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pStatus, REG_IPC_STATUS, sizeof(*pStatus));
    BAIL_ON_NT_STATUS(status);

    pStatus->status = statusCode;

    *ppStatus = pStatus;

error:
    return status;
}

LWMsgStatus
RegSrvIpcEnumRootKeysW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    NTSTATUS status = 0;
    PREG_IPC_ENUM_ROOTKEYS_RESPONSE pRegResp = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    PWSTR* ppwszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    int iCount = 0;

    status = RegSrvEnumRootKeysW(
        RegSrvIpcGetSessionData(pCall),
        &ppwszRootKeyNames,
        &dwNumRootKeys
        );

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_ENUM_ROOTKEYS_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->ppwszRootKeyNames = ppwszRootKeyNames;
        ppwszRootKeyNames = NULL;
        pRegResp->dwNumRootKeys = dwNumRootKeys;

        pOut->tag = REG_R_ENUM_ROOT_KEYSW;
        pOut->data = pRegResp;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    if (ppwszRootKeyNames)
    {
        for (iCount=0; iCount<dwNumRootKeys; iCount++)
        {
        	LWREG_SAFE_FREE_MEMORY(ppwszRootKeyNames[iCount]);
        }
        ppwszRootKeyNames = NULL;
    }

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcCreateKeyEx(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    NTSTATUS status = 0;
    PREG_IPC_CREATE_KEY_EX_REQ pReq = pIn->data;
    PREG_IPC_CREATE_KEY_EX_RESPONSE pRegResp = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hkResult = NULL;
    DWORD dwDisposition = 0;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    if (status != STATUS_INVALID_HANDLE)
    {
        BAIL_ON_NT_STATUS(status);

        status = RegSrvCreateKeyEx(
            RegSrvIpcGetSessionData(pCall),
            hKey,
            pReq->pSubKey,
            0,
            pReq->pClass,
            pReq->dwOptions,
            pReq->AccessDesired,
            pReq->pSecDescRel,
            pReq->ulSecDescLen,
            &hkResult,
            &dwDisposition);
    }

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_CREATE_KEY_EX_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->dwDisposition = dwDisposition;

        status = RegSrvIpcRegisterHandle(
                                      pCall,
                                      "HKEY",
                                      hkResult,
                                      RegSrvIpcCloseHandle,
                                      &pRegResp->hkResult);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_CREATE_KEY_EX;
        pOut->data = pRegResp;
        hkResult = NULL;

        RegSrvIpcRetainHandle(pCall, pRegResp->hkResult);
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    RegSrvIpcCloseHandle((PVOID)hkResult);

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcOpenKeyExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_OPEN_KEY_EX_REQ pReq = pIn->data;
    PREG_IPC_OPEN_KEY_EX_RESPONSE pRegResp = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hkResult = NULL;
    HKEY hKey = NULL;

    if (pReq->hKey)
    {
        status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSrvOpenKeyExW(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pSubKey,
        0,
        pReq->AccessDesired,
        &hkResult
        );

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_OPEN_KEY_EX_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        status = RegSrvIpcRegisterHandle(
                                      pCall,
                                      "HKEY",
                                      hkResult,
                                      RegSrvIpcCloseHandle,
                                      &pRegResp->hkResult);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_OPEN_KEYW_EX;
        pOut->data = pRegResp;
        hkResult = NULL;

        RegSrvIpcRetainHandle(pCall, pRegResp->hkResult);
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    RegSrvIpcCloseHandle((PVOID)hkResult);

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcCloseKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_CLOSE_KEY_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;

    status = RegSrvIpcUnregisterHandle(pCall, pReq->hKey);
    if (!status)
    {
        pOut->tag = REG_R_CLOSE_KEY;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcDeleteKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_DELETE_KEY_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvDeleteKey(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pSubKey);

    if (!status)
    {
        pOut->tag = REG_R_DELETE_KEY;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcEnumKeyExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_ENUM_KEY_EX_REQ pReq = pIn->data;
    PREG_IPC_ENUM_KEY_EX_RESPONSE pRegResp = NULL;
    PWSTR pKeyName = NULL;
    PWSTR pClassName = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    if (pReq->cName)
    {
    	status = LW_RTL_ALLOCATE((PVOID*)&pKeyName, WCHAR, pReq->cName*sizeof(*pKeyName));
        BAIL_ON_NT_STATUS(status);
    }

    if (pReq->cClass)
    {
    	status = LW_RTL_ALLOCATE((PVOID*)&pClassName, WCHAR, pReq->cClass*sizeof(*pClassName));
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSrvEnumKeyExW(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->dwIndex,
        pKeyName,
        &pReq->cName,
        NULL,
        pClassName,
        &pReq->cClass,
        NULL);

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_ENUM_KEY_EX_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->pName= pKeyName;
        pKeyName = NULL;
        pRegResp->cName = pReq->cName;

        pRegResp->pClass= pClassName;
        pClassName = NULL;
        pRegResp->cClass = pReq->cClass;

        pOut->tag = REG_R_ENUM_KEYW_EX;
        pOut->data = pRegResp;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pKeyName);
    LWREG_SAFE_FREE_MEMORY(pClassName);

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcQueryInfoKeyW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_QUERY_INFO_KEY_REQ pReq = pIn->data;
    PREG_IPC_QUERY_INFO_KEY_RESPONSE pRegResp = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    DWORD dwSubKeyCount = 0;
    DWORD dwMaxKeyLength = 0;
    DWORD dwValueCount = 0;
    DWORD dwMaxValueNameLen = 0;
    DWORD dwMaxValueLen = 0;
    DWORD dwSecurityDescriptorLen = 0;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvQueryInfoKeyW(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        NULL,
        pReq->pcClass,
        NULL,
        &dwSubKeyCount,
        &dwMaxKeyLength,
        NULL,
        &dwValueCount,
        &dwMaxValueNameLen,
        &dwMaxValueLen,
        &dwSecurityDescriptorLen,
        NULL);
    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_QUERY_INFO_KEY_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->cSubKeys = dwSubKeyCount;
        pRegResp->cMaxSubKeyLen = dwMaxKeyLength;
        pRegResp->cValues = dwValueCount;
        pRegResp->cMaxValueNameLen = dwMaxValueNameLen;
        pRegResp->cMaxValueLen = dwMaxValueLen;
        pRegResp->cSecurityDescriptor = dwSecurityDescriptorLen;

        pOut->tag = REG_R_QUERY_INFO_KEYW;
        pOut->data = pRegResp;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcGetValueW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_GET_VALUE_REQ pReq = pIn->data;
    PREG_IPC_GET_VALUE_RESPONSE pRegResp = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    DWORD dwType = 0;
    PBYTE pData = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    if (pReq->cbData)
    {
    	status = LW_RTL_ALLOCATE((PVOID*)&pData, BYTE, pReq->cbData*sizeof(*pData));
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSrvGetValueW(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pSubKey,
        pReq->pValue,
        pReq->Flags,
        &dwType,
        pData,
        &pReq->cbData);

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_GET_VALUE_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->cbData = pReq->cbData;
        pRegResp->pvData = pData;
        pData = NULL;
        pRegResp->dwType = dwType;

        pOut->tag = REG_R_GET_VALUEW;
        pOut->data = pRegResp;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pData);

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcDeleteKeyValue(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_DELETE_KEY_VALUE_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvDeleteKeyValue(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pSubKey,
        pReq->pValueName);

    if (!status)
    {
        pOut->tag = REG_R_DELETE_KEY_VALUE;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcDeleteTree(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_DELETE_TREE_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvDeleteTree(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pSubKey);

    if (!status)
    {
        pOut->tag = REG_R_DELETE_TREE;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}


LWMsgStatus
RegSrvIpcDeleteValue(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_DELETE_VALUE_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvDeleteValue(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pValueName);

    if (!status)
    {
        pOut->tag = REG_R_DELETE_VALUE;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcEnumValueW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_ENUM_VALUE_REQ pReq = pIn->data;
    PREG_IPC_ENUM_VALUE_RESPONSE pRegResp = NULL;
    PWSTR pValueName = NULL;
    PBYTE pValue = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    REG_DATA_TYPE type = REG_NONE;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    if (pReq->cName)
    {
    	status = LW_RTL_ALLOCATE((PVOID*)&pValueName, WCHAR, pReq->cName*sizeof(*pValueName));
        BAIL_ON_NT_STATUS(status);
    }

    if (pReq->cValue)
    {
    	status = LW_RTL_ALLOCATE((PVOID*)&pValue, BYTE, pReq->cValue*sizeof(*pValue));
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSrvEnumValueW(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->dwIndex,
        pValueName,
        &pReq->cName,
        NULL,
        &type,
        pValue,
        &pReq->cValue);

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_ENUM_VALUE_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->pName= pValueName;
        pValueName = NULL;
        pRegResp->cName = pReq->cName;
        pRegResp->pValue = pValue;
        pValue = NULL;
        pRegResp->cValue = pReq->cValue;
        pRegResp->type = type;

        pOut->tag = REG_R_ENUM_VALUEW;
        pOut->data = pRegResp;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pValueName);
    LWREG_SAFE_FREE_MEMORY(pValue);

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcQueryMultipleValues(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_QUERY_MULTIPLE_VALUES_REQ pReq = pIn->data;
    PREG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE pRegResp = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvQueryMultipleValues(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->val_list,
        pReq->num_vals,
        pReq->pValue,
        &pReq->dwTotalsize);

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->dwTotalsize = pReq->dwTotalsize;
        pRegResp->num_vals = pReq->num_vals;
        pRegResp->val_list = pReq->val_list;
        pReq->val_list = NULL;
        pRegResp->pValue = pReq->pValue;
        pReq->pValue = NULL;

        pOut->tag = REG_R_QUERY_MULTIPLE_VALUES;
        pOut->data = pRegResp;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcSetValueExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_SET_VALUE_EX_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvSetValueExW(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pValueName,
        0,
        pReq->dwType,
        pReq->pData,
        pReq->cbData);

    if (!status)
    {
        pOut->tag = REG_R_SET_VALUEW_EX;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcSetKeySecurity(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_SET_KEY_SECURITY_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvSetKeySecurity(RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->SecurityInformation,
        pReq->SecurityDescriptor,
        pReq->Length);

    if (!status)
    {
        pOut->tag = REG_R_SET_KEY_SECURITY;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcGetKeySecurity(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
	NTSTATUS status = 0;
    PREG_IPC_GET_KEY_SECURITY_REQ pReq = pIn->data;
    PREG_IPC_GET_KEY_SECURITY_RES pRegResp = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    if (pReq->bRetSecurityDescriptor && pReq->Length)
    {
    	status = LW_RTL_ALLOCATE((PVOID*)&pSecDescRel, VOID, pReq->Length);
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSrvGetKeySecurity(RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->SecurityInformation,
        pSecDescRel,
        &pReq->Length);

	if (!status)
	{
		status = LW_RTL_ALLOCATE((PVOID*)&pRegResp, REG_IPC_GET_KEY_SECURITY_RES, sizeof(*pRegResp));
		BAIL_ON_NT_STATUS(status);

		pRegResp->SecurityDescriptor = pSecDescRel;
		pSecDescRel = NULL;
		pRegResp->Length = pReq->Length;

		pOut->tag = REG_R_GET_KEY_SECURITY;
		pOut->data = pRegResp;
	}
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pSecDescRel);
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcSetValueAttibutesW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    NTSTATUS status = 0;
    PREG_IPC_SET_VALUE_ATTRS_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);


    status = RegSrvSetValueAttributesW(
        RegSrvIpcGetSessionData(pCall),
        hKey,
        pReq->pSubKey,
        pReq->pValueName,
        pReq->pValueAttributes);

    if (!status)
    {
        pOut->tag = REG_R_SET_VALUEW_ATTRIBUTES;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcGetValueAttibutesW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    NTSTATUS status = 0;
    PREG_IPC_GET_VALUE_ATTRS_REQ pReq = pIn->data;
    PREG_IPC_GET_VALUE_ATTRS_RESPONSE pRegResp = NULL;
    PREG_IPC_STATUS pStatus = NULL;
    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);


    if (!pReq->bRetCurrentValue && !pReq->bRetValueAttributes)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSrvGetValueAttributesW(
            RegSrvIpcGetSessionData(pCall),
            hKey,
            pReq->pSubKey,
            pReq->pValueName,
            pReq->bRetCurrentValue ? &pCurrentValue :NULL,
            pReq->bRetValueAttributes ? &pValueAttributes : NULL);

    if (!status)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegResp,
                                 REG_IPC_GET_VALUE_ATTRS_RESPONSE,
                                 sizeof(*pRegResp));
        BAIL_ON_NT_STATUS(status);

        pRegResp->pCurrentValue = pCurrentValue;
        pCurrentValue = NULL;
        pRegResp->pValueAttributes = pValueAttributes;
        pValueAttributes = NULL;

        pOut->tag = REG_R_GET_VALUEW_ATTRIBUTES;
        pOut->data = pRegResp;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:
    RegSafeFreeCurrentValueInfo(&pCurrentValue);
    RegSafeFreeValueAttributes(&pValueAttributes);

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcDeleteValueAttibutesW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    NTSTATUS status = 0;
    PREG_IPC_DELETE_VALUE_ATTRS_REQ pReq = pIn->data;
    PREG_IPC_STATUS pStatus = NULL;
    HKEY hKey = NULL;

    status = RegSrvIpcGetHandleData(pCall, pReq->hKey, &hKey);
    BAIL_ON_NT_STATUS(status);

    status = RegSrvDeleteValueAttributesW(
            RegSrvIpcGetSessionData(pCall),
            hKey,
            pReq->pSubKey,
            pReq->pValueName);

    if (!status)
    {
        pOut->tag = REG_R_DELETE_VALUEW_ATTRIBUTES;
        pOut->data = NULL;
    }
    else
    {
        status = RegSrvIpcCreateError(status, &pStatus);
        BAIL_ON_NT_STATUS(status);

        pOut->tag = REG_R_ERROR;
        pOut->data = pStatus;
    }

cleanup:

    return MAP_REG_ERROR_IPC(status);

error:
    goto cleanup;
}

