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
    LW_TASK_TYPE**             ppTaskTypes,
    PDWORD                     pdwNumTypes
    )
{
    DWORD dwError = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;
    LW_TASK_TYPE* pTaskTypes = NULL;
    DWORD dwNumTypes = 0;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_POINTER(ppTaskTypes);
    BAIL_ON_INVALID_POINTER(pdwNumTypes);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_GET_TYPES;
    in.data = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_GET_TYPES_SUCCESS:

        {
            PLW_TASK_IPC_GET_TYPES pTypes = (PLW_TASK_IPC_GET_TYPES) out.data;
            dwNumTypes = pTypes->dwNumTaskTypes;
            DWORD iType = 0;

            dwError = LwAllocateMemory(
                            sizeof(LW_TASK_TYPE) * pTypes->dwNumTaskTypes,
                            (PVOID*)&pTaskTypes);
            BAIL_ON_LW_TASK_ERROR(dwError);

            for (; iType < pTypes->dwNumTaskTypes; iType++)
            {
                pTaskTypes[iType] = pTypes->pdwTaskTypeArray[iType];
            }
        }

            break;

        case LW_TASK_GET_TYPES_FAILED:

            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;

            break;

        default:

            dwError = LwErrnoToWin32Error(EINVAL);

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppTaskTypes = pTaskTypes;
    *pdwNumTypes = dwNumTypes;

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (ppTaskTypes)
    {
        *ppTaskTypes = NULL;
    }
    if (pdwNumTypes)
    {
        *pdwNumTypes = 0;
    }

    LW_SAFE_FREE_MEMORY(pTaskTypes);

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
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;
    LW_TASK_IPC_GET_SCHEMA schemaRequest =
    {
        .taskType = taskType
    };
    PLW_TASK_ARG_INFO pArgInfoArray = NULL;
    DWORD             dwNumArgInfo  = 0;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_POINTER(ppArgInfoArray);
    BAIL_ON_INVALID_POINTER(pdwNumArgInfo);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_GET_SCHEMA;
    in.data = &schemaRequest;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_GET_SCHEMA_SUCCESS:

        {
            PLW_TASK_IPC_SCHEMA pSchemaInfo = (PLW_TASK_IPC_SCHEMA)out.data;
            if (pSchemaInfo->dwNumArgInfos)
            {
                DWORD iArg = 0;

                dwError = LwAllocateMemory(
                                sizeof(LW_TASK_ARG_INFO) * pSchemaInfo->dwNumArgInfos,
                                (PVOID*)&pArgInfoArray);
                BAIL_ON_LW_TASK_ERROR(dwError);

                dwNumArgInfo = pSchemaInfo->dwNumArgInfos;

                for (; iArg < pSchemaInfo->dwNumArgInfos; iArg++)
                {
                    PLW_TASK_ARG_INFO pSrcArgInfo = &pSchemaInfo->pArgInfoArray[iArg];
                    PLW_TASK_ARG_INFO pTargetArgInfo = &pArgInfoArray[iArg];

                    dwError = LwAllocateString(
                                    pSrcArgInfo->pszArgName,
                                    &pTargetArgInfo->pszArgName);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    pTargetArgInfo->argType = pSrcArgInfo->argType;
                    pTargetArgInfo->dwFlags = pSrcArgInfo->dwFlags;
                }
            }
        }

            break;

        case LW_TASK_GET_SCHEMA_FAILED:

            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;

            break;

        default:

            dwError = LwErrnoToWin32Error(EINVAL);

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppArgInfoArray = pArgInfoArray;
    *pdwNumArgInfo = dwNumArgInfo;

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (ppArgInfoArray)
    {
        *ppArgInfoArray = NULL;
    }
    if (pdwNumArgInfo)
    {
        *pdwNumArgInfo = 0;
    }

    if (pArgInfoArray)
    {
        LwTaskFreeArgInfoArray(pArgInfoArray, dwNumArgInfo);
    }

    goto cleanup;
}

