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

#include "includes.h"

NTSTATUS
LwIoConnectionAcquireCall(
    OUT LWMsgCall** ppCall
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_CONNECTION connection = {0};

    status = LwIoAcquireConnection(&connection);
    BAIL_ON_NT_STATUS(status);

    status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_peer_acquire_call(
            connection.pClient,
            ppCall));
    BAIL_ON_NT_STATUS(status);

error:

    LwIoReleaseConnection(&connection);

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

    status = LwIoConnectionAcquireCall(&pCall);
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

    return status;

error:

    goto cleanup;
}
