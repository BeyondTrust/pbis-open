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
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#include "client.h"

DWORD
LsaOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    static LWMsgTime connectTimeout = {10, 0};

    BAIL_ON_INVALID_POINTER(phConnection);

    dwError = LwAllocateMemory(sizeof(LSA_CLIENT_CONNECTION_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pContext->pProtocol));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(pContext->pProtocol, LsaIPCGetProtocolSpec()));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_new(NULL, pContext->pProtocol, &pContext->pAssoc));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  CACHEDIR "/" LSA_SERVER_FILENAME));
    BAIL_ON_LSA_ERROR(dwError);

    if (getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
    {
        /* Give up connecting within 2 seconds in case lsassd
           is unresponsive (e.g. it's being traced in a debugger) */
        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_set_timeout(
                                      pContext->pAssoc,
                                      LWMSG_TIMEOUT_ESTABLISH,
                                      &connectTimeout));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_connect(pContext->pAssoc, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session(pContext->pAssoc, &pContext->pSession));
    BAIL_ON_LSA_ERROR(dwError);

    *phConnection = (HANDLE)pContext;

cleanup:
    return dwError;

error:
    if (pContext)
    {
        if (pContext->pAssoc)
        {
            lwmsg_assoc_delete(pContext->pAssoc);
        }

        if (pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }

        LwFreeMemory(pContext);
    }

    if (phConnection)
    {
        *phConnection = (HANDLE)NULL;
    }

    goto cleanup;
}

DWORD
LsaCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (pContext->pAssoc)
    {
        (void) lwmsg_assoc_close(pContext->pAssoc);
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LwFreeMemory(pContext);

    return dwError;
}

DWORD
LsaDropServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (pContext->pAssoc)
    {
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LwFreeMemory(pContext);

    return dwError;
}