DWORD
LwTaskEnum(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_INFO*             ppTaskInfoArray,
    PDWORD                     pdwNumTaskInfos,
    PDWORD                     pdwNumTotalTaskInfos,
    PDWORD                     pdwResume
    )
{
    DWORD dwError = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;
    LW_TASK_IPC_ENUM_REQUEST enumRequest =
    {
        .taskType = taskType,
        .dwResume = pdwResume ? *pdwResume : 0
    };
    PLW_TASK_INFO pTaskInfoArray = NULL;
    DWORD         dwNumTaskInfos = 0;
    DWORD         dwNumTotalTaskInfos = 0;
    DWORD         dwResume       = 0;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_POINTER(ppTaskInfoArray);
    BAIL_ON_INVALID_POINTER(pdwNumTaskInfos);
    BAIL_ON_INVALID_POINTER(pdwNumTotalTaskInfos);
    BAIL_ON_INVALID_POINTER(pdwResume);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_ENUM;
    in.data = &enumRequest;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_ENUM_SUCCESS:

        {
            PLW_TASK_IPC_ENUM_RESPONSE pEnumResponse =
                                        (PLW_TASK_IPC_ENUM_RESPONSE)out.data;
            if (pEnumResponse->dwNumTaskInfos)
            {
                DWORD iArg = 0;

                dwError = LwAllocateMemory(
                                sizeof(LW_TASK_INFO) * pEnumResponse->dwNumTaskInfos,
                                (PVOID*)&pTaskInfoArray);
                BAIL_ON_LW_TASK_ERROR(dwError);

                dwNumTaskInfos = pEnumResponse->dwNumTaskInfos;

                for (; iArg < pEnumResponse->dwNumTaskInfos; iArg++)
                {
                    PLW_TASK_INFO pSrcTaskInfo = &pEnumResponse->pTaskInfoArray[iArg];
                    PLW_TASK_INFO pTargetTaskInfo = &pTaskInfoArray[iArg];

                    dwError = LwAllocateString(
                                    pSrcTaskInfo->pszTaskId,
                                    &pTargetTaskInfo->pszTaskId);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    if (pSrcTaskInfo->dwNumArgs)
                    {
                        DWORD iTaskArg = 0;

                        dwError = LwAllocateMemory(
                                        sizeof(LW_TASK_ARG) * pSrcTaskInfo->dwNumArgs,
                                        (PVOID*)&pTargetTaskInfo->pArgArray);
                        BAIL_ON_LW_TASK_ERROR(dwError);

                        pTargetTaskInfo->dwNumArgs = pSrcTaskInfo->dwNumArgs;

                        for (; iTaskArg < pSrcTaskInfo->dwNumArgs; iTaskArg++)
                        {
                            PLW_TASK_ARG pSrcTaskArg = &pSrcTaskInfo->pArgArray[iTaskArg];
                            PLW_TASK_ARG pTargetTaskArg = &pTargetTaskInfo->pArgArray[iTaskArg];

                            dwError = LwAllocateString(
                                            pSrcTaskArg->pszArgName,
                                            &pTargetTaskArg->pszArgName);
                            BAIL_ON_LW_TASK_ERROR(dwError);

                            pTargetTaskArg->dwArgType = pSrcTaskArg->dwArgType;

                            if (pSrcTaskArg->pszArgValue)
                            {
                                dwError = LwAllocateString(
                                                pSrcTaskArg->pszArgValue,
                                                &pTargetTaskArg->pszArgValue);
                                BAIL_ON_LW_TASK_ERROR(dwError);
                            }
                        }
                    }
                }
            }

            dwNumTotalTaskInfos = pEnumResponse->dwTotalTaskInfos;
            dwResume = pEnumResponse->dwResume;

        }

            break;

        case LW_TASK_ENUM_FAILED:

            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;

            break;

        default:

            dwError = LwErrnoToWin32Error(EINVAL);

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppTaskInfoArray = pTaskInfoArray;
    *pdwNumTaskInfos = dwNumTaskInfos;
    *pdwNumTotalTaskInfos = dwNumTotalTaskInfos;
    *pdwResume       = dwResume;

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (ppTaskInfoArray)
    {
        *ppTaskInfoArray = NULL;
    }
    if (pdwNumTaskInfos)
    {
        *pdwNumTaskInfos = 0;
    }
    if (pdwNumTotalTaskInfos)
    {
        *pdwNumTotalTaskInfos = 0;
    }
    if (pdwResume)
    {
        *pdwResume = 0;
    }

    if (pTaskInfoArray)
    {
        LwTaskFreeTaskInfoArray(pTaskInfoArray, dwNumTaskInfos);
    }

    goto cleanup;
}

DWORD
LwTaskCreate(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_TYPE               taskType,
    PLW_TASK_ARG               pArgArray,
    DWORD                      dwNumArgs,
    PSTR*                      ppszTaskId
    )
{
    DWORD dwError = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;
    LW_TASK_IPC_CREATE_ARGS createArgs =
    {
        .taskType  = taskType,
        .dwNumArgs = dwNumArgs,
        .pArgArray = pArgArray
    };
    PSTR pszTaskId = NULL;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_POINTER(pArgArray);
    BAIL_ON_INVALID_POINTER(ppszTaskId);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_CREATE;
    in.data = &createArgs;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_CREATE_SUCCESS:

            dwError = LwAllocateString((PSTR)out.data, &pszTaskId);
            BAIL_ON_LW_TASK_ERROR(dwError);

            break;

        case LW_TASK_CREATE_FAILED:

            dwError = ((PLW_TASK_STATUS_REPLY) out.data)->dwError;

            break;

        default:

            dwError = LwErrnoToWin32Error(EINVAL);

            break;
    }
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppszTaskId = pszTaskId;

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (ppszTaskId)
    {
        *ppszTaskId = NULL;
    }

    LW_SAFE_FREE_MEMORY(pszTaskId);

    goto cleanup;
}

DWORD
LwTaskStart(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PCSTR                      pszTaskId,
    PLW_TASK_ARG               pArgArray,
    DWORD                      dwNumArgs
    )
{
    DWORD dwError = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;
    LW_TASK_IPC_START_ARGS startArgs =
    {
        .pszTaskId = pszTaskId,
        .pArgArray = pArgArray,
        .dwNumArgs = dwNumArgs
    };

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_STRING(pszTaskId);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_START;
    in.data = &startArgs;

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
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_STRING(pszTaskId);
    BAIL_ON_INVALID_POINTER(pStatus);

    dwError = LwTaskContextAcquireCall(pConnection, &pCall);
    BAIL_ON_LW_TASK_ERROR(dwError);

    in.tag = LW_TASK_GET_STATUS;
    in.data = (PVOID)pszTaskId;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (out.tag)
    {
        case LW_TASK_GET_STATUS_SUCCESS:

            *pStatus = *((PLW_TASK_STATUS)(out.data));

            break;

        case LW_TASK_GET_STATUS_FAILED:

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


