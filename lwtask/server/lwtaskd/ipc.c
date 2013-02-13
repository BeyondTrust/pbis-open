
#include "includes.h"

static
LWMsgStatus
LwTaskDaemonIpcSetLogInfo(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcGetLogInfo(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcGetPid(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcTaskStart(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcTaskStop(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcTaskDelete(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcGetTypes(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcGetStatus(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcCreate(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcGetSchema(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgStatus
LwTaskDaemonIpcEnum(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

static
LWMsgDispatchSpec
gLwTaskDaemonIpcDispatchSpec[] =
{
    LWMSG_DISPATCH_BLOCK(LW_TASK_SET_LOG_INFO, LwTaskDaemonIpcSetLogInfo),
    LWMSG_DISPATCH_BLOCK(LW_TASK_GET_LOG_INFO, LwTaskDaemonIpcGetLogInfo),
    LWMSG_DISPATCH_BLOCK(LW_TASK_GET_PID,      LwTaskDaemonIpcGetPid),
    LWMSG_DISPATCH_BLOCK(LW_TASK_START,        LwTaskDaemonIpcTaskStart),
    LWMSG_DISPATCH_BLOCK(LW_TASK_STOP,         LwTaskDaemonIpcTaskStop),
    LWMSG_DISPATCH_BLOCK(LW_TASK_DELETE,       LwTaskDaemonIpcTaskDelete),
    LWMSG_DISPATCH_BLOCK(LW_TASK_GET_TYPES,    LwTaskDaemonIpcGetTypes),
    LWMSG_DISPATCH_BLOCK(LW_TASK_GET_STATUS,   LwTaskDaemonIpcGetStatus),
    LWMSG_DISPATCH_BLOCK(LW_TASK_CREATE,       LwTaskDaemonIpcCreate),
    LWMSG_DISPATCH_BLOCK(LW_TASK_GET_SCHEMA,   LwTaskDaemonIpcGetSchema),
    LWMSG_DISPATCH_BLOCK(LW_TASK_ENUM,         LwTaskDaemonIpcEnum),
    LWMSG_DISPATCH_END
};

static
LWMsgStatus
LwTaskDaemonIpcGetPid(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    );

DWORD
LwTaskDaemonIpcAddDispatch(
    LWMsgServer* pServer /* IN OUT */
    )
{
    return MAP_LWMSG_ERROR(lwmsg_server_add_dispatch_spec(
                                    pServer,
                                    gLwTaskDaemonIpcDispatchSpec));
}

static
LWMsgStatus
LwTaskDaemonIpcSetLogInfo(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskLogSetInfo_r((PLW_TASK_LOG_INFO)pIn->data);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_SET_LOG_INFO_FAILED;
        pOut->data = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_SET_LOG_INFO_SUCCESS;
    pOut->data = pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcGetLogInfo(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;
    PLW_TASK_LOG_INFO pLogInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskLogGetInfo_r(&pLogInfo);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_GET_LOG_INFO_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_GET_LOG_INFO_SUCCESS;
    pOut->data = pLogInfo;

cleanup:

    LW_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcGetPid(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    pid_t* pPid = NULL;

    dwError = LwAllocateMemory(sizeof(*pPid), OUT_PPVOID(&pPid));
    BAIL_ON_LW_TASK_ERROR(dwError);

    *pPid = getpid();

    pOut->tag = LW_TASK_GET_PID_SUCCESS;
    pOut->data = pPid;
    pPid = NULL;

cleanup:

    LW_SAFE_FREE_MEMORY(pPid);

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcTaskStart(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_IPC_START_ARGS pRequest = NULL;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pRequest = (PLW_TASK_IPC_START_ARGS)pIn->data;

    dwError = LwTaskSrvStart(
                    pRequest->pszTaskId,
                    pRequest->pArgArray,
                    pRequest->dwNumArgs);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_START_FAILED;
        pOut->data = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_START_SUCCESS;
    pOut->data = pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcTaskStop(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvStop((PCSTR)pIn->data);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_STOP_FAILED;
        pOut->data = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_STOP_SUCCESS;
    pOut->data = pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcTaskDelete(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvDelete((PCSTR)pIn->data);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_DELETE_FAILED;
        pOut->data = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_DELETE_SUCCESS;
    pOut->data = pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcGetTypes(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;
    PLW_TASK_IPC_GET_TYPES pResult = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_IPC_GET_TYPES),
                    (PVOID*)&pResult);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvGetTypes(
                    &pResult->pdwTaskTypeArray,
                    &pResult->dwNumTaskTypes);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_GET_TYPES_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_GET_TYPES_SUCCESS;
    pOut->data = pResult;
    pResult = NULL;

cleanup:

    LW_SAFE_FREE_MEMORY(pStatusResponse);

    if (pResult)
    {
        LwFreeMemory(pResult);
    }

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcGetStatus(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;
    PLW_TASK_STATUS pResult = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(LW_TASK_STATUS), (PVOID*)&pResult);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvGetStatus((PCSTR)pIn->data, pResult);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_GET_STATUS_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_GET_STATUS_SUCCESS;
    pOut->data = pResult;
    pResult = NULL;

cleanup:

    LW_SAFE_FREE_MEMORY(pStatusResponse);

    if (pResult)
    {
        LwFreeMemory(pResult);
    }

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcCreate(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;
    PLW_TASK_IPC_CREATE_ARGS pCreateArgs = NULL;
    PSTR  pszTaskId = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pCreateArgs = (PLW_TASK_IPC_CREATE_ARGS)pIn->data;

    dwError = LwTaskSrvCreate(
                    pCreateArgs->taskType,
                    pCreateArgs->pArgArray,
                    pCreateArgs->dwNumArgs,
                    &pszTaskId);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_CREATE_FAILED;
        pOut->data = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_CREATE_SUCCESS;
    pOut->data = pszTaskId;
    pszTaskId = NULL;

cleanup:

    LW_SAFE_FREE_MEMORY(pStatusResponse);

    LW_SAFE_FREE_MEMORY(pszTaskId);

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcGetSchema(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;
    PLW_TASK_IPC_SCHEMA pResult = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(LW_TASK_IPC_SCHEMA), (PVOID*)&pResult);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvGetSchema(
                    ((PLW_TASK_IPC_GET_SCHEMA)pIn->data)->taskType,
                    &pResult->pArgInfoArray,
                    &pResult->dwNumArgInfos);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_GET_SCHEMA_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_GET_SCHEMA_SUCCESS;
    pOut->data = pResult;
    pResult = NULL;

cleanup:

    LW_SAFE_FREE_MEMORY(pStatusResponse);

    if (pResult)
    {
        LwFreeMemory(pResult);
    }

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwTaskDaemonIpcEnum(
    LWMsgCall*         pCall,  /* IN     */
    const LWMsgParams* pIn,    /* IN     */
    LWMsgParams*       pOut,   /*    OUT */
    void*              pData   /* IN     */
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PLW_TASK_STATUS_REPLY pStatusResponse = NULL;
    PLW_TASK_IPC_ENUM_REQUEST pRequest = NULL;
    PLW_TASK_IPC_ENUM_RESPONSE pResult = NULL;

    BAIL_ON_INVALID_POINTER(pIn->data);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_IPC_ENUM_RESPONSE),
                    (PVOID*)&pResult);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pRequest = (PLW_TASK_IPC_ENUM_REQUEST)pIn->data;
    pResult->dwResume = pRequest->dwResume;

    dwError = LwTaskSrvEnum(
                    pRequest->taskType,
                    &pResult->dwTotalTaskInfos,
                    &pResult->dwNumTaskInfos,
                    &pResult->pTaskInfoArray,
                    &pResult->dwResume);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pOut->tag = LW_TASK_ENUM_FAILED;
        pOut->data = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pOut->tag = LW_TASK_ENUM_SUCCESS;
    pOut->data = pResult;
    pResult = NULL;

cleanup:

    LW_SAFE_FREE_MEMORY(pStatusResponse);

    if (pResult)
    {
        LwFreeMemory(pResult);
    }

    return status;

error:

    goto cleanup;
}

