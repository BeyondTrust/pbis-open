/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"
#include "ntipcmsg.h"
#include "goto.h"
#include "ntlogmacros.h"
#include "ioinit.h"

static
LWMsgStatus
LwIoDaemonIpcRefreshConfiguration(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(LWIO_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LWIO_ERROR(dwError);

    ntStatus = LwioSrvRefreshConfig(&gLwioServerConfig);

    /* Transmit refresh error to client but do not fail out of dispatch loop */
    if (ntStatus)
    {
        pStatusResponse->dwError = LwNtStatusToWin32Error(ntStatus);
        pOut->tag = LWIO_REFRESH_CONFIG_FAILED;
        pOut->data = (PVOID) pStatusResponse;

        goto cleanup;
    }

    pOut->tag = LWIO_REFRESH_CONFIG_SUCCESS;
    pOut->data = (PVOID) pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcSetLogInfo(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(LWIO_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LWIO_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwioLogSetInfo_r((PLWIO_LOG_INFO)pIn->data);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LWIO_SET_LOG_INFO_FAILED;
        pOut->data = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LWIO_SET_LOG_INFO_SUCCESS;
    pOut->data = pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcGetLogInfo(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;
    PLWIO_LOG_INFO pLogInfo = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(LWIO_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwioLogGetInfo_r(&pLogInfo);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LWIO_GET_LOG_INFO_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LWIO_GET_LOG_INFO_SUCCESS;
    pOut->data = pLogInfo;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcGetDriverStatus(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PWSTR pwszDriverName = pIn->data;
    PLWIO_DRIVER_STATUS pStatus = NULL;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(sizeof(*pStatus), OUT_PPVOID(&pStatus));
    BAIL_ON_LWIO_ERROR(dwError);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LWIO_GET_DRIVER_STATUS_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    *pStatus = IoMgrGetDriverStatus(pwszDriverName);
    pOut->tag = LWIO_GET_DRIVER_STATUS_SUCCESS;
    pOut->data = pStatus;
    pStatus = NULL;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pStatus);
    LWIO_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcLoadDriver(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PWSTR pwszDriverName = pIn->data;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(sizeof(*pStatusResponse), OUT_PPVOID(&pStatusResponse));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = IoMgrLoadDriver(pwszDriverName);
    
    pStatusResponse->dwError = dwError;
    pOut->tag = dwError ? LWIO_LOAD_DRIVER_SUCCESS : LWIO_LOAD_DRIVER_FAILED;
    pOut->data = pStatusResponse;
    pStatusResponse = NULL;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcUnloadDriver(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PWSTR pwszDriverName = pIn->data;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(sizeof(*pStatusResponse), OUT_PPVOID(&pStatusResponse));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = IoMgrUnloadDriver(pwszDriverName);
    
    pStatusResponse->dwError = dwError;
    pOut->tag = dwError ? LWIO_UNLOAD_DRIVER_SUCCESS : LWIO_UNLOAD_DRIVER_FAILED;
    pOut->data = pStatusResponse;
    pStatusResponse = NULL;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcGetPid(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    pid_t* pPid = NULL;

    dwError = SMBAllocateMemory(sizeof(*pPid), OUT_PPVOID(&pPid));
    BAIL_ON_LWIO_ERROR(dwError);

    *pPid = getpid();

    pOut->tag = LWIO_GET_PID_SUCCESS;
    pOut->data = pPid;
    pPid = NULL;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pPid);

    return status;

error:

    goto cleanup;
}

static LWMsgDispatchSpec gLwIoDaemonIpcDispatchSpec[] =
{
    LWMSG_DISPATCH_BLOCK(LWIO_REFRESH_CONFIG, LwIoDaemonIpcRefreshConfiguration),
    LWMSG_DISPATCH_NONBLOCK(LWIO_SET_LOG_INFO, LwIoDaemonIpcSetLogInfo),
    LWMSG_DISPATCH_NONBLOCK(LWIO_GET_LOG_INFO, LwIoDaemonIpcGetLogInfo),
    LWMSG_DISPATCH_BLOCK(LWIO_GET_DRIVER_STATUS, LwIoDaemonIpcGetDriverStatus),
    LWMSG_DISPATCH_BLOCK(LWIO_LOAD_DRIVER, LwIoDaemonIpcLoadDriver),
    LWMSG_DISPATCH_BLOCK(LWIO_UNLOAD_DRIVER, LwIoDaemonIpcUnloadDriver),
    LWMSG_DISPATCH_NONBLOCK(LWIO_GET_PID, LwIoDaemonIpcGetPid),
    LWMSG_DISPATCH_END
};

NTSTATUS
LwIoDaemonIpcAddDispatch(
    IN OUT LWMsgServer* pServer
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_server_add_dispatch_spec(
                    pServer,
                    gLwIoDaemonIpcDispatchSpec));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