DWORD
LsaIpcAcquireCall(
    HANDLE hServer,
    LWMsgCall** ppCall
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = hServer;

    dwError = MAP_LWMSG_ERROR(lwmsg_session_acquire_call(pContext->pSession, ppCall));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

VOID
LsaIpcReleaseHandle(
    HANDLE hServer,
    LWMsgHandle* pHandle
    )
{
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = hServer;

    lwmsg_session_release_handle(pContext->pSession, pHandle);
}

DWORD
LsaTransactAddGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LSA2_IPC_ADD_GROUP_REQ req = {0};
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.pGroupAddInfo = pGroupAddInfo;

    in.tag = LSA2_Q_ADD_GROUP;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA2_R_ADD_GROUP:
            break;
        case LSA2_R_ERROR:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactDeleteObject(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LSA2_IPC_DELETE_OBJECT_REQ req = {0};
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.pszSid = pszSid;

    in.tag = LSA2_Q_DELETE_OBJECT;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA2_R_DELETE_OBJECT:
            break;
        case LSA2_R_ERROR:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactAddUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_ADD_INFO pUserAddInfo
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LSA2_IPC_ADD_USER_REQ req = {0};
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.pUserAddInfo = pUserAddInfo;

    in.tag = LSA2_Q_ADD_USER;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_ADD_USER:
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:
    
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactAuthenticateUserPam(
    HANDLE hServer,
    PLSA_AUTH_USER_PAM_PARAMS pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    in.tag = LSA_Q_AUTH_USER_PAM;
    in.data = pParams;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_R_AUTH_USER_PAM_SUCCESS:
            if (ppPamAuthInfo)
            {
                *ppPamAuthInfo = out.data;
                out.data = NULL;
            }
            break;
        case LSA_R_AUTH_USER_PAM_FAILURE:
            pError = (PLSA_IPC_ERROR) out.data;
            if (pParams->dwFlags & LSA_AUTH_USER_PAM_FLAG_RETURN_MESSAGE && 
                    ppPamAuthInfo)
            {
                dwError = LwAllocateMemory(sizeof(*ppPamAuthInfo),
                                            (PVOID)ppPamAuthInfo);
                BAIL_ON_LSA_ERROR(dwError);

                (*ppPamAuthInfo)->pszMessage = pError->pszErrorMessage;
                pError->pszErrorMessage = NULL;
            }
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactAuthenticateUserEx(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    IN LSA_AUTH_USER_PARAMS* pParams,
    OUT PLSA_AUTH_USER_INFO* ppUserInfo
    )
{
    DWORD dwError = 0;
    LSA_IPC_AUTH_USER_EX_REQ req = {0};
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.authUserParams.AuthType = pParams->AuthType;
    req.authUserParams.pass = pParams->pass;
    req.authUserParams.pszAccountName = pParams->pszAccountName;
    req.authUserParams.pszDomain = pParams->pszDomain;
    req.authUserParams.pszWorkstation = pParams->pszWorkstation;

    in.tag = LSA_Q_AUTH_USER_EX;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_R_AUTH_USER_EX_SUCCESS:
            *ppUserInfo = (PLSA_AUTH_USER_INFO) out.data;
            out.data = NULL;
            break;
        case LSA_R_AUTH_USER_EX_FAILURE:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    LSA_IPC_AUTH_USER_REQ validateUserReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    validateUserReq.pszLoginName = pszLoginName;
    validateUserReq.pszPassword = pszPassword;

    in.tag = LSA_Q_VALIDATE_USER;
    in.data = &validateUserReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_R_VALIDATE_USER_SUCCESS:
            break;
        case LSA_R_VALIDATE_USER_FAILURE:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszOldPassword,
    PCSTR  pszNewPassword
    )
{
    DWORD dwError = 0;
    LSA_IPC_CHANGE_PASSWORD_REQ changePasswordReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    changePasswordReq.pszLoginName = pszLoginName;
    changePasswordReq.pszOldPassword = pszOldPassword;
    changePasswordReq.pszNewPassword = pszNewPassword;

    in.tag = LSA_Q_CHANGE_PASSWORD;
    in.data = &changePasswordReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_R_CHANGE_PASSWORD_SUCCESS:
            break;
        case LSA_R_CHANGE_PASSWORD_FAILURE:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactSetPassword(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword
    )
{
    DWORD dwError = 0;
    LSA_IPC_SET_PASSWORD_REQ setPasswordReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    setPasswordReq.pszLoginName   = pszLoginName;
    setPasswordReq.pszNewPassword = pszNewPassword;

    in.tag    = LSA_Q_SET_PASSWORD;
    in.data = &setPasswordReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_R_SET_PASSWORD_SUCCESS:
            break;
        case LSA_R_SET_PASSWORD_FAILURE:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactModifyUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LSA2_IPC_MODIFY_USER_REQ req = {0};
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.pUserModInfo = pUserModInfo;

    in.tag = LSA2_Q_MODIFY_USER;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA2_R_MODIFY_USER:
            break;
        case LSA2_R_ERROR:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactModifyGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LSA2_IPC_MODIFY_GROUP_REQ req = {0};
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.pGroupModInfo = pGroupModInfo;

    in.tag = LSA2_Q_MODIFY_GROUP;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag) {
    case LSA2_R_MODIFY_GROUP:
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR)out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactProviderIoControl(
    IN HANDLE  hServer,
    IN PCSTR   pszProvider,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    LSA_IPC_PROVIDER_IO_CONTROL_REQ providerIoControlReq;
    // Do not free pResultBuffer and pError
    PLSA_DATA_BLOB pResultBuffer = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    providerIoControlReq.pszProvider = pszProvider;
    providerIoControlReq.dwIoControlCode = dwIoControlCode;
    providerIoControlReq.dwDataLen = dwInputBufferSize;
    providerIoControlReq.pData = pInputBuffer;

    in.tag = LSA_Q_PROVIDER_IO_CONTROL;
    in.data = &providerIoControlReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_R_PROVIDER_IO_CONTROL_SUCCESS:
            *pdwOutputBufferSize = 0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_R_PROVIDER_IO_CONTROL_SUCCESS_DATA:
            pResultBuffer = (PLSA_DATA_BLOB)out.data;
            *pdwOutputBufferSize = pResultBuffer->dwLen;
            *ppOutputBuffer = (PVOID)(pResultBuffer->pData);
            pResultBuffer->pData = NULL;
            break;
        case LSA_R_PROVIDER_IO_CONTROL_FAILURE:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    goto cleanup;
}

DWORD
LsaTransactFindObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LSA2_IPC_FIND_OBJECTS_REQ req = {0};
    PLSA2_IPC_FIND_OBJECTS_RES pRes = NULL;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);
    
    req.pszTargetProvider = pszTargetProvider;
    req.FindFlags = FindFlags;
    req.ObjectType = ObjectType;
    req.QueryType = QueryType;
    
    switch (QueryType)
    {
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        req.IpcQueryType = LSA2_IPC_QUERY_DWORDS;
        break;
    case LSA_QUERY_TYPE_BY_DN:
    case LSA_QUERY_TYPE_BY_SID:
    case LSA_QUERY_TYPE_BY_NT4:
    case LSA_QUERY_TYPE_BY_ALIAS:
    case LSA_QUERY_TYPE_BY_UPN:
    case LSA_QUERY_TYPE_BY_NAME:
        req.IpcQueryType = LSA2_IPC_QUERY_STRINGS;
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    req.dwCount = dwCount;
    req.QueryList = QueryList;

    in.tag = LSA2_Q_FIND_OBJECTS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_FIND_OBJECTS:
        pRes = out.data;
        if (pRes->dwCount != dwCount)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        *pppObjects = pRes->ppObjects;
        pRes->ppObjects = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *pppObjects = NULL;

    goto cleanup;
}


DWORD
LsaTransactOpenEnumObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    LSA2_IPC_OPEN_ENUM_OBJECTS_REQ req = {0};
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);
    
    req.pszTargetProvider = pszTargetProvider;
    req.FindFlags = FindFlags;
    req.ObjectType = ObjectType;
    req.pszDomainName = pszDomainName;

    in.tag = LSA2_Q_OPEN_ENUM_OBJECTS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_OPEN_ENUM_OBJECTS:
        *phEnum = out.data;
        out.data = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *phEnum = NULL;

    goto cleanup;
}

DWORD
LsaTransactEnumObjects(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LSA2_IPC_ENUM_OBJECTS_REQ req = {0};
    PLSA2_IPC_ENUM_OBJECTS_RES pRes = NULL;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.hEnum = hEnum;   
    req.dwMaxObjectsCount = dwMaxObjectsCount;

    in.tag = LSA2_Q_ENUM_OBJECTS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_ENUM_OBJECTS:
        pRes = out.data;
        if (pRes->dwObjectsCount > dwMaxObjectsCount)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        *pdwObjectsCount = pRes->dwObjectsCount;
        *pppObjects = pRes->ppObjects;
        pRes->ppObjects = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    goto cleanup;
}

DWORD
LsaTransactOpenEnumMembers(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    LSA2_IPC_OPEN_ENUM_MEMBERS_REQ req = {0};
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);
    
    req.pszTargetProvider = pszTargetProvider;
    req.FindFlags = FindFlags;
    req.pszSid = pszSid;

    in.tag = LSA2_Q_OPEN_ENUM_MEMBERS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_OPEN_ENUM_MEMBERS:
        *phEnum = out.data;
        out.data = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *phEnum = NULL;

    goto cleanup;
}

