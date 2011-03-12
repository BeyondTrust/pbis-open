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
 *        ipc_dispatch.c
 *
 * Abstract:
 *
 *        Likewise Security and Authorization Subsystem (LSASS)
 *
 *        Server IPC dispatch table
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "api.h"

static
VOID
LsaFreeSecurityObjectPrivateAttrs(
    PLSA_SECURITY_OBJECT pObject
    )
{
    if (pObject)
    {
        if (pObject->type == LSA_OBJECT_TYPE_USER)
        {
            LW_SECURE_FREE_MEMORY(pObject->userInfo.pLmHash, pObject->userInfo.dwLmHashLen);
            LW_SECURE_FREE_MEMORY(pObject->userInfo.pNtHash, pObject->userInfo.dwNtHashLen);
        }
    }
}

static
VOID
LsaFreeSecurityObjectListPrivateAttrs(
    DWORD dwCount,
    PLSA_SECURITY_OBJECT* ppObjectList
    )
{
    DWORD dwIndex = 0;

    if (ppObjectList)
    {
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            LsaFreeSecurityObjectPrivateAttrs(ppObjectList[dwIndex]);
        }
    }
}

static
DWORD
LsaSrvIpcRegisterHandle(
    LWMsgCall* pCall,
    PCSTR pszHandleType,
    PVOID pData,
    LWMsgHandleCleanupFunction pfnCleanup,
    LWMsgHandle** ppHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(
        lwmsg_session_register_handle(
            pSession,
            pszHandleType,
            pData,
            pfnCleanup,
            ppHandle));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
VOID
LsaSrvIpcRetainHandle(
    LWMsgCall* pCall,
    LWMsgHandle* pHandle
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    lwmsg_session_retain_handle(pSession, pHandle);
}

static
DWORD
LsaSrvIpcUnregisterHandle(
    LWMsgCall* pCall,
    LWMsgHandle* pHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_unregister_handle(pSession, pHandle));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
LsaSrvIpcGetHandle(
    LWMsgCall* pCall,
    LWMsgHandle* pHandle,
    PHANDLE phHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(
        lwmsg_session_get_handle_data(pSession, pHandle, OUT_PPVOID(phHandle)));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
HANDLE
LsaSrvIpcGetSessionData(
    LWMsgCall* pCall
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    return lwmsg_session_get_data(pSession);
}

static void
LsaSrvCleanupArtefactEnumHandle(
    void* pData
    )
{
    LsaSrvEndEnumNSSArtefacts(
        NULL,
        (HANDLE) pData);
}

static LWMsgStatus
LsaSrvIpcFindNSSArtefactByKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pUserInfo
    PVOID pNSSArtefactInfo = NULL;
    PVOID* ppNSSArtefactInfo = NULL;
    PLSA_NSS_ARTEFACT_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_NSSARTEFACT_BY_KEY_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvFindNSSArtefactByKey(
                       LsaSrvIpcGetSessionData(pCall),
                       pReq->pszKeyName,
                       pReq->pszMapName,
                       pReq->dwFlags,
                       pReq->dwInfoLevel,
                       &pNSSArtefactInfo);

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwNssArtefactInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumNssArtefacts = 1;

        dwError = LwAllocateMemory(
                        sizeof(*ppNSSArtefactInfo) * 1,
                        (PVOID*)&ppNSSArtefactInfo);
        BAIL_ON_LSA_ERROR(dwError);

        ppNSSArtefactInfo[0] = pNSSArtefactInfo;
        pNSSArtefactInfo = NULL;

        switch (pResult->dwNssArtefactInfoLevel)
        {
            case 0:
                pResult->ppNssArtefactInfoList.ppInfoList0 = (PLSA_NSS_ARTEFACT_INFO_0*)ppNSSArtefactInfo;
                ppNSSArtefactInfo = NULL;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_FIND_NSS_ARTEFACT_BY_KEY_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_FIND_NSS_ARTEFACT_BY_KEY_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if (pNSSArtefactInfo)
    {
        LsaFreeNSSArtefactInfo(pReq->dwInfoLevel, pNSSArtefactInfo);
    }
    if (ppNSSArtefactInfo)
    {
        LsaFreeNSSArtefactInfoList(pReq->dwInfoLevel, ppNSSArtefactInfo, 1);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNssArtefactInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcBeginEnumNSSArtefacts(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PSTR pszGUID = NULL;
    PLSA_IPC_BEGIN_ENUM_NSSARTEFACT_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hResume = NULL;
    LWMsgHandle* pHandle = NULL;

    dwError = LsaSrvBeginEnumNSSArtefacts(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszMapName,
                        pReq->dwFlags,
                        pReq->dwInfoLevel,
                        pReq->dwMaxNumNSSArtefacts,
                        &hResume);

    if (!dwError)
    {
        dwError = LsaSrvIpcRegisterHandle(
                                      pCall,
                                      "EnumArtefacts",
                                      hResume,
                                      LsaSrvCleanupArtefactEnumHandle,
                                      &pHandle);

        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_SUCCESS;
        pOut->data = pHandle;
        hResume = NULL;

        LsaSrvIpcRetainHandle(pCall, pHandle);
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszGUID);

    return MAP_LW_ERROR_IPC(dwError);

error:

    if (hResume)
    {
        LsaSrvCleanupArtefactEnumHandle(hResume);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumNSSArtefacts(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNSSArtefactInfoLevel = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    PLSA_NSS_ARTEFACT_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hEnum = NULL;

    dwError = LsaSrvIpcGetHandle(
        pCall,
        (LWMsgHandle*) pIn->data,
        &hEnum);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumNSSArtefacts(
                       LsaSrvIpcGetSessionData(pCall),
                       hEnum,
                       &dwNSSArtefactInfoLevel,
                       &ppNSSArtefactInfoList,
                       &dwNumNSSArtefactsFound);

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pResult),
                                   (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwNssArtefactInfoLevel = dwNSSArtefactInfoLevel;
        pResult->dwNumNssArtefacts = dwNumNSSArtefactsFound;
        switch (pResult->dwNssArtefactInfoLevel)
        {
            case 0:
                pResult->ppNssArtefactInfoList.ppInfoList0 = (PLSA_NSS_ARTEFACT_INFO_0*)ppNSSArtefactInfoList;
                ppNSSArtefactInfoList = NULL;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_ENUM_NSS_ARTEFACTS_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ENUM_NSS_ARTEFACTS_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    if(ppNSSArtefactInfoList)
    {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNssArtefactInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEndEnumNSSArtefacts(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvIpcUnregisterHandle(pCall, (LWMsgHandle*) pIn->data);
    if (!dwError)
    {
        pOut->tag = LSA_R_END_ENUM_NSS_ARTEFACTS_SUCCESS;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_END_ENUM_NSS_ARTEFACTS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}


static LWMsgStatus
LsaSrvIpcAuthenticateUserPam(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_USER_PAM_PARAMS pParams = pIn->data;
    PLSA_AUTH_USER_PAM_INFO pInfo = NULL;
    PLSA_IPC_ERROR pError = NULL;

    // The following call with *always* allocate pInfo

    dwError = LsaSrvAuthenticateUserPam(
                        LsaSrvIpcGetSessionData(pCall),
                        pParams,
                        &pInfo);

    if (!dwError)
    {
        pOut->tag = LSA_R_AUTH_USER_PAM_SUCCESS;
        pOut->data = pInfo;
        pInfo = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(
                        dwError,
                        pInfo ? pInfo->pszMessage : NULL,
                        &pError);
        BAIL_ON_LSA_ERROR(dwError);
        if (pInfo)
        {
            pInfo->pszMessage = NULL;
        }

        pOut->tag = LSA_R_AUTH_USER_PAM_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    if (pInfo)
    {
        LsaFreeAuthUserPamInfo(pInfo);
    }
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcValidateUser(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_AUTH_USER_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvValidateUser(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszPassword);

    if (!dwError)
    {
        pOut->tag = LSA_R_VALIDATE_USER_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_VALIDATE_USER_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcCheckUserInList(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHECK_USER_IN_LIST_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvCheckUserInList(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszListName);

    if (!dwError)
    {
        pOut->tag = LSA_R_CHECK_USER_IN_LIST_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_CHECK_USER_IN_LIST_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcChangePassword(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHANGE_PASSWORD_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvChangePassword(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszOldPassword,
                        pReq->pszNewPassword);

    if (!dwError)
    {
        pOut->tag = LSA_R_CHANGE_PASSWORD_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_CHANGE_PASSWORD_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcSetPassword(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_SET_PASSWORD_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvSetPassword(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszNewPassword);
    if (!dwError)
    {
        pOut->tag    = LSA_R_SET_PASSWORD_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag    = LSA_R_SET_PASSWORD_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}


static VOID
FreeAuthUserInfo(PLSA_AUTH_USER_INFO *pUserInfo)
{
    if (!pUserInfo)
        return;

    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszAccount);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszUserPrincipalName);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszFullName);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszDomain);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszDnsDomain);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszLogonServer);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszLogonScript);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszProfilePath);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszHomeDirectory);
    LW_SAFE_FREE_MEMORY((*pUserInfo)->pszHomeDrive);

    LsaDataBlobFree(&(*pUserInfo)->pSessionKey);
    LsaDataBlobFree(&(*pUserInfo)->pLmSessionKey);

    LW_SAFE_FREE_MEMORY((*pUserInfo)->pSidAttribList);	

    LW_SAFE_FREE_MEMORY(*pUserInfo);

    return;
}


static LWMsgStatus
LsaSrvIpcAuthenticateUserEx(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = LW_ERROR_NOT_IMPLEMENTED;
    PLSA_IPC_AUTH_USER_EX_REQ pReq = (PLSA_IPC_AUTH_USER_EX_REQ) pIn->data;
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvAuthenticateUserEx(LsaSrvIpcGetSessionData(pCall),
                                       pReq->pszTargetProvider,
                                       &pReq->authUserParams,
                                       &pUserInfo);

    if (!dwError)
    {
        pOut->tag = LSA_R_AUTH_USER_EX_SUCCESS;
        pOut->data = pUserInfo;
        pUserInfo = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_AUTH_USER_EX_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if (pUserInfo)
    {
       FreeAuthUserInfo(&pUserInfo);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
   goto cleanup;
}

static LWMsgStatus
LsaSrvIpcAddGroup2(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PLSA2_IPC_ADD_GROUP_REQ pReq = pIn->data;

    dwError = LsaSrvAddGroup2(
        LsaSrvIpcGetSessionData(pCall),
        pReq->pszTargetProvider,
        pReq->pGroupAddInfo);

    if (!dwError)
    {
        pOut->tag = LSA2_R_ADD_GROUP;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcModifyGroup2(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PLSA2_IPC_MODIFY_GROUP_REQ pReq = pIn->data;

    dwError = LsaSrvModifyGroup2(
                    LsaSrvIpcGetSessionData(pCall),
                    pReq->pszTargetProvider,
                    pReq->pGroupModInfo);

    if (!dwError)
    {
        pOut->tag = LSA2_R_MODIFY_GROUP;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcDeleteObject(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PLSA2_IPC_DELETE_OBJECT_REQ pReq = pIn->data;

    dwError = LsaSrvDeleteObject(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszTargetProvider,
                        pReq->pszSid);

    if (!dwError)
    {
        pOut->tag = LSA2_R_DELETE_OBJECT;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

#define ZERO_STRUCT(_s_) memset((char*)&(_s_),0,sizeof(_s_))

static LWMsgStatus
LsaSrvIpcGetMetrics(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID pMetricPack = NULL;
    PLSA_METRIC_PACK pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;
    DWORD dwInfoLevel = *(PDWORD)pIn->data;

    dwError = LsaSrvGetMetrics(
                        LsaSrvIpcGetSessionData(pCall),
                        dwInfoLevel,
                        &pMetricPack);

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwInfoLevel = dwInfoLevel;

        switch (pResult->dwInfoLevel)
        {
            case 0:
                pResult->pMetricPack.pMetricPack0 = (PLSA_METRIC_PACK_0)pMetricPack;
                pMetricPack = NULL;
                break;

            case 1:
                pResult->pMetricPack.pMetricPack1 = (PLSA_METRIC_PACK_1)pMetricPack;
                pMetricPack = NULL;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_GET_METRICS_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_METRICS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if(pMetricPack)
    {
        LW_SAFE_FREE_MEMORY(pMetricPack);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaSrvFreeIpcMetriPack(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcProviderIoControl(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    // Do not free pProviderIoControlReq
    PLSA_IPC_PROVIDER_IO_CONTROL_REQ pProviderIoControlReq =
        (PLSA_IPC_PROVIDER_IO_CONTROL_REQ)pIn->data;
    DWORD dwOutputBufferSize = 0;
    PVOID pOutputBuffer = NULL;
    PLSA_DATA_BLOB pBlob = NULL;

    dwError = LsaSrvProviderIoControl(
                  LsaSrvIpcGetSessionData(pCall),
                  pProviderIoControlReq->pszProvider,
                  pProviderIoControlReq->dwIoControlCode,
                  pProviderIoControlReq->dwDataLen,
                  pProviderIoControlReq->pData,
                  &dwOutputBufferSize,
                  &pOutputBuffer);

    if (!dwError)
    {
        if ( dwOutputBufferSize )
        {
            pOut->tag = LSA_R_PROVIDER_IO_CONTROL_SUCCESS_DATA;
            dwError = LsaDataBlobStore(
                          &pBlob,
                          dwOutputBufferSize,
                          pOutputBuffer);
            BAIL_ON_LSA_ERROR(dwError);
            pOut->data = pBlob;
        }
        else
        {
            pOut->tag = LSA_R_PROVIDER_IO_CONTROL_SUCCESS;
            pOut->data = NULL;
        }
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_PROVIDER_IO_CONTROL_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    if ( pOutputBuffer )
    {
        LwFreeMemory(pOutputBuffer);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:

    LsaDataBlobFree( &pBlob );

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcOpenSession(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvOpenSession(
                    LsaSrvIpcGetSessionData(pCall),
                    (PSTR)pIn->data);

    if (!dwError)
    {
        pOut->tag = LSA_R_OPEN_SESSION_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_OPEN_SESSION_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcCloseSession(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvCloseSession(
                    LsaSrvIpcGetSessionData(pCall),
                    (PSTR)pIn->data);

    if (!dwError)
    {
        pOut->tag = LSA_R_CLOSE_SESSION_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_CLOSE_SESSION_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetStatus(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSASTATUS pLsaStatus = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvGetStatus(
                    LsaSrvIpcGetSessionData(pCall),
                    (PSTR)pIn->data,
                    &pLsaStatus);

    if (!dwError)
    {
        pOut->tag = LSA_R_GET_STATUS_SUCCESS;
        pOut->data = pLsaStatus;
        pLsaStatus = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_STATUS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if(pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcSetTraceInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_SET_TRACE_INFO_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvSetTraceFlags(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pTraceFlagArray,
                        pReq->dwNumFlags);

    if (!dwError)
    {
        pOut->tag = LSA_R_SET_TRACE_INFO_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_SET_TRACE_INFO_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetTraceInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_TRACE_INFO_LIST pResult = NULL;
    PLSA_TRACE_INFO pTraceInfo = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvGetTraceInfo(
                        LsaSrvIpcGetSessionData(pCall),
                        *(PDWORD)pIn->data,
                        &pTraceInfo);

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwNumFlags = 1;
        pResult->pTraceInfoArray = pTraceInfo;
        pTraceInfo = NULL;

        pOut->tag = LSA_R_GET_TRACE_INFO_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_TRACE_INFO_FAILURE;
        pOut->data = pError;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pTraceInfo);

    return MAP_LW_ERROR_IPC(dwError);

error:
    if (pResult)
    {
        LW_SAFE_FREE_MEMORY(pResult->pTraceInfoArray);
        LwFreeMemory(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumTraceInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_TRACE_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(sizeof(*pResult),
                               (PVOID)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumTraceFlags(
                       LsaSrvIpcGetSessionData(pCall),
                       &pResult->pTraceInfoArray,
                       &pResult->dwNumFlags);

    if (!dwError)
    {
        pOut->tag = LSA_R_ENUM_TRACE_INFO_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ENUM_TRACE_INFO_FAILURE;
        pOut->data = pError;
        goto error;
    }

cleanup:

    return MAP_LW_ERROR_IPC(dwError);

error:

    if (pResult)
    {
        LW_SAFE_FREE_MEMORY(pResult->pTraceInfoArray);
        LwFreeMemory(pResult);
    }

    goto cleanup;
}

static void
LsaSrvCleanupEnumHandle(
    void* pData
    )
{
    LsaSrvCloseEnum(NULL, pData);
}

static LWMsgStatus
LsaSrvIpcAddUser2(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PLSA2_IPC_ADD_USER_REQ pReq = pIn->data;

    dwError = LsaSrvAddUser2(
        LsaSrvIpcGetSessionData(pCall),
        pReq->pszTargetProvider,
        pReq->pUserAddInfo);

    if (!dwError)
    {
        pOut->tag = LSA2_R_ADD_USER;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcModifyUser2(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PLSA2_IPC_MODIFY_USER_REQ pReq = pIn->data;

    dwError = LsaSrvModifyUser2(
                    LsaSrvIpcGetSessionData(pCall),
                    pReq->pszTargetProvider,
                    pReq->pUserModInfo);

    if (!dwError)
    {
        pOut->tag = LSA2_R_MODIFY_USER;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static
LWMsgStatus
LsaSrvIpcGetPamConfig(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN OPTIONAL void* data
    )
{
    DWORD dwError = 0;
    PLSA_PAM_CONFIG pPamConfig = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvGetPamConfig(LsaSrvIpcGetSessionData(pCall),
                               &pPamConfig);

    if (!dwError)
    {
        pOut->tag = LSA_R_GET_PAM_CONFIG_SUCCESS;
        pOut->data = pPamConfig;
        pPamConfig = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_PAM_CONFIG_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if (pPamConfig)
    {
        LsaUtilFreePamConfig(pPamConfig);
    }
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcFindObjects(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_FIND_OBJECTS_REQ pReq = pIn->data;
    PLSA2_IPC_FIND_OBJECTS_RES pRes = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_IPC_ERROR pError = NULL;

    switch (pReq->QueryType)
    {
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        if (pReq->IpcQueryType != LSA2_IPC_QUERY_DWORDS)
        {
            dwError = LW_ERROR_INTERNAL;
        }
        break;
    case LSA_QUERY_TYPE_BY_DN:
    case LSA_QUERY_TYPE_BY_SID:
    case LSA_QUERY_TYPE_BY_NT4:
    case LSA_QUERY_TYPE_BY_ALIAS:
    case LSA_QUERY_TYPE_BY_UPN:
    case LSA_QUERY_TYPE_BY_NAME:
        if (pReq->IpcQueryType != LSA2_IPC_QUERY_STRINGS)
        {
            dwError = LW_ERROR_INTERNAL;
        }
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!dwError)
    {
        dwError = LsaSrvFindObjects(
            LsaSrvIpcGetSessionData(pCall),
            pReq->pszTargetProvider,
            pReq->FindFlags,
            pReq->ObjectType,
            pReq->QueryType,
            pReq->dwCount,
            pReq->QueryList,
            &ppObjects);
    }

    if (!dwError)
    {
        LsaFreeSecurityObjectListPrivateAttrs(
            pReq->dwCount,
            ppObjects);

        dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        BAIL_ON_LSA_ERROR(dwError);

        pRes->dwCount = pReq->dwCount;
        pRes->ppObjects = ppObjects;
        ppObjects = NULL;

        pOut->tag = LSA2_R_FIND_OBJECTS;
        pOut->data = pRes;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(pReq->dwCount, ppObjects);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcOpenEnumObjects(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_OPEN_ENUM_OBJECTS_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hEnum = NULL;
    LWMsgHandle* pHandle = NULL;

    dwError = LsaSrvOpenEnumObjects(
            LsaSrvIpcGetSessionData(pCall),
            pReq->pszTargetProvider,
            &hEnum,
            pReq->FindFlags,
            pReq->ObjectType,
            pReq->pszDomainName);
    
    if (!dwError)
    {
        dwError = LsaSrvIpcRegisterHandle(
            pCall,
            "LSA2_IPC_ENUM_HANDLE",
            hEnum,
            LsaSrvCleanupEnumHandle,
            &pHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_OPEN_ENUM_OBJECTS;
        pOut->data = pHandle;

        LsaSrvIpcRetainHandle(pCall, pHandle);
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumObjects(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_ENUM_OBJECTS_REQ pReq = pIn->data;
    PLSA2_IPC_ENUM_OBJECTS_RES pRes = NULL;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hEnum = NULL;

    dwError = LsaSrvIpcGetHandle(
        pCall,
        pReq->hEnum,
        &hEnum);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumObjects(
        LsaSrvIpcGetSessionData(pCall),
        hEnum,
        pReq->dwMaxObjectsCount,
        &dwObjectsCount,
        &ppObjects);

    if (!dwError)
    {
        LsaFreeSecurityObjectListPrivateAttrs(
            dwObjectsCount,
            ppObjects);

        dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        BAIL_ON_LSA_ERROR(dwError);

        pRes->dwObjectsCount = dwObjectsCount;
        pRes->ppObjects = ppObjects;
        ppObjects = NULL;

        pOut->tag = LSA2_R_ENUM_OBJECTS;
        pOut->data = pRes;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwObjectsCount, ppObjects);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcOpenEnumMembers(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_OPEN_ENUM_MEMBERS_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hEnum = NULL;
    LWMsgHandle* pHandle = NULL;

    dwError = LsaSrvOpenEnumMembers(
            LsaSrvIpcGetSessionData(pCall),
            pReq->pszTargetProvider,
            &hEnum,
            pReq->FindFlags,
            pReq->pszSid);
    
    if (!dwError)
    {
        dwError = LsaSrvIpcRegisterHandle(
            pCall,
            "LSA2_IPC_ENUM_HANDLE",
            hEnum,
            LsaSrvCleanupEnumHandle,
            &pHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_OPEN_ENUM_MEMBERS;
        pOut->data = pHandle;

        LsaSrvIpcRetainHandle(pCall, pHandle);
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumMembers(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_ENUM_MEMBERS_REQ pReq = pIn->data;
    PLSA2_IPC_ENUM_MEMBERS_RES pRes = NULL;
    DWORD dwSidCount = 0;
    PSTR* ppszMemberSids = NULL;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hEnum = NULL;

    dwError = LsaSrvIpcGetHandle(
        pCall,
        pReq->hEnum,
        &hEnum);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumMembers(
        LsaSrvIpcGetSessionData(pCall),
        hEnum,
        pReq->dwMaxSidCount,
        &dwSidCount,
        &ppszMemberSids);

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        BAIL_ON_LSA_ERROR(dwError);

        pRes->dwSidCount = dwSidCount;
        pRes->ppszMemberSids = ppszMemberSids;
        ppszMemberSids = NULL;

        pOut->tag = LSA2_R_ENUM_MEMBERS;
        pOut->data = pRes;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    if (ppszMemberSids)
    {
        LwFreeStringArray(ppszMemberSids, dwSidCount);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcQueryMemberOf(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_QUERY_MEMBER_OF_REQ pReq = pIn->data;
    PLSA2_IPC_QUERY_MEMBER_OF_RES pRes = NULL;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSids = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvQueryMemberOf(
            LsaSrvIpcGetSessionData(pCall),
            pReq->pszTargetProvider,
            pReq->FindFlags,
            pReq->dwSidCount,
            pReq->ppszSids,
            &dwGroupSidCount,
            &ppszGroupSids);

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        BAIL_ON_LSA_ERROR(dwError);

        pRes->dwGroupSidCount = dwGroupSidCount;
        pRes->ppszGroupSids = ppszGroupSids;
        ppszGroupSids = NULL;

        pOut->tag = LSA2_R_QUERY_MEMBER_OF;
        pOut->data = pRes;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    if (ppszGroupSids)
    {
        LwFreeStringArray(ppszGroupSids, dwGroupSidCount);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcFindGroupAndExpandedMembers(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_REQ pReq = pIn->data;
    PLSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_RES pRes = NULL;
    PLSA_SECURITY_OBJECT pGroupObject = NULL;
    DWORD dwMemberObjectCount = 0;
    PLSA_SECURITY_OBJECT* ppMemberObjects = NULL;
    PLSA_IPC_ERROR pError = NULL;

    switch (pReq->QueryType)
    {
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        if (pReq->IpcQueryType != LSA2_IPC_QUERY_DWORDS)
        {
            dwError = LW_ERROR_INTERNAL;
        }
        break;
    case LSA_QUERY_TYPE_BY_DN:
    case LSA_QUERY_TYPE_BY_SID:
    case LSA_QUERY_TYPE_BY_NT4:
    case LSA_QUERY_TYPE_BY_ALIAS:
    case LSA_QUERY_TYPE_BY_UPN:
    case LSA_QUERY_TYPE_BY_NAME:
        if (pReq->IpcQueryType != LSA2_IPC_QUERY_STRINGS)
        {
            dwError = LW_ERROR_INTERNAL;
        }
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!dwError)
    {
        dwError = LsaSrvFindGroupAndExpandedMembers(
            LsaSrvIpcGetSessionData(pCall),
            pReq->pszTargetProvider,
            pReq->FindFlags,
            pReq->QueryType,
            pReq->QueryItem,
            &pGroupObject,
            &dwMemberObjectCount,
            &ppMemberObjects);
    }

    if (!dwError)
    {
        LsaFreeSecurityObjectListPrivateAttrs(
            dwMemberObjectCount,
            ppMemberObjects);

        dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        BAIL_ON_LSA_ERROR(dwError);

        pRes->pGroup = pGroupObject;
        pRes->dwMemberObjectCount = dwMemberObjectCount;
        pRes->ppMemberObjects = ppMemberObjects;

        pGroupObject = NULL;
        ppMemberObjects = NULL;

        pOut->tag = LSA2_R_FIND_GROUP_AND_EXPANDED_MEMBERS;
        pOut->data = pRes;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    if (pGroupObject)
    {
        LsaUtilFreeSecurityObject(pGroupObject);
    }
    if (ppMemberObjects)
    {
        LsaUtilFreeSecurityObjectList(dwMemberObjectCount, ppMemberObjects);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcCloseEnum(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvIpcUnregisterHandle(pCall, pIn->data);
    if (!dwError)
    {
        pOut->tag = LSA2_R_CLOSE_ENUM;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);
        
        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetSmartCardUserObject(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA2_IPC_GET_SMART_CARD_USER_RES pRes = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSTR pszSmartCardReader = NULL;

    dwError = LsaSrvGetSmartCardUserObject(
        LsaSrvIpcGetSessionData(pCall),
        &pObject, &pszSmartCardReader);

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        BAIL_ON_LSA_ERROR(dwError);

        pRes->pObject = pObject;
        pObject = NULL;
        pRes->pszSmartCardReader = pszSmartCardReader;
        pszSmartCardReader = NULL;

        pOut->tag = LSA2_R_GET_SMARTCARD_USER_OBJECT;
        pOut->data = pRes;
    }
    else
    {
        PLSA_IPC_ERROR pError = NULL;

        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA2_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    LW_SAFE_FREE_STRING(pszSmartCardReader);

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}

static LWMsgDispatchSpec gMessageHandlers[] =
{
    LWMSG_DISPATCH_BLOCK(LSA_Q_AUTH_USER_PAM, LsaSrvIpcAuthenticateUserPam),
    LWMSG_DISPATCH_BLOCK(LSA_Q_AUTH_USER_EX, LsaSrvIpcAuthenticateUserEx),
    LWMSG_DISPATCH_BLOCK(LSA_Q_VALIDATE_USER, LsaSrvIpcValidateUser),
    LWMSG_DISPATCH_BLOCK(LSA_Q_CHANGE_PASSWORD, LsaSrvIpcChangePassword),
    LWMSG_DISPATCH_BLOCK(LSA_Q_SET_PASSWORD, LsaSrvIpcSetPassword),
    LWMSG_DISPATCH_BLOCK(LSA_Q_OPEN_SESSION, LsaSrvIpcOpenSession),
    LWMSG_DISPATCH_BLOCK(LSA_Q_CLOSE_SESSION, LsaSrvIpcCloseSession),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_METRICS, LsaSrvIpcGetMetrics),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_STATUS, LsaSrvIpcGetStatus),
    LWMSG_DISPATCH_BLOCK(LSA_Q_CHECK_USER_IN_LIST, LsaSrvIpcCheckUserInList),
    LWMSG_DISPATCH_BLOCK(LSA_Q_BEGIN_ENUM_NSS_ARTEFACTS, LsaSrvIpcBeginEnumNSSArtefacts),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ENUM_NSS_ARTEFACTS, LsaSrvIpcEnumNSSArtefacts),
    LWMSG_DISPATCH_BLOCK(LSA_Q_END_ENUM_NSS_ARTEFACTS, LsaSrvIpcEndEnumNSSArtefacts),
    LWMSG_DISPATCH_BLOCK(LSA_Q_FIND_NSS_ARTEFACT_BY_KEY, LsaSrvIpcFindNSSArtefactByKey),
    LWMSG_DISPATCH_BLOCK(LSA_Q_SET_TRACE_INFO, LsaSrvIpcSetTraceInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_TRACE_INFO, LsaSrvIpcGetTraceInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ENUM_TRACE_INFO, LsaSrvIpcEnumTraceInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_PROVIDER_IO_CONTROL, LsaSrvIpcProviderIoControl),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_PAM_CONFIG, LsaSrvIpcGetPamConfig),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_FIND_OBJECTS, LsaSrvIpcFindObjects),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_OPEN_ENUM_OBJECTS, LsaSrvIpcOpenEnumObjects),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_ENUM_OBJECTS, LsaSrvIpcEnumObjects),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_OPEN_ENUM_MEMBERS, LsaSrvIpcOpenEnumMembers),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_ENUM_MEMBERS, LsaSrvIpcEnumMembers),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_QUERY_MEMBER_OF, LsaSrvIpcQueryMemberOf),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_FIND_GROUP_AND_EXPANDED_MEMBERS, LsaSrvIpcFindGroupAndExpandedMembers),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_CLOSE_ENUM, LsaSrvIpcCloseEnum),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_MODIFY_USER, LsaSrvIpcModifyUser2),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_ADD_GROUP, LsaSrvIpcAddGroup2),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_MODIFY_GROUP, LsaSrvIpcModifyGroup2),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_DELETE_OBJECT, LsaSrvIpcDeleteObject),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_ADD_USER, LsaSrvIpcAddUser2),
    LWMSG_DISPATCH_BLOCK(LSA2_Q_GET_SMARTCARD_USER_OBJECT, LsaSrvIpcGetSmartCardUserObject),
    LWMSG_DISPATCH_BLOCK(LSA_PRIVS_Q_ENUM_PRIVILEGES_SIDS, LsaSrvIpcPrivsEnumPrivilegesSids),
    LWMSG_DISPATCH_BLOCK(LSA_PRIVS_Q_ADD_ACCOUNT_RIGHTS, LsaSrvIpcPrivsAddAccountRights),
    LWMSG_DISPATCH_BLOCK(LSA_PRIVS_Q_REMOVE_ACCOUNT_RIGHTS, LsaSrvIpcPrivsRemoveAccountRights),
    LWMSG_DISPATCH_BLOCK(LSA_PRIVS_Q_ENUM_ACCOUNT_RIGHTS, LsaSrvIpcPrivsEnumAccountRights),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
LsaSrvGetDispatchSpec(
    void
    )
{
    return gMessageHandlers;
}
