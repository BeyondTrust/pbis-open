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

DWORD
LwTaskGetTypes(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE*              pTaskTypes,
    PDWORD                     pdwNumTypes
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pConnection);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskGetSchema(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_ARG_INFO*         ppArgInfoArray,
    PDWORD                     pdwNumArgInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pConnection);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskEnum(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_INFO*             pTaskInfoArray,
    PDWORD                     pdwNumTaskInfos,
    PDWORD                     pdwResume
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pConnection);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskCreate(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_ARG               pArgArray,
    DWORD                      dwNumArgs
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pConnection);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskStart(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId
    )
{
    DWORD dwError = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_STRING(pszTaskId);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_START;
    in.data = (PVOID)pszTaskId;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_START_SUCCESS:

            break;

        case LW_TASK_START_FAILED:

            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;

            break;

        default:

            dwError = LwErrnoToWin32Error(EINVAL);

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

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
LwTaskGetStatus(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId,
    PLW_TASK_STATUS            pStatus
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_STRING(pszTaskId);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskStop(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId
    )
{
    DWORD dwError = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_STRING(pszTaskId);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_STOP;
    in.data = (PVOID)pszTaskId;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_STOP_SUCCESS:

            break;

        case LW_TASK_STOP_FAILED:

            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;

            break;

        default:

            dwError = LwErrnoToWin32Error(EINVAL);

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

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
LwTaskDelete(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId
    )
{
    DWORD dwError = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_STRING(pszTaskId);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_DELETE;
    in.data = (PVOID)pszTaskId;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_DELETE_SUCCESS:

            break;

        case LW_TASK_DELETE_FAILED:

            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;

            break;

        default:

            dwError = LwErrnoToWin32Error(EINVAL);

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

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