DWORD
LsaTransactEnumMembers(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxSidCount,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszMemberSids
    )
{
    DWORD dwError = 0;
    LSA2_IPC_ENUM_MEMBERS_REQ req = {0};
    PLSA2_IPC_ENUM_MEMBERS_RES pRes = NULL;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.hEnum = hEnum;   
    req.dwMaxSidCount = dwMaxSidCount;

    in.tag = LSA2_Q_ENUM_MEMBERS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_ENUM_MEMBERS:
        pRes = out.data;
        if (pRes->dwSidCount > dwMaxSidCount)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        *pdwSidCount = pRes->dwSidCount;
        *pppszMemberSids = pRes->ppszMemberSids;
        pRes->ppszMemberSids = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *pdwSidCount = 0;
    *pppszMemberSids = NULL;

    goto cleanup;
}

DWORD
LsaTransactQueryMemberOf(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    LSA2_IPC_QUERY_MEMBER_OF_REQ req = {0};
    PLSA2_IPC_QUERY_MEMBER_OF_RES pRes = NULL;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.FindFlags = FindFlags;
    req.dwSidCount = dwSidCount;
    req.ppszSids = ppszSids;

    in.tag = LSA2_Q_QUERY_MEMBER_OF;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_QUERY_MEMBER_OF:
        pRes = out.data;
        *pdwGroupSidCount = pRes->dwGroupSidCount;
        *pppszGroupSids = pRes->ppszGroupSids;
        pRes->ppszGroupSids = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *pdwGroupSidCount = 0;
    *pppszGroupSids = NULL;

    goto cleanup;
}

