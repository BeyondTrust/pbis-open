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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntfileapiipc.c
 *
 * Abstract:
 *
 *        NT File API IPC Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

static
VOID
NtpCtxFreeResponse(
    IN LWMsgCall* pCall,
    IN LWMsgTag ResponseType,
    IN PVOID pResponse
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgParams params = LWMSG_PARAMS_INITIALIZER;

    params.tag = ResponseType;
    params.data = pResponse;

    status = lwmsg_call_destroy_params(pCall, &params);
    LW_ASSERT(status == LWMSG_STATUS_SUCCESS);
}

static
void
NtpCtxCallComplete(
    LWMsgCall* pCall,
    LWMsgStatus callStatus,
    void* data
    )
{
    PIO_CLIENT_ASYNC_CANCEL_CONTEXT pCancelContext = data;
    NTSTATUS status = NtIpcLWMsgStatusToNtStatus(callStatus);

    if (status == STATUS_SUCCESS &&
        pCancelContext->out.tag != pCancelContext->responseType)
    {
        status = STATUS_INTERNAL_ERROR;
    }

    status = pCancelContext->pfnComplete(
        status,
        pCancelContext->out.data,
        pCancelContext->pData);
    pCancelContext->pfnUserCallback(pCancelContext->pUserCallbackContext);

    lwmsg_call_destroy_params(pCancelContext->pCall, &pCancelContext->out);
    lwmsg_call_release(pCancelContext->pCall);

    LwNtDereferenceAsyncCancelContext(&pCancelContext);
}

