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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        logging.c
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
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