LW_DWORD
LsaTransactFindGroupAndExpandedMembers(
    LW_IN LW_HANDLE hLsa,
    LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_QUERY_TYPE QueryType,
    LW_IN LSA_QUERY_ITEM QueryItem,
    LW_OUT PLSA_SECURITY_OBJECT* ppGroupObject,
    LW_OUT LW_PDWORD pdwMemberObjectCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    )
{
    DWORD dwError = 0;
    LSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_REQ req = {0};
    PLSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_RES pRes = NULL;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    req.pszTargetProvider = pszTargetProvider;
    req.FindFlags = FindFlags;
    req.QueryType = QueryType;
    req.QueryItem = QueryItem;
    
    switch(QueryType)
    {
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        req.IpcQueryType = LSA2_IPC_QUERY_DWORDS;
        break;
    default:
        req.IpcQueryType = LSA2_IPC_QUERY_STRINGS;
        break;
    }

    in.tag = LSA2_Q_FIND_GROUP_AND_EXPANDED_MEMBERS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_FIND_GROUP_AND_EXPANDED_MEMBERS:
        pRes = out.data;
        *ppGroupObject = pRes->pGroup;
        *pdwMemberObjectCount = pRes->dwMemberObjectCount;
        *pppMemberObjects = pRes->ppMemberObjects;

        pRes->pGroup = NULL;
        pRes->ppMemberObjects = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

   *ppGroupObject = NULL;
   *pdwMemberObjectCount = 0;
   *pppMemberObjects = NULL;

    goto cleanup;
}

DWORD
LsaTransactCloseEnum(
    IN HANDLE hLsa,
    IN OUT HANDLE hEnum
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    in.tag = LSA2_Q_CLOSE_ENUM;
    in.data = hEnum;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_CLOSE_ENUM:
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    /* Release handle no matter what */
    LsaIpcReleaseHandle(hLsa, hEnum);

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}


DWORD
LsaTransactGetSmartCardUserObject(
    HANDLE hServer,
    PLSA_SECURITY_OBJECT* ppObject,
    PSTR* ppszSmartCardReader
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;
    LSA2_IPC_GET_SMART_CARD_USER_RES* getSmartCardUserResponse;

    dwError = LsaIpcAcquireCall(hServer, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    in.tag = LSA2_Q_GET_SMARTCARD_USER_OBJECT;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out,
                NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
    case LSA2_R_GET_SMARTCARD_USER_OBJECT:
        getSmartCardUserResponse = out.data;
        *ppObject = getSmartCardUserResponse->pObject;
        *ppszSmartCardReader = getSmartCardUserResponse->pszSmartCardReader;
        getSmartCardUserResponse->pObject = NULL;
        getSmartCardUserResponse->pszSmartCardReader = NULL;
        break;
    case LSA2_R_ERROR:
        pError = (PLSA_IPC_ERROR) out.data;
        BAIL_WITH_LSA_ERROR(pError->dwError);
        break;
    default:
        BAIL_WITH_LSA_ERROR(LW_ERROR_INTERNAL);
    }
    
cleanup:
    
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaTransactGetStatus(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PLSASTATUS* ppLsaStatus
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;
    PLSASTATUS pLsaStatus = NULL;

    dwError = LsaIpcAcquireCall(hLsa, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    in.tag = LSA_Q_GET_STATUS;
    in.data = (PVOID)pszTargetProvider;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_R_GET_STATUS_SUCCESS:
            pLsaStatus = (PLSASTATUS)out.data;
            out.data = NULL;
            break;
        case LSA_R_GET_STATUS_FAILURE:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    *ppLsaStatus = pLsaStatus;

    return dwError;

error:

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
        pLsaStatus = NULL;
    }

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
