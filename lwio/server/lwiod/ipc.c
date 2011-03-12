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
NTSTATUS
LwIoGetPeerIdentity(
    IN LWMsgCall* pCall,
    OUT uid_t* pUid,
    OUT gid_t* pGid
    )
{
    NTSTATUS ntStatus = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    LWMsgSecurityToken* token = lwmsg_session_get_peer_security_token(pSession);
    uid_t uid = (uid_t) -1;
    gid_t gid = (gid_t) -1;

    if (token == NULL || strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(ntStatus);
    }

    ntStatus = NtIpcLWMsgStatusToNtStatus(lwmsg_local_token_get_eid(token, &uid, &gid));
    BAIL_ON_LWIO_ERROR(ntStatus);

error:

     if (pUid)
     {
         *pUid = uid;
     }

     if (pGid)
     {
         *pGid = gid;
     }

    return ntStatus;
}

static
NTSTATUS
LwIoVerifyRootAccess(
    IN LWMsgCall* pCall
    )
{
    NTSTATUS ntStatus = 0;
    uid_t uid = 0;

    ntStatus = LwIoGetPeerIdentity(pCall, &uid, NULL);
    BAIL_ON_LWIO_ERROR(ntStatus);

    if (uid != 0)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_LWIO_ERROR(ntStatus);
    }

error:

    return ntStatus;
}

static
LWMsgStatus
LwIoDaemonIpcQueryStateDriver(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS ntStatus = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PWSTR pwszDriverName = pIn->data;
    PLWIO_DRIVER_STATE pState = NULL;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    ntStatus = LwIoAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));
    BAIL_ON_LWIO_ERROR(ntStatus);

    ntStatus = LwIoAllocateMemory(sizeof(*pStatusResponse), OUT_PPVOID(&pStatusResponse));
    BAIL_ON_LWIO_ERROR(ntStatus);

    ntStatus = IoMgrQueryStateDriver(pwszDriverName, pState);
    if (ntStatus)
    {
        pStatusResponse->dwError = ntStatus;
        pOut->tag = LWIO_QUERY_STATE_DRIVER_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        ntStatus = 0;
        goto cleanup;
    }

    pOut->tag = LWIO_QUERY_STATE_DRIVER_SUCCESS;
    pOut->data = pState;
    pState = NULL;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pState);
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
    NTSTATUS ntStatus = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PWSTR pwszDriverName = pIn->data;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    ntStatus = LwIoVerifyRootAccess(pCall);
    BAIL_ON_LWIO_ERROR(ntStatus);

    ntStatus = LwIoAllocateMemory(sizeof(*pStatusResponse), OUT_PPVOID(&pStatusResponse));
    BAIL_ON_LWIO_ERROR(ntStatus);

    ntStatus = IoMgrLoadDriver(pwszDriverName);
    
    pStatusResponse->dwError = ntStatus;
    pOut->tag = ntStatus ? LWIO_LOAD_DRIVER_SUCCESS : LWIO_LOAD_DRIVER_FAILED;
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
    NTSTATUS ntStatus = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PWSTR pwszDriverName = pIn->data;
    PLWIO_STATUS_REPLY pStatusResponse = NULL;

    ntStatus = LwIoVerifyRootAccess(pCall);
    BAIL_ON_LWIO_ERROR(ntStatus);

    ntStatus = LwIoAllocateMemory(sizeof(*pStatusResponse), OUT_PPVOID(&pStatusResponse));
    BAIL_ON_LWIO_ERROR(ntStatus);

    ntStatus = IoMgrUnloadDriver(pwszDriverName);
    
    pStatusResponse->dwError = ntStatus;
    pOut->tag = ntStatus ? LWIO_UNLOAD_DRIVER_SUCCESS : LWIO_UNLOAD_DRIVER_FAILED;
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
    NTSTATUS ntStatus = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    pid_t* pPid = NULL;

    ntStatus = LwIoAllocateMemory(sizeof(*pPid), OUT_PPVOID(&pPid));
    BAIL_ON_LWIO_ERROR(ntStatus);

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
    LWMSG_DISPATCH_BLOCK(LWIO_QUERY_STATE_DRIVER, LwIoDaemonIpcQueryStateDriver),
    LWMSG_DISPATCH_BLOCK(LWIO_LOAD_DRIVER, LwIoDaemonIpcLoadDriver),
    LWMSG_DISPATCH_BLOCK(LWIO_UNLOAD_DRIVER, LwIoDaemonIpcUnloadDriver),
    LWMSG_DISPATCH_NONBLOCK(LWIO_GET_PID, LwIoDaemonIpcGetPid),
    LWMSG_DISPATCH_END
};

NTSTATUS
LwIoDaemonIpcAddDispatch(
    IN OUT LWMsgPeer* pServer
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_peer_add_dispatch_spec(
                    pServer,
                    gLwIoDaemonIpcDispatchSpec));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
