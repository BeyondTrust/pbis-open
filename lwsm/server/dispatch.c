/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        dispatch.c
 *
 * Abstract:
 *
 *        Server call dispatch functions and table
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwSmGetCallUid(
    LWMsgCall* pCall,
    uid_t* pUid
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    LWMsgSecurityToken* pToken = lwmsg_session_get_peer_security_token(pSession);

    if (!pToken || strcmp(lwmsg_security_token_get_type(pToken), "local"))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_ERROR(dwError);
    }

    dwError = MAP_LWMSG_STATUS(lwmsg_local_token_get_eid(pToken, pUid, NULL));
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    *pUid = (uid_t) -1;
    goto cleanup;
}

static
void
LwSmCleanupHandle(void* pData)
{
    LW_SERVICE_HANDLE hHandle = pData;

    LwSmSrvReleaseHandle(hHandle);
}

static
DWORD
LwSmRegisterAndRetainHandle(
    LWMsgCall* pCall,
    LW_SERVICE_HANDLE hHandle,
    LWMsgHandle** ppHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    
    dwError = MAP_LWMSG_STATUS(lwmsg_session_register_handle(
                                   pSession,
                                   "LW_SERVICE_HANDLE",
                                   hHandle,
                                   LwSmCleanupHandle,
                                   ppHandle));
    BAIL_ON_ERROR(dwError);

    lwmsg_session_retain_handle(pSession, *ppHandle);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmUnregisterHandle(
    LWMsgCall* pCall,
    LWMsgHandle* pHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    
    dwError = MAP_LWMSG_STATUS(lwmsg_session_unregister_handle(
                                   pSession,
                                   pHandle));
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmGetHandle(
    LWMsgCall* pCall,
    LWMsgHandle* pHandle,
    PLW_SERVICE_HANDLE phHandle
    )
{
    DWORD dwError = ERROR_SUCCESS;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_STATUS(
        lwmsg_session_get_handle_data(
            pSession,
            pHandle,
            OUT_PPVOID(phHandle)));
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}

static
LWMsgStatus
LwSmMapLwError(
    DWORD dwError
    )
{
    switch (dwError)
    {
    case 0:
        return LWMSG_STATUS_SUCCESS;
    default:
        return LWMSG_STATUS_ERROR;
    }
}

static
DWORD
LwSmSetError(
    LWMsgParams* pOut,
    DWORD dwErrorCode
    )
{
    DWORD dwError = 0;
    PDWORD pdwBox = NULL;

    dwError = LwAllocateMemory(sizeof(*pdwBox), (PVOID*) (PVOID) &pdwBox);
    BAIL_ON_ERROR(dwError);

    *pdwBox = dwErrorCode;
    pOut->tag = SM_IPC_ERROR;
    pOut->data = pdwBox;

error:

    return dwError;
}

static
LWMsgStatus
LwSmDispatchAcquireServiceHandle(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    LWMsgHandle* pHandle = NULL;

    dwError = LwSmSrvAcquireServiceHandle((PCWSTR) pIn->data, &hHandle);
    
    if (dwError == 0)
    {
        dwError = LwSmRegisterAndRetainHandle(pCall, hHandle, &pHandle);
        BAIL_ON_ERROR(dwError);

        pOut->tag = SM_IPC_ACQUIRE_SERVICE_HANDLE_RES;
        pOut->data = pHandle;
        hHandle = NULL;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    if (hHandle)
    {
        LwSmSrvReleaseHandle(hHandle);
    }

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchReleaseServiceHandle(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LWMsgHandle* pHandle = pIn->data;

    dwError = LwSmUnregisterHandle(pCall, pHandle);
    
    if (dwError == 0)
    {
        pOut->tag = SM_IPC_RELEASE_SERVICE_HANDLE_RES;
        pOut->data = NULL;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchEnumerateServices(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceNames = NULL;
    
    dwError = LwSmSrvEnumerateServices(&ppwszServiceNames);
    
    if (dwError == 0)
    {
        pOut->tag = SM_IPC_ENUMERATE_SERVICES_RES;
        pOut->data = ppwszServiceNames;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    
cleanup:
    
    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchGetServiceStatus(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    PLW_SERVICE_STATUS pStatus = NULL;

    dwError = LwSmGetHandle(pCall, (LWMsgHandle*) pIn->data, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pStatus), OUT_PPVOID(&pStatus));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmSrvGetServiceStatus(hHandle, pStatus);
    
    if (dwError == 0)
    {
        pOut->tag = SM_IPC_QUERY_SERVICE_STATUS_RES;
        pOut->data = pStatus;
        pStatus = NULL;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    
cleanup:

    LW_SAFE_FREE_MEMORY(pStatus);
    
    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchStartService(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    uid_t uid = -1;

    dwError = LwSmGetHandle(pCall, (LWMsgHandle*) pIn->data, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid == 0)
    {
        dwError = LwSmSrvStartService(hHandle);
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

    if (dwError == 0)
    {
        pOut->tag = SM_IPC_START_SERVICE_RES;
        pOut->data = NULL;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    
cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchStopService(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    uid_t uid = -1;

    dwError = LwSmGetHandle(pCall, (LWMsgHandle*) pIn->data, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid == 0)
    {
        dwError = LwSmSrvStopService(hHandle);
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }
    
    if (dwError == 0)
    {
        pOut->tag = SM_IPC_STOP_SERVICE_RES;
        pOut->data = NULL;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    
cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchRefreshService(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    uid_t uid = -1;

    dwError = LwSmGetHandle(pCall, (LWMsgHandle*) pIn->data, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid == 0)
    {
        dwError = LwSmTableRefreshEntry(hHandle->pEntry);
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }
    
    if (dwError == 0)
    {
        pOut->tag = SM_IPC_REFRESH_SERVICE_RES;
        pOut->data = NULL;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    
cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchGetServiceInfo(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    PLW_SERVICE_INFO pInfo = NULL;

    dwError = LwSmGetHandle(pCall, (LWMsgHandle*) pIn->data, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmSrvGetServiceInfo(hHandle, &pInfo);
    
    if (dwError == 0)
    {
        pOut->tag = SM_IPC_QUERY_SERVICE_INFO_RES;
        pOut->data = pInfo;
        pInfo = NULL;
    }
    else
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    
cleanup:

    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

typedef struct _SM_WAIT_CONTEXT
{
    PSM_TABLE_ENTRY pEntry;
    LWMsgCall* pCall;
    LWMsgParams* pOut;
} SM_WAIT_CONTEXT, *PSM_WAIT_CONTEXT;

static
VOID
LwSmWaitNotifyCallback(
    LW_SERVICE_STATE state,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_WAIT_CONTEXT pContext = pData;
    PLW_SERVICE_STATE pState = NULL;

    dwError = LwAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));

    if (dwError == 0)
    {
        *pState = state;
        pContext->pOut->data = pState;
        pContext->pOut->tag = SM_IPC_WAIT_SERVICE_RES;
    }
    else
    {
        dwError = LwSmSetError(pContext->pOut, dwError);
    }
        
    lwmsg_call_complete(pContext->pCall, LwSmMapLwError(dwError));

    LwFreeMemory(pContext);
}

static
VOID
LwSmWaitCancelCallback(
    LWMsgCall* pCall,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_WAIT_CONTEXT pContext = (PSM_WAIT_CONTEXT) pData;

    dwError = LwSmTableUnregisterEntryNotify(pContext->pEntry, LwSmWaitNotifyCallback, pData);
    
    if (dwError == LW_ERROR_SUCCESS)
    {
        lwmsg_call_complete(pContext->pCall, LWMSG_STATUS_CANCELLED);
        
        LwFreeMemory(pContext);
    }
}

static
LWMsgStatus
LwSmDispatchWaitService(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_IPC_WAIT_STATE_CHANGE_REQ pReq = pIn->data;
    PSM_WAIT_CONTEXT pContext = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;

    dwError = LwSmGetHandle(pCall, pReq->hHandle, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    BAIL_ON_ERROR(dwError);

    pContext->pEntry = hHandle->pEntry;
    pContext->pCall = pCall;
    pContext->pOut = pOut;

    lwmsg_call_pend(pCall, LwSmWaitCancelCallback, pContext);

    dwError = LwSmTableRegisterEntryNotify(
        pContext->pEntry,
        pReq->state,
        LwSmWaitNotifyCallback,
        pContext);
    BAIL_ON_ERROR(dwError);

    pContext = NULL;
    
cleanup:

    LW_SAFE_FREE_MEMORY(pContext);

    return dwError ? LwSmMapLwError(dwError) : LWMSG_STATUS_PENDING;

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchSetLogInfo(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_SET_LOG_INFO_REQ pReq = pIn->data;
    uid_t uid = 0;
    LW_SERVICE_HANDLE hHandle = NULL;

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid == 0)
    {
        if (pReq->hHandle)
        {
            dwError = LwSmGetHandle(pCall, pReq->hHandle, &hHandle);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmTableSetEntryLogInfo(
                hHandle->pEntry,
                pReq->pFacility,
                pReq->type,
                pReq->pszTarget);
            BAIL_ON_ERROR(dwError);
        }
        else switch (pReq->type)
        {
        case LW_SM_LOGGER_NONE:
            dwError = LwSmSetLogger(pReq->pFacility, NULL, NULL);
            break;
        case LW_SM_LOGGER_FILE:
            dwError = LwSmSetLoggerToPath(pReq->pFacility, pReq->pszTarget);
            break;
        case LW_SM_LOGGER_SYSLOG:
            dwError = LwSmSetLoggerToSyslog(pReq->pFacility);
            break;
        case LW_SM_LOGGER_DEFAULT:
            if (!pReq->pFacility)
            {
                dwError = ERROR_INVALID_PARAMETER;
            }
            else
            {
                dwError = LwSmSetLoggerToDefault(pReq->pFacility);
            }
            break;
        }
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

    if (dwError)
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        pOut->tag = SM_IPC_SET_LOG_INFO_RES;
        pOut->data = NULL;
    }
    
cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchGetLogState(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_GET_LOG_STATE_REQ pReq = pIn->data;
    PSM_GET_LOG_STATE_RES pRes = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;

    dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
    BAIL_ON_ERROR(dwError);

    if (pReq->hHandle)
    {
        dwError = LwSmGetHandle(pCall, pReq->hHandle, &hHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableGetEntryLogState(hHandle->pEntry, pReq->pFacility, &pRes->type, &pRes->pszTarget, &pRes->Level);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LwSmGetLoggerState(pReq->pFacility, &pRes->type, &pRes->pszTarget, &pRes->Level);
        BAIL_ON_ERROR(dwError);
    }

    pOut->tag = SM_IPC_GET_LOG_STATE_RES;
    pOut->data = pRes;
    
cleanup:

    return LwSmMapLwError(dwError);

error:

    LW_SAFE_FREE_MEMORY(pRes);

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchSetLogLevel(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_SET_LOG_LEVEL_REQ pReq = pIn->data;
    uid_t uid = 0;
    LW_SERVICE_HANDLE hHandle = NULL;

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid == 0)
    {
        if (pReq->hHandle)
        {
            dwError = LwSmGetHandle(pCall, pReq->hHandle, &hHandle);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmTableSetEntryLogLevel(
                hHandle->pEntry,
                pReq->pFacility,
                pReq->Level);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            LwSmSetMaxLogLevel(pReq->pFacility, pReq->Level);
        }
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

    if (dwError)
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        pOut->tag = SM_IPC_SET_LOG_LEVEL_RES;
        pOut->data = NULL;
    }
    
cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchEnumFacilities(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    PWSTR* ppFacilities = NULL;

    if (pIn->data)
    {
        dwError = LwSmGetHandle(pCall, (LWMsgHandle*) pIn->data, &hHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableGetEntryFacilityList(hHandle->pEntry, &ppFacilities);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LwSmGetLogFacilityList(&ppFacilities);
        BAIL_ON_ERROR(dwError);
    }

    pOut->tag = SM_IPC_ENUM_FACILITIES_RES;
    pOut->data = ppFacilities;

cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}


static
LWMsgStatus
LwSmDispatchRefresh(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    uid_t uid = 0;

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid == 0)
    {
        dwError = LwSmPopulateTable();
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

    if (dwError)
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        pOut->tag = SM_IPC_REFRESH_RES;
        pOut->data = NULL;
    }

cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchShutdown(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    uid_t uid = 0;

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid == 0)
    {
        dwError = LwMapErrnoToLwError(kill(getpid(), SIGTERM));
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

    if (dwError)
    {
        dwError = LwSmSetError(pOut, dwError);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        pOut->tag = SM_IPC_SHUTDOWN_RES;
        pOut->data = NULL;
    }

cleanup:

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchSetGlobal(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_SET_GLOBAL_REQ pReq = pIn->data;
    uid_t uid = 0;

    dwError = LwSmGetCallUid(pCall, &uid);
    BAIL_ON_ERROR(dwError);

    if (uid != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_ERROR(dwError);
    }

    switch (pReq->Setting)
    {
    case LW_SM_GLOBAL_SETTING_WATCHDOG:
        if (pReq->Value.Type != SM_GLOBAL_TYPE_BOOLEAN)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_ERROR(dwError);
        }

        gState.bWatchdog = pReq->Value.Value.Boolean;
        break;
    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    pOut->tag = SM_IPC_SET_GLOBAL_RES;
    pOut->data = NULL;

cleanup:

    if (dwError)
    {
        dwError = LwSmSetError(pOut, dwError);
    }

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgStatus
LwSmDispatchGetGlobal(
    LWMsgCall* pCall,
    LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSM_GET_GLOBAL_REQ pReq = pIn->data;
    PSM_GLOBAL_VALUE pValue = NULL;

    dwError = LwAllocateMemory(sizeof(*pValue), OUT_PPVOID(&pValue));
    BAIL_ON_ERROR(dwError);

    switch (pReq->Setting)
    {
    case LW_SM_GLOBAL_SETTING_WATCHDOG:
        pValue->Type = SM_GLOBAL_TYPE_BOOLEAN;
        pValue->Value.Boolean = gState.bWatchdog;
        break;
    default:
        /* Should not be reached */
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    pOut->tag = SM_IPC_GET_GLOBAL_RES;
    pOut->data = pValue;
    pValue = NULL;

cleanup:

    if (dwError)
    {
        dwError = LwSmSetError(pOut, dwError);
    }

    LW_SAFE_FREE_MEMORY(pValue);

    return LwSmMapLwError(dwError);

error:

    goto cleanup;
}

static
LWMsgDispatchSpec gDispatchSpec[] =
{
    LWMSG_DISPATCH_BLOCK(SM_IPC_ACQUIRE_SERVICE_HANDLE_REQ, LwSmDispatchAcquireServiceHandle),
    LWMSG_DISPATCH_BLOCK(SM_IPC_RELEASE_SERVICE_HANDLE_REQ, LwSmDispatchReleaseServiceHandle),
    LWMSG_DISPATCH_BLOCK(SM_IPC_ENUMERATE_SERVICES_REQ, LwSmDispatchEnumerateServices),
    LWMSG_DISPATCH_BLOCK(SM_IPC_START_SERVICE_REQ, LwSmDispatchStartService),
    LWMSG_DISPATCH_BLOCK(SM_IPC_STOP_SERVICE_REQ, LwSmDispatchStopService),
    LWMSG_DISPATCH_BLOCK(SM_IPC_REFRESH_SERVICE_REQ, LwSmDispatchRefreshService),
    LWMSG_DISPATCH_BLOCK(SM_IPC_QUERY_SERVICE_STATUS_REQ, LwSmDispatchGetServiceStatus),
    LWMSG_DISPATCH_BLOCK(SM_IPC_QUERY_SERVICE_INFO_REQ, LwSmDispatchGetServiceInfo),
    LWMSG_DISPATCH_NONBLOCK(SM_IPC_WAIT_SERVICE_REQ, LwSmDispatchWaitService),
    LWMSG_DISPATCH_BLOCK(SM_IPC_SET_LOG_INFO_REQ, LwSmDispatchSetLogInfo),
    LWMSG_DISPATCH_BLOCK(SM_IPC_GET_LOG_STATE_REQ, LwSmDispatchGetLogState),
    LWMSG_DISPATCH_BLOCK(SM_IPC_ENUM_FACILITIES_REQ, LwSmDispatchEnumFacilities),
    LWMSG_DISPATCH_BLOCK(SM_IPC_SET_LOG_LEVEL_REQ, LwSmDispatchSetLogLevel),
    LWMSG_DISPATCH_BLOCK(SM_IPC_REFRESH_REQ, LwSmDispatchRefresh),
    LWMSG_DISPATCH_NONBLOCK(SM_IPC_SHUTDOWN_REQ, LwSmDispatchShutdown),
    LWMSG_DISPATCH_NONBLOCK(SM_IPC_SET_GLOBAL_REQ, LwSmDispatchSetGlobal),
    LWMSG_DISPATCH_NONBLOCK(SM_IPC_GET_GLOBAL_REQ, LwSmDispatchGetGlobal),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
LwSmGetDispatchSpec(
    VOID
    )
{
    return gDispatchSpec;
}