static
NTSTATUS
NtpCtxCallAsync(
    IN PIO_CONTEXT pContext,
    IN LWMsgTag RequestType,
    IN PVOID pRequest,
    IN LWMsgTag ResponseType,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK pControl,
    IN IO_CLIENT_ASYNC_COMPLETE_FUNCTION pfnComplete,
    IN PVOID pData
    )
{
    NTSTATUS status = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    PIO_CLIENT_ASYNC_CANCEL_CONTEXT pCancelContext = NULL;
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pContext, &pCall);
    BAIL_ON_NT_STATUS(status);

    if (pControl)
    {
        status = RTL_ALLOCATE(&pCancelContext, IO_CLIENT_ASYNC_CANCEL_CONTEXT, sizeof(*pCancelContext));
        BAIL_ON_NT_STATUS(status);

        pCancelContext->lRefcount = 2;
        pCancelContext->pCall = pCall;
        pCall = NULL;
        pCancelContext->pfnComplete = pfnComplete;
        pCancelContext->pData = pData;
        pCancelContext->pfnUserCallback = pControl->Callback;
        pCancelContext->pUserCallbackContext = pControl->CallbackContext;

        pCancelContext->responseType = ResponseType;
        pCancelContext->in.tag = RequestType;
        pCancelContext->in.data = pRequest;
        pCancelContext->out.tag = LWMSG_TAG_INVALID;
        pCancelContext->out.data = NULL;

        status = NtIpcLWMsgStatusToNtStatus(lwmsg_call_dispatch(
                                                pCancelContext->pCall,
                                                &pCancelContext->in,
                                                &pCancelContext->out,
                                                NtpCtxCallComplete,
                                                pCancelContext));
        switch (status)
        {
        case STATUS_PENDING:
            pControl->AsyncCancelContext = pCancelContext;
            pCancelContext = NULL;
            break;
        default:
            BAIL_ON_NT_STATUS(status);
        }
    }
    else
    {
        in.tag = RequestType;
        in.data = pRequest;
    
        status = NtIpcLWMsgStatusToNtStatus(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
        if (status == STATUS_SUCCESS && out.tag != ResponseType)
        {
            status = STATUS_INTERNAL_ERROR;
        }
        status = pfnComplete(status, out.data, pData);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    if (status != STATUS_PENDING && pCancelContext)
    {
        if (status == STATUS_SUCCESS && pCancelContext->out.tag
            != pCancelContext->responseType)
        {
            status = STATUS_INTERNAL_ERROR;
        }

        status = pfnComplete(
            status,
            pCancelContext->out.data,
            pCancelContext->pData);

        lwmsg_call_destroy_params(pCancelContext->pCall, &out);
        lwmsg_call_release(pCancelContext->pCall);

        RTL_FREE(&pCancelContext);
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
NtpCtxCall(
    IN LWMsgCall* pCall,
    IN LWMsgTag RequestType,
    IN PVOID pRequest,
    IN LWMsgTag ResponseType,
    OUT PVOID* ppResponse
    )
{
    NTSTATUS status = 0;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = RequestType;
    in.data = pRequest;
    
    status = NtIpcLWMsgStatusToNtStatus(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    if (out.tag != ResponseType)
    {
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
    }
    
    *ppResponse = out.data;
    
cleanup:

    return status;

error:
    
    *ppResponse = NULL;

    NtpCtxFreeResponse(pCall, out.tag, out.data);

    goto cleanup;
}

static
NTSTATUS
NtpCtxGetIoResult(
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse
    )
{
    pIoStatusBlock->Status = pResponse->Status;
    pIoStatusBlock->BytesTransferred = pResponse->BytesTransferred;
    return pIoStatusBlock->Status;
}

static
NTSTATUS
NtpCtxGetBufferResult(
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse
    )
{
    pIoStatusBlock->Status = pResponse->Status;
    pIoStatusBlock->BytesTransferred = pResponse->BytesTransferred;
    assert(pResponse->BytesTransferred <= Length);
    memcpy(Buffer, pResponse->Buffer, SMB_MIN(pResponse->BytesTransferred, Length));
    return pIoStatusBlock->Status;
}

// Need to add a way to cancel operation from outside IRP layer.
// Probably requires something in IO_ASYNC_CONTROL_BLOCK.

//
// The operations below are in categories:
//
// - Core I/O
// - Additional
// - Namespace
// - Advanced
//

//
// Core I/O Operations
//

typedef struct CREATEPIPE_CONTEXT
{
    IO_ASYNC_CONTROL_BLOCK AsyncControl;
    PIO_ASYNC_CONTROL_BLOCK pChain;
    PIO_ECP_LIST pEcpList;
} CREATEPIPE_CONTEXT, *PCREATEPIPE_CONTEXT;

static
VOID
LwNtCreateNamedPipeComplete(
    PVOID pParam
    )
{
    PCREATEPIPE_CONTEXT pContext = (PCREATEPIPE_CONTEXT) pParam;

    pContext->pChain->Callback(pContext->pChain->CallbackContext);

    IoRtlEcpListFree(&pContext->pEcpList);
    RTL_FREE(&pContext);
}

NTSTATUS
LwNtCtxCreateNamedPipeFile(
    IN PIO_CONTEXT pConnection,
    IN LW_PIO_CREDS pSecurityToken,
    OUT PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN OPTIONAL PVOID SecurityDescriptor, // TBD
    IN OPTIONAL PVOID SecurityQualityOfService, // TBD
    IN ACCESS_MASK DesiredAccess,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN FILE_PIPE_TYPE_MASK NamedPipeType,
    IN FILE_PIPE_READ_MODE_MASK ReadMode,
    IN FILE_PIPE_COMPLETION_MODE_MASK CompletionMode,
    IN ULONG MaximumInstances,
    IN ULONG InboundQuota,
    IN ULONG OutboundQuota,
    IN OPTIONAL PLONG64 DefaultTimeout
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_ECP_LIST ecpList = NULL;
    PIO_ECP_NAMED_PIPE pPipeParams = NULL;
    PCREATEPIPE_CONTEXT pContext = NULL;

    status = RTL_ALLOCATE(&pPipeParams, IO_ECP_NAMED_PIPE, sizeof(*pPipeParams));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoRtlEcpListAllocate(&ecpList);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    
    pPipeParams->NamedPipeType = NamedPipeType;
    pPipeParams->ReadMode = ReadMode;
    pPipeParams->CompletionMode = CompletionMode;
    pPipeParams->MaximumInstances = MaximumInstances;
    pPipeParams->InboundQuota = InboundQuota;
    pPipeParams->OutboundQuota = OutboundQuota;
    if (DefaultTimeout)
    {
        pPipeParams->DefaultTimeout = *DefaultTimeout;
        pPipeParams->HaveDefaultTimeout = TRUE;
    }

    status = IoRtlEcpListInsert(ecpList,
                                IO_ECP_TYPE_NAMED_PIPE,
                                pPipeParams,
                                sizeof(*pPipeParams),
                                LwRtlMemoryFree);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    pPipeParams = NULL;

    if (AsyncControlBlock)
    {
        status = RTL_ALLOCATE(&pContext, CREATEPIPE_CONTEXT, sizeof(*pContext));
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        pContext->pEcpList = ecpList;
        pContext->pChain = AsyncControlBlock;
        pContext->AsyncControl.Callback = LwNtCreateNamedPipeComplete;
        pContext->AsyncControl.CallbackContext = pContext;

        AsyncControlBlock = &pContext->AsyncControl;
    }

    status = NtCtxCreateFile(
                    pConnection,
                    pSecurityToken,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileName,
                    SecurityDescriptor,
                    SecurityQualityOfService,
                    DesiredAccess,
                    0,
                    0,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions,
                    NULL,
                    0,
                    ecpList);

    if (pContext)
    {
        pContext->pChain->AsyncCancelContext = pContext->AsyncControl.AsyncCancelContext;
    }

cleanup:

    if (status != STATUS_PENDING)
    {
        if (pContext)
        {
            IoRtlEcpListFree(&pContext->pEcpList);
            RTL_FREE(&pContext);
        }
        else
        {
            IoRtlEcpListFree(&ecpList);
        }

        IoStatusBlock->Status = status;

        RTL_FREE(&pPipeParams);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

typedef struct CREATEFILE_CONTEXT
{
    NT_IPC_MESSAGE_CREATE_FILE request;
    OUT PIO_STATUS_BLOCK IoStatusBlock;
    OUT PIO_FILE_HANDLE FileHandle;
} CREATEFILE_CONTEXT, *PCREATEFILE_CONTEXT;

static
NTSTATUS
LwNtCtxCreateFileComplete(
    NTSTATUS status,
    PVOID pOut,
    PVOID pData
    )
{
    PCREATEFILE_CONTEXT pContext = pData;
    PNT_IPC_MESSAGE_CREATE_FILE_RESULT pResponse = pOut;

    if (status == STATUS_SUCCESS)
    {
        *pContext->FileHandle = pResponse->FileHandle;
        pResponse->FileHandle = NULL;
        pContext->IoStatusBlock->Status = pResponse->Status;
        pContext->IoStatusBlock->CreateResult = pResponse->CreateResult;

        status = pContext->IoStatusBlock->Status;
    }
    else
    {
        pContext->IoStatusBlock->Status = status;
    }

    if (pContext->request.pSecurityToken)
    {
        LwIoDeleteCreds(pContext->request.pSecurityToken);
    }

    if (pContext->request.EcpList)
    {
        RTL_FREE(&pContext->request.EcpList);
    } 

    RTL_FREE(&pContext);

    return status;
}

NTSTATUS
LwNtCtxCreateFile(
    IN PIO_CONTEXT pConnection,
    IN LW_PIO_CREDS pSecurityToken,
    OUT PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN OPTIONAL PVOID SecurityQualityOfService, // TBD
    IN ACCESS_MASK DesiredAccess,
    IN OPTIONAL LONG64 AllocationSize,
    IN FILE_ATTRIBUTES FileAttributes,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN OPTIONAL PVOID EaBuffer, // PFILE_FULL_EA_INFORMATION
    IN ULONG EaLength,
    IN OPTIONAL PIO_ECP_LIST EcpList
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_CREDS pActiveCreds = NULL;
    PCREATEFILE_CONTEXT pCreateContext = NULL;

    if (!pSecurityToken)
    {
        status = LwIoGetActiveCreds(FileName->FileName, &pActiveCreds);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        pSecurityToken = pActiveCreds;
    }

    status = RTL_ALLOCATE(&pCreateContext, CREATEFILE_CONTEXT, sizeof(*pCreateContext));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCreateContext->IoStatusBlock = IoStatusBlock;
    pCreateContext->FileHandle = FileHandle;

    status = LwIoResolveCreds(pSecurityToken, &pCreateContext->request.pSecurityToken);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCreateContext->request.FileName = *FileName;
    pCreateContext->request.DesiredAccess = DesiredAccess;
    pCreateContext->request.AllocationSize = AllocationSize;
    pCreateContext->request.FileAttributes = FileAttributes;
    pCreateContext->request.ShareAccess = ShareAccess;
    pCreateContext->request.CreateDisposition = CreateDisposition;
    pCreateContext->request.CreateOptions = CreateOptions;
    pCreateContext->request.EaBuffer = EaBuffer;
    pCreateContext->request.EaLength = EaLength;
    if (SecurityDescriptor)
    {
        pCreateContext->request.SecurityDescriptor = SecurityDescriptor;
        pCreateContext->request.SecDescLength =
                    RtlLengthSecurityDescriptorRelative(SecurityDescriptor);
    }
    pCreateContext->request.EcpCount = IoRtlEcpListGetCount(EcpList);
    if (pCreateContext->request.EcpCount)
    {
        PCSTR pszType = NULL;
        ULONG ecpIndex = 0;

        status = RTL_ALLOCATE(
            &pCreateContext->request.EcpList,
            NT_IPC_HELPER_ECP,
            sizeof(*pCreateContext->request.EcpList) * pCreateContext->request.EcpCount);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        while (ecpIndex < pCreateContext->request.EcpCount)
        {
            status = IoRtlEcpListGetNext(
                            EcpList,
                            pszType,
                            &pCreateContext->request.EcpList[ecpIndex].pszType,
                            &pCreateContext->request.EcpList[ecpIndex].pData,
                            &pCreateContext->request.EcpList[ecpIndex].Size);
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);

            pszType = pCreateContext->request.EcpList[ecpIndex].pszType;
            ecpIndex++;
        }

        assert(ecpIndex == pCreateContext->request.EcpCount);
        status = STATUS_SUCCESS;
    }

    status = NtpCtxCallAsync(
        pConnection,
        NT_IPC_MESSAGE_TYPE_CREATE_FILE,
        &pCreateContext->request,
        NT_IPC_MESSAGE_TYPE_CREATE_FILE_RESULT,
        AsyncControlBlock,
        LwNtCtxCreateFileComplete,
        pCreateContext);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pActiveCreds)
    {
        LwIoDeleteCreds(pActiveCreds);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    
    return status;
}

NTSTATUS
LwNtCtxCloseFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_CLOSE_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_CLOSE_FILE_RESULT;
    NT_IPC_MESSAGE_GENERIC_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    // In case we add IO_STATUS_BLOCK to close later, which we may want/need.
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    request.FileHandle = FileHandle;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    assert(0 == ioStatusBlock.BytesTransferred);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    /* Release file handle regardless of result */
    lwmsg_session_release_handle(pConnection->pSession, FileHandle);

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxReadFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_READ_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_READ_FILE_RESULT;
    NT_IPC_MESSAGE_READ_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.ByteOffset = ByteOffset;
    request.Key = Key;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, Buffer, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxWriteFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_WRITE_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_WRITE_FILE_RESULT;
    NT_IPC_MESSAGE_WRITE_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.Buffer = Buffer;
    request.Length = Length;
    request.ByteOffset = ByteOffset;
    request.Key = Key;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    assert(ioStatusBlock.BytesTransferred <= Length);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS 
LwNtCtxDeviceIoControlFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE_RESULT;
    NT_IPC_MESSAGE_GENERIC_CONTROL_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.ControlCode = IoControlCode;
    request.InputBuffer = InputBuffer;
    request.InputBufferLength = InputBufferLength;
    request.OutputBufferLength = OutputBufferLength;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, OutputBuffer, OutputBufferLength, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

typedef struct FSCONTROL_CONTEXT
{
    NT_IPC_MESSAGE_GENERIC_CONTROL_FILE request;
    OUT PIO_STATUS_BLOCK IoStatusBlock;
    OUT PVOID OutputBuffer;
    IN ULONG OutputBufferLength;
} FSCONTROL_CONTEXT, *PFSCONTROL_CONTEXT;

static
NTSTATUS
LwNtCtxFsControlFileComplete(
    NTSTATUS status,
    PVOID pOut,
    PVOID pData
    )
{
    PFSCONTROL_CONTEXT pContext = pData;

    if (status == STATUS_SUCCESS)
    {
        NtpCtxGetBufferResult(
            pContext->IoStatusBlock,
            pContext->OutputBuffer,
            pContext->OutputBufferLength,
            pOut);
        status = pContext->IoStatusBlock->Status;
    }
    else
    {
        pContext->IoStatusBlock->Status = status;
    }

    RTL_FREE(&pContext);

    return status;
}

NTSTATUS
LwNtCtxFsControlFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PFSCONTROL_CONTEXT pControlContext = NULL;

    status = RTL_ALLOCATE(&pControlContext, FSCONTROL_CONTEXT, sizeof(*pControlContext));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pControlContext->request.FileHandle = FileHandle;
    pControlContext->request.ControlCode = FsControlCode;
    pControlContext->request.InputBuffer = InputBuffer;
    pControlContext->request.InputBufferLength = InputBufferLength;
    pControlContext->request.OutputBufferLength = OutputBufferLength;
    pControlContext->IoStatusBlock = IoStatusBlock;
    pControlContext->OutputBuffer = OutputBuffer;
    pControlContext->OutputBufferLength = OutputBufferLength;

    status = NtpCtxCallAsync(
        pConnection,
        NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE,
        &pControlContext->request,
        NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE_RESULT,
        AsyncControlBlock,
        LwNtCtxFsControlFileComplete,
        pControlContext);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    LOG_LEAVE_IF_STATUS_EE(status, EE);

    return status;
}

NTSTATUS
LwNtCtxFlushBuffersFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE_RESULT;
    NT_IPC_MESSAGE_GENERIC_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxQueryInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE_RESULT;
    NT_IPC_MESSAGE_QUERY_INFORMATION_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.FileInformationClass = FileInformationClass;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, FileInformation, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS 
LwNtCtxSetInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE_RESULT;
    NT_IPC_MESSAGE_SET_INFORMATION_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.FileInformation = FileInformation;
    request.Length = Length;
    request.FileInformationClass = FileInformationClass;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

//
// Additional Operations
//

#if 0
NTSTATUS
LwNtCtxQueryFullAttributesFile(
    IN PIO_CONTEXT pConnection,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );
#endif

NTSTATUS 
LwNtCtxQueryDirectoryFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PIO_MATCH_FILE_SPEC FileSpec,
    IN BOOLEAN RestartScan
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE_RESULT;
    NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.FileInformationClass = FileInformationClass;
    request.ReturnSingleEntry = ReturnSingleEntry;
    request.FileSpec = FileSpec;
    request.RestartScan = RestartScan;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, FileInformation, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS 
LwNtCtxReadDirectoryChangeFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WatchTree,
    IN FILE_NOTIFY_CHANGE NotifyFilter,
    IN OPTIONAL PULONG MaxBufferSize
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_READ_DIRECTORY_CHANGE_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_READ_DIRECTORY_CHANGE_FILE_RESULT;
    NT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.WatchTree = WatchTree;
    request.NotifyFilter = NotifyFilter;
    request.MaxBufferSize = MaxBufferSize;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, Buffer, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxQueryVolumeInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_QUERY_VOLUME_INFORMATION_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_QUERY_VOLUME_INFORMATION_FILE_RESULT;
    NT_IPC_MESSAGE_QUERY_VOLUME_INFORMATION_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.FsInformationClass = FsInformationClass;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, FsInformation, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxSetVolumeInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS 
LwNtCtxLockFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_LOCK_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_LOCK_FILE_RESULT;    
    NT_IPC_MESSAGE_LOCK_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.ByteOffset = ByteOffset,
    request.Length = Length;
    request.Key = Key;
    request.FailImmediately = FailImmediately;
    request.ExclusiveLock = ExclusiveLock;    

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}


NTSTATUS 
LwNtCtxUnlockFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_UNLOCK_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_UNLOCK_FILE_RESULT;
    NT_IPC_MESSAGE_UNLOCK_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = FileHandle;
    request.ByteOffset = ByteOffset,
    request.Length = Length;
    request.Key = Key;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;

}

//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

#if 0
NTSTATUS
LwNtCtxRemoveDirectoryFile(
    IN PIO_CONTEXT pConnection,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
LwNtCtxDeleteFile(
    IN PIO_CONTEXT pConnection,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
LwNtCtxLinkFile(
    IN PIO_CONTEXT pConnection,
    IN PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    );

NTSTATUS
LwNtCtxRenameFile(
    IN PIO_CONTEXT pConnection,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FromName,
    IN PIO_FILE_NAME ToName
    );
#endif

//
// Advanced Operations
//

NTSTATUS
LwNtCtxQueryQuotaInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PVOID SidList,
    IN ULONG SidListLength,
    IN OPTIONAL PSID StartSid,
    IN BOOLEAN RestartScan
    );

NTSTATUS
LwNtCtxSetQuotaInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
LwNtCtxQuerySecurityFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE Handle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_QUERY_SECURITY_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_QUERY_SECURITY_FILE_RESULT;
    NT_IPC_MESSAGE_QUERY_SECURITY_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = Handle;
    request.SecurityInformation = SecurityInformation;
    request.Length = Length;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, SecurityDescriptor, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxSetSecurityFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE Handle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag requestType = NT_IPC_MESSAGE_TYPE_SET_SECURITY_FILE;
    const LWMsgTag responseType = NT_IPC_MESSAGE_TYPE_SET_SECURITY_FILE_RESULT;
    NT_IPC_MESSAGE_SET_SECURITY_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    LWMsgCall* pCall = NULL;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (AsyncControlBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    request.FileHandle = Handle;
    request.SecurityInformation = SecurityInformation;
    request.SecurityDescriptor = SecurityDescriptor;
    request.Length = Length;

    status = NtpCtxCall(pCall,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    if (pCall)
    {
        NtpCtxFreeResponse(pCall, responseType, pResponse);
        lwmsg_call_release(pCall);
    }

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

VOID
LwNtCancelAsyncCancelContext(
    LW_IN PIO_ASYNC_CANCEL_CONTEXT AsyncCancelContext
    )
{
    if (AsyncCancelContext)
    {
        lwmsg_call_cancel(AsyncCancelContext->pCall);
    }
}

VOID
LwNtDereferenceAsyncCancelContext(
    LW_IN LW_OUT PIO_ASYNC_CANCEL_CONTEXT* ppAsyncCancelContext
    )
{
    if (LwInterlockedDecrement(&(*ppAsyncCancelContext)->lRefcount) == 0)
    {
        RTL_FREE(ppAsyncCancelContext);
    }
}

// TODO: QueryEaFile and SetEaFile.

