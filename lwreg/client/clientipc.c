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
 *        clientipc.c
 *
 * Abstract:
 *
 *        Registry Subsystem
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "client.h"

static REG_CLIENT_CONNECTION_CONTEXT gContext = {0};
static volatile LONG glLibraryRefCount = 0;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;

static
VOID
RegIpcReleaseHandle(
    IN HANDLE hConnection,
    IN PVOID pHandle
    )
{
    PREG_CLIENT_CONNECTION_CONTEXT pContext = hConnection;

    lwmsg_session_release_handle(pContext->pSession, pHandle);
}

DWORD
RegOpenServer(
	OUT PHANDLE phConnection
	)
{
    return RegNtStatusToWin32Error(
    		NtRegOpenServer(phConnection));
}

NTSTATUS
NtRegOpenServerInitialize(
    VOID
    )
{
    NTSTATUS status = 0;
    
    pthread_mutex_lock(&gLock);

    if (!gContext.pProtocol)
    {
        status = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &gContext.pProtocol));
        BAIL_ON_NT_STATUS(status);
        
        status = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(gContext.pProtocol, RegIPCGetProtocolSpec()));
        BAIL_ON_NT_STATUS(status);
    }

    if (!gContext.pClient)
    {
        status = MAP_LWMSG_ERROR(lwmsg_peer_new(NULL, gContext.pProtocol, &gContext.pClient));
        BAIL_ON_NT_STATUS(status);
        
        status = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
                                     gContext.pClient,
                                     LWMSG_ENDPOINT_DIRECT,
                                     "lwreg"));

        status = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
                                     gContext.pClient,
                                     LWMSG_ENDPOINT_LOCAL,
                                     CACHEDIR "/" REG_SERVER_FILENAME));
        BAIL_ON_NT_STATUS(status);
    }

    if (!gContext.pSession)
    {
        status = MAP_LWMSG_ERROR(lwmsg_peer_connect(gContext.pClient, &gContext.pSession));
        BAIL_ON_NT_STATUS(status);  
    }
        
cleanup:

    pthread_mutex_unlock(&gLock);

    return status;

error:

    goto cleanup;
}


NTSTATUS
NtRegOpenServer(
    OUT PHANDLE phConnection
    )
{
    NTSTATUS status = 0;

    BAIL_ON_NT_INVALID_POINTER(phConnection);

    status = NtRegOpenServerInitialize();
    BAIL_ON_NT_STATUS(status);

    *phConnection = (HANDLE) &gContext;

cleanup:

    return status;

error:

    if (phConnection)
    {
        *phConnection = NULL;
    }

    goto cleanup;
}

VOID
RegCloseServer(
    IN HANDLE hConnection
    )
{
	NtRegCloseServer(hConnection);
}


VOID
NtRegCloseServer(
    HANDLE hConnection
    )
{
    return;
}

static
__attribute__((constructor))
VOID
NtRegOpenServerOnce(
    VOID
    )
{
    LwInterlockedIncrement(&glLibraryRefCount);
}

static
__attribute__((destructor))
VOID
NtRegCloseServerOnce(
    VOID
    )
{
    if (!LwInterlockedDecrement(&glLibraryRefCount))
    {
        if (gContext.pClient)
        {
            lwmsg_peer_delete(gContext.pClient);
        }

        if (gContext.pProtocol)
        {
            lwmsg_protocol_delete(gContext.pProtocol);
        }

        memset(&gContext, 0, sizeof(gContext));
    }
}

