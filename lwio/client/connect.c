/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"

NTSTATUS
LwIoOpenContext(
    PIO_CONTEXT* ppContext
    )
{
    return LwIoOpenContextShared(ppContext);
}

NTSTATUS
LwIoCloseContext(
    PIO_CONTEXT pContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (pContext)
    {
        LwIoFreeMemory(pContext);
    }

    return Status;
}

NTSTATUS
LwIoContextAcquireCall(
    IN PIO_CONTEXT pConnection,
    OUT LWMsgCall** ppCall
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_client_acquire_call(
            pConnection->pClient,
            ppCall));
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

LW_NTSTATUS
LwIoGetPid(
    pid_t* pPid
    )
{
    NTSTATUS status = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    IO_CONTEXT context = {0};

    status = LwIoAcquireContext(&context);
    BAIL_ON_NT_STATUS(status);

    status = LwIoContextAcquireCall(&context, &pCall);
    BAIL_ON_NT_STATUS(status);

    in.tag = LWIO_GET_PID;
    in.data = NULL;

    status = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
    case LWIO_GET_PID_SUCCESS:
        *pPid = *((pid_t*) out.data);
        break;
    case LWIO_GET_PID_FAILED:
        status = ((PLWIO_STATUS_REPLY) out.data)->dwError;
        BAIL_ON_LWIO_ERROR(status);
        break;
    default:
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_LWIO_ERROR(status);
        break;
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    LwIoReleaseContext(&context);

    return status;

error:

    goto cleanup;
}
