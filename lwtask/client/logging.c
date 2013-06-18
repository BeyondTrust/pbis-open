/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        logging.c
 *
 * Abstract:
 *
 *        Likewise Task System (LWTASK)
 *
 *        Logging
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#include "includes.h"

DWORD
LwTaskSetLogLevel(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_LOG_LEVEL          logLevel
    )
{
    LW_TASK_LOG_INFO logInfo = {0};

    logInfo.maxAllowedLogLevel = logLevel;

    return LwTaskSetLogInfo(pConnection, &logInfo);
}

DWORD
LwTaskSetLogInfo(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PLW_TASK_LOG_INFO          pLogInfo
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_SET_LOG_INFO;
    in.data = pLogInfo;
    
    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));

    switch (out.tag)
    {
        case LW_TASK_SET_LOG_INFO_SUCCESS:
            break;

        case LW_TASK_SET_LOG_INFO_FAILED:
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
LwTaskGetLogInfo(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PLW_TASK_LOG_INFO*         ppLogInfo
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    
    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_GET_LOG_INFO;
    in.data = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_GET_LOG_INFO_SUCCESS:
            *ppLogInfo = out.data;
            out.data = NULL;
            break;

        case LW_TASK_GET_LOG_INFO_FAILED:
            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;
            break;

        default:
            dwError = ERROR_INTERNAL_ERROR;
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