NTSTATUS
RegIpcAcquireCall(
    HANDLE hConnection,
    LWMsgCall** ppCall
    )
{
    NTSTATUS status = 0;
    PREG_CLIENT_CONNECTION_CONTEXT pContext = hConnection;

    status = MAP_LWMSG_ERROR(lwmsg_peer_acquire_call(pContext->pClient, ppCall));
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

NTSTATUS
RegTransactEnumRootKeysW(
    IN HANDLE hConnection,
    OUT PWSTR** pppwszRootKeyNames,
    OUT PDWORD pdwNumRootKey
    )
{
	NTSTATUS status = 0;
	// Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;
    PREG_IPC_ENUM_ROOTKEYS_RESPONSE pEnumRootKeysResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    in.tag = REG_Q_ENUM_ROOT_KEYSW;
    in.data = NULL;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_ENUM_ROOT_KEYSW:
            pEnumRootKeysResp = (PREG_IPC_ENUM_ROOTKEYS_RESPONSE)out.data;
            *pppwszRootKeyNames = pEnumRootKeysResp->ppwszRootKeyNames;
            pEnumRootKeysResp->ppwszRootKeyNames = NULL;
            *pdwNumRootKey = pEnumRootKeysResp->dwNumRootKeys;
            pEnumRootKeysResp->dwNumRootKeys = 0;

            break;
        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
        	status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

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
    )
{
	NTSTATUS status = 0;
    REG_IPC_CREATE_KEY_EX_REQ CreateKeyExReq = {0};
    PREG_IPC_CREATE_KEY_EX_RESPONSE pCreateKeyExResp = NULL;
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescLen = 0;


    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    if (pSecurityDescriptor)
    {
    	ulSecDescLen = 1024;

    	do
    	{
    	    status = NtRegReallocMemory(pSecDescRel,
    		   						    (PVOID*)&pSecDescRel,
    									ulSecDescLen);
    		BAIL_ON_NT_STATUS(status);

    	    memset(pSecDescRel, 0, ulSecDescLen);

    		status = RtlAbsoluteToSelfRelativeSD(pSecurityDescriptor,
    									  		 pSecDescRel,
    											 &ulSecDescLen);
    		if (STATUS_BUFFER_TOO_SMALL  == status)
    		{
    		    ulSecDescLen *= 2;
    		}
    		else
    		{
    			BAIL_ON_NT_STATUS(status);
    		}
    	}
    	while((status != STATUS_SUCCESS) &&
    		  (ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
    }

    CreateKeyExReq.hKey = (LWMsgHandle*) hKey;
    CreateKeyExReq.pSubKey = pSubKey;
    CreateKeyExReq.pClass = pClass;
    CreateKeyExReq.dwOptions = dwOptions;
    CreateKeyExReq.AccessDesired = AccessDesired;
    CreateKeyExReq.pSecDescRel = pSecDescRel;
    CreateKeyExReq.ulSecDescLen = ulSecDescLen;

    in.tag = REG_Q_CREATE_KEY_EX;
    in.data = &CreateKeyExReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_CREATE_KEY_EX:
            pCreateKeyExResp = (PREG_IPC_CREATE_KEY_EX_RESPONSE)out.data;
            *phkResult = (HKEY) pCreateKeyExResp->hkResult;
            pCreateKeyExResp->hkResult = NULL;

            if(pdwDisposition)
            {
                *pdwDisposition = pCreateKeyExResp->dwDisposition;
            }

            break;
        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;
        default:
        	status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    LWREG_SAFE_FREE_MEMORY(pSecDescRel);

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactOpenKeyExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
    NTSTATUS status = 0;
    REG_IPC_OPEN_KEY_EX_REQ OpenKeyExReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;
    PREG_IPC_OPEN_KEY_EX_RESPONSE pOpenKeyExResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    OpenKeyExReq.hKey = (LWMsgHandle*) hKey;
    OpenKeyExReq.pSubKey = pwszSubKey;
    OpenKeyExReq.AccessDesired = AccessDesired;

    in.tag = REG_Q_OPEN_KEYW_EX;
    in.data = &OpenKeyExReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_OPEN_KEYW_EX:
            pOpenKeyExResp = (PREG_IPC_OPEN_KEY_EX_RESPONSE) out.data;

            *phkResult = (HKEY) pOpenKeyExResp->hkResult;
            pOpenKeyExResp->hkResult = NULL;

            break;
        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;
        default:
        	status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactCloseKey(
    IN HANDLE hConnection,
    IN HKEY hKey
    )
{
    NTSTATUS status = 0;
    REG_IPC_CLOSE_KEY_REQ CloseKeyReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    CloseKeyReq.hKey = (LWMsgHandle*) hKey;

    in.tag = REG_Q_CLOSE_KEY;
    in.data = &CloseKeyReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_CLOSE_KEY:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:

    /* Release handle no matter what */
    RegIpcReleaseHandle(hConnection, hKey);

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactDeleteKeyW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
	NTSTATUS status = 0;
    REG_IPC_DELETE_KEY_REQ DeleteKeyReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    DeleteKeyReq.hKey = (LWMsgHandle*) hKey;
    DeleteKeyReq.pSubKey = pSubKey;

    in.tag = REG_Q_DELETE_KEY;
    in.data = &DeleteKeyReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_DELETE_KEY:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactQueryInfoKeyW(
    IN HANDLE hConnection,
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
    )
{
	NTSTATUS status = 0;
    REG_IPC_QUERY_INFO_KEY_REQ QueryInfoKeyReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;
    PREG_IPC_QUERY_INFO_KEY_RESPONSE pQueryInfoKeyResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    QueryInfoKeyReq.hKey = (LWMsgHandle*) hKey;
    QueryInfoKeyReq.pcClass = pcClass;

    in.tag = REG_Q_QUERY_INFO_KEYW;
    in.data = &QueryInfoKeyReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_QUERY_INFO_KEYW:
            pQueryInfoKeyResp = (PREG_IPC_QUERY_INFO_KEY_RESPONSE) out.data;

            if (pcSubKeys)
            {
                *pcSubKeys = pQueryInfoKeyResp->cSubKeys;
            }
            if (pcMaxSubKeyLen)
            {
                *pcMaxSubKeyLen = pQueryInfoKeyResp->cMaxSubKeyLen;
            }
            if (pcValues)
            {
                *pcValues = pQueryInfoKeyResp->cValues;
            }
            if (pcMaxValueNameLen)
            {
                *pcMaxValueNameLen = pQueryInfoKeyResp->cMaxValueNameLen;
            }
            if (pcMaxValueLen)
            {
                *pcMaxValueLen = pQueryInfoKeyResp->cMaxValueLen;
            }
            if (pcbSecurityDescriptor)
            {
            	*pcbSecurityDescriptor = pQueryInfoKeyResp->cSecurityDescriptor;
            }

            break;
        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactEnumKeyExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT OPTIONAL PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
	NTSTATUS status = 0;

    REG_IPC_ENUM_KEY_EX_REQ EnumKeyExReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;
    PREG_IPC_ENUM_KEY_EX_RESPONSE pEnumKeyExResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    EnumKeyExReq.hKey = (LWMsgHandle*) hKey;
    EnumKeyExReq.dwIndex = dwIndex;
    EnumKeyExReq.cName = *pcName;
    EnumKeyExReq.cClass = pcClass ? *pcClass : 0;

    in.tag = REG_Q_ENUM_KEYW_EX;
    in.data = &EnumKeyExReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_ENUM_KEYW_EX:
            pEnumKeyExResp = (PREG_IPC_ENUM_KEY_EX_RESPONSE) out.data;

            memcpy(pName, pEnumKeyExResp->pName, (pEnumKeyExResp->cName+1)*sizeof(*pName));
            *pcName = pEnumKeyExResp->cName;

            if (pClass)
            {
                memcpy(pClass, pEnumKeyExResp->pClass, (pEnumKeyExResp->cClass+1)*sizeof(*pClass));
                if (pcClass)
                {
                	*pcClass = pEnumKeyExResp->cClass;
                }
            }

            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

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
    )
{
	NTSTATUS status = 0;
    REG_IPC_GET_VALUE_REQ GetValueReq = {0};
    PREG_IPC_GET_VALUE_RESPONSE pGetValueResp = NULL;
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    GetValueReq.hKey = (LWMsgHandle*) hKey;
    GetValueReq.pSubKey = pSubKey;
    GetValueReq.pValue = pValue;
    GetValueReq.Flags = Flags;
    GetValueReq.cbData = pcbData ? *pcbData : 0;

    in.tag = REG_Q_GET_VALUEW;
    in.data = &GetValueReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_GET_VALUEW:
            pGetValueResp = (PREG_IPC_GET_VALUE_RESPONSE) out.data;

            if (pdwType)
            {
            	*pdwType = pGetValueResp->dwType;
            }

            if (pvData)
            {
                memcpy(pvData, pGetValueResp->pvData, pGetValueResp->cbData);
            }

            if (pcbData)
            {
                *pcbData = pGetValueResp->cbData;
            }

            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactDeleteKeyValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
	NTSTATUS status = 0;
    REG_IPC_DELETE_KEY_VALUE_REQ DeleteKeyValueReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    DeleteKeyValueReq.hKey = (LWMsgHandle*) hKey;
    DeleteKeyValueReq.pSubKey = pSubKey;
    DeleteKeyValueReq.pValueName = pValueName;

    in.tag = REG_Q_DELETE_KEY_VALUE;
    in.data = &DeleteKeyValueReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_DELETE_KEY_VALUE:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }
    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactDeleteTreeW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
	NTSTATUS status = 0;
    REG_IPC_DELETE_TREE_REQ DeleteTreeReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    DeleteTreeReq.hKey = (LWMsgHandle*) hKey;
    DeleteTreeReq.pSubKey = pSubKey;

    in.tag = REG_Q_DELETE_TREE;
    in.data = &DeleteTreeReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_DELETE_TREE:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactDeleteValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
	NTSTATUS status = 0;
    REG_IPC_DELETE_VALUE_REQ DeleteValueReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    DeleteValueReq.hKey = (LWMsgHandle*) hKey;
    DeleteValueReq.pValueName = pValueName;

    in.tag = REG_Q_DELETE_VALUE;
    in.data = &DeleteValueReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_DELETE_VALUE:
            break;
        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

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
    )
{
	NTSTATUS status = 0;
    REG_IPC_ENUM_VALUE_REQ EnumValueReq = {0};
    PREG_IPC_ENUM_VALUE_RESPONSE pEnumValueResp = NULL;
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    EnumValueReq.hKey = (LWMsgHandle*) hKey;
    EnumValueReq.dwIndex = dwIndex;
    EnumValueReq.cName = *pcchValueName;
    EnumValueReq.cValue = pcbData == NULL ? 0 : *pcbData;


    in.tag = REG_Q_ENUM_VALUEW;
    in.data = &EnumValueReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_ENUM_VALUEW:
            pEnumValueResp = (PREG_IPC_ENUM_VALUE_RESPONSE) out.data;

            memcpy(pValueName, pEnumValueResp->pName, (pEnumValueResp->cName+1)*sizeof(*pValueName));
            *pcchValueName = pEnumValueResp->cName;

            if (pData)
            {
                memcpy(pData, pEnumValueResp->pValue, pEnumValueResp->cValue*sizeof(*pData));
            }

            if (pcbData)
            {
                *pcbData = pEnumValueResp->cValue;
            }

            if (pType)
            {
                *pType = pEnumValueResp->type;
            }

            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactQueryMultipleValues(
    IN HANDLE hConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD pdwTotsize
    )
{
	NTSTATUS status = 0;
    REG_IPC_QUERY_MULTIPLE_VALUES_REQ QueryMultipleValuesReq = {0};
    PREG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE pRegResp = NULL;
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;
    int iCount = 0;
    int offSet = 0;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    QueryMultipleValuesReq.hKey = (LWMsgHandle*) hKey;
    QueryMultipleValuesReq.num_vals = num_vals;
    QueryMultipleValuesReq.val_list = val_list;
    if (pValueBuf)
    {
        QueryMultipleValuesReq.pValue = pValueBuf;
    }
    if (pdwTotsize)
    {
        QueryMultipleValuesReq.dwTotalsize = *pdwTotsize;
    }

    in.tag = REG_Q_QUERY_MULTIPLE_VALUES;
    in.data = &QueryMultipleValuesReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
       case REG_R_QUERY_MULTIPLE_VALUES:
            pRegResp = (PREG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE) out.data;

            if (pValueBuf)
            {
                memcpy(pValueBuf, pRegResp->pValue, pRegResp->dwTotalsize*sizeof(*pRegResp->pValue));
            }

            for (iCount = 0; iCount < pRegResp->num_vals; iCount++)
            {
                offSet = iCount==0 ? 0 : (offSet + val_list[iCount-1].ve_valuelen);

                val_list[iCount].ve_type = pRegResp->val_list[iCount].ve_type;
                val_list[iCount].ve_valuelen = pRegResp->val_list[iCount].ve_valuelen;

                if (pValueBuf)
                {
                    val_list[iCount].ve_valueptr = (PDWORD)(pValueBuf+offSet);
                }
            }
            if (pdwTotsize)
            {
                *pdwTotsize = pRegResp->dwTotalsize;
            }

            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactSetValueExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
	NTSTATUS status = 0;
    REG_IPC_SET_VALUE_EX_REQ SetValueExReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
	LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
	LWMsgCall* pCall = NULL;

	status = RegIpcAcquireCall(hConnection, &pCall);
	BAIL_ON_NT_STATUS(status);

    SetValueExReq.hKey = (LWMsgHandle*) hKey;
    SetValueExReq.pValueName = pValueName;
    SetValueExReq.dwType = dwType;
    SetValueExReq.pData = pData;
    SetValueExReq.cbData = cbData;

    in.tag = REG_Q_SET_VALUEW_EX;
    in.data = &SetValueExReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_SET_VALUEW_EX:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
	if (pCall)
	{
		lwmsg_call_destroy_params(pCall, &out);
		lwmsg_call_release(pCall);
	}

	return status;

error:
	goto cleanup;
}

NTSTATUS
RegTransactSetKeySecurity(
	IN HANDLE hConnection,
	IN HKEY hKey,
	IN SECURITY_INFORMATION SecurityInformation,
	IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
	IN ULONG Length
	)
{
	NTSTATUS status = 0;
    REG_IPC_SET_KEY_SECURITY_REQ SetKeySecurityReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    SetKeySecurityReq.hKey = (LWMsgHandle*) hKey;
    SetKeySecurityReq.SecurityInformation = SecurityInformation;
    SetKeySecurityReq.SecurityDescriptor = SecurityDescriptor;
    SetKeySecurityReq.Length = Length;

    in.tag = REG_Q_SET_KEY_SECURITY;
    in.data = &SetKeySecurityReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_SET_KEY_SECURITY:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
	if (pCall)
	{
		lwmsg_call_destroy_params(pCall, &out);
		lwmsg_call_release(pCall);
	}

	return status;

error:
	goto cleanup;
}

NTSTATUS
RegTransactGetKeySecurity(
	IN HANDLE hConnection,
	IN HKEY hKey,
	IN SECURITY_INFORMATION SecurityInformation,
	OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
	IN OUT PULONG lpcbSecurityDescriptor
	)
{
	NTSTATUS status = 0;
    REG_IPC_GET_KEY_SECURITY_REQ GetKeySecurityReq = {0};
    PREG_IPC_GET_KEY_SECURITY_RES pGetKeySecurityResp = NULL;
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    GetKeySecurityReq.hKey = (LWMsgHandle*) hKey;
    GetKeySecurityReq.SecurityInformation = SecurityInformation;
    GetKeySecurityReq.Length = *lpcbSecurityDescriptor;
    GetKeySecurityReq.bRetSecurityDescriptor = SecurityDescriptor ? TRUE : FALSE;

    in.tag = REG_Q_GET_KEY_SECURITY;
    in.data = &GetKeySecurityReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_GET_KEY_SECURITY:
        	pGetKeySecurityResp = (PREG_IPC_GET_KEY_SECURITY_RES) out.data;

        	*lpcbSecurityDescriptor = pGetKeySecurityResp->Length;

        	if (SecurityDescriptor)
        	{
        	    memcpy(SecurityDescriptor,
        	           pGetKeySecurityResp->SecurityDescriptor,
        	           pGetKeySecurityResp->Length);
        	}

            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
	if (pCall)
    {
		lwmsg_call_destroy_params(pCall, &out);
		lwmsg_call_release(pCall);
    }

	return status;

error:
    goto cleanup;
}


NTSTATUS
RegTransactSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    NTSTATUS status = 0;
    REG_IPC_SET_VALUE_ATTRS_REQ SetValueAttrsReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hRegConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    SetValueAttrsReq.hKey = (LWMsgHandle*) hKey;
    SetValueAttrsReq.pSubKey = pSubKey;
    SetValueAttrsReq.pValueName = pValueName;
    SetValueAttrsReq.pValueAttributes = pValueAttributes;

    in.tag = REG_Q_SET_VALUEW_ATTRIBUTES;
    in.data = &SetValueAttrsReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_SET_VALUEW_ATTRIBUTES:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}


NTSTATUS
RegTransactGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    NTSTATUS status = 0;
    REG_IPC_GET_VALUE_ATTRS_REQ GetValueAttrsReq = {0};
    PREG_IPC_GET_VALUE_ATTRS_RESPONSE pGetValueAttrsResp = NULL;
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    if (!ppCurrentValue && !ppValueAttributes)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = RegIpcAcquireCall(hRegConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    GetValueAttrsReq.hKey = (LWMsgHandle*) hKey;
    GetValueAttrsReq.pSubKey = pwszSubKey;
    GetValueAttrsReq.pValueName = pwszValueName;
    GetValueAttrsReq.bRetCurrentValue = ppCurrentValue ? TRUE : FALSE;
    GetValueAttrsReq.bRetValueAttributes = ppValueAttributes ? TRUE : FALSE;

    in.tag = REG_Q_GET_VALUEW_ATTRIBUTES;
    in.data = &GetValueAttrsReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_GET_VALUEW_ATTRIBUTES:
            pGetValueAttrsResp = (PREG_IPC_GET_VALUE_ATTRS_RESPONSE) out.data;

            if (ppCurrentValue)
            {
                *ppCurrentValue = pGetValueAttrsResp->pCurrentValue;
                pGetValueAttrsResp->pCurrentValue = NULL;
            }

            if (ppValueAttributes)
            {
                *ppValueAttributes = pGetValueAttrsResp->pValueAttributes;
                pGetValueAttrsResp->pValueAttributes = NULL;
            }

            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegTransactDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    )
{
    NTSTATUS status = 0;
    REG_IPC_DELETE_VALUE_ATTRS_REQ DeleteValueAttrsReq = {0};
    // Do not free pStatus
    PREG_IPC_STATUS pStatus = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    status = RegIpcAcquireCall(hRegConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    DeleteValueAttrsReq.hKey = (LWMsgHandle*) hKey;
    DeleteValueAttrsReq.pSubKey = pwszSubKey;
    DeleteValueAttrsReq.pValueName = pwszValueName;

    in.tag = REG_Q_DELETE_VALUEW_ATTRIBUTES;
    in.data = &DeleteValueAttrsReq;

    status = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case REG_R_DELETE_VALUEW_ATTRIBUTES:
            break;

        case REG_R_ERROR:
            pStatus = (PREG_IPC_STATUS) out.data;
            status = pStatus->status;
            BAIL_ON_NT_STATUS(status);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:
    goto cleanup;
}
/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
