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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        close.c
 *
 * Abstract:
 *
 *        Likewise SMB Redirector File System Driver (RDR)
 *
 *        Close Dispatch Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

static
NTSTATUS
RdrTranscieveDelete(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile
    );

static
NTSTATUS
RdrTranscieveDeleteDirectory(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile
    );

static
BOOLEAN
RdrFinishClose(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
VOID
RdrCancelClose(
    PIRP pIrp,
    PVOID pContext
    )
{
    return;
}

NTSTATUS
RdrClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);
    PCLOSE_REQUEST_HEADER pHeader = NULL;
    PRDR_OP_CONTEXT pContext = NULL;

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelClose, pContext);

    if (pFile->fid)
    {
        status = RdrAllocateContextPacket(
            pContext,
            1024*64);
        BAIL_ON_NT_STATUS(status);
    
        status = SMBPacketMarshallHeader(
            pContext->Packet.pRawBuffer,
            pContext->Packet.bufferLen,
            COM_CLOSE,
            0,
            0,
            pFile->pTree->tid,
            gRdrRuntime.SysPid,
            pFile->pTree->pSession->uid,
            0,
            TRUE,
            &pContext->Packet);
        BAIL_ON_NT_STATUS(status);
    
        pContext->Packet.pData = pContext->Packet.pParams + sizeof(CLOSE_REQUEST_HEADER);
        pContext->Packet.bufferUsed += sizeof(CLOSE_REQUEST_HEADER);

        pContext->Packet.pSMBHeader->wordCount = 3;
        
        pHeader = (PCLOSE_REQUEST_HEADER) pContext->Packet.pParams;
        
        pHeader->fid = SMB_HTOL16(pFile->fid);
        pHeader->ulLastWriteTime = SMB_HTOL32(0);
        pHeader->byteCount = SMB_HTOL16(0);
        
        status = SMBPacketMarshallFooter(&pContext->Packet);
        BAIL_ON_NT_STATUS(status);

        pContext->Continue = RdrFinishClose;
    
        status = RdrSocketTransceive(
            pFile->pTree->pSession->pSocket,
            pContext);
        BAIL_ON_NT_STATUS(status);
    }
    else if (pFile->Params.CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        if (pFile->Params.CreateOptions & FILE_DIRECTORY_FILE)
        {
            status = RdrTranscieveDeleteDirectory(pContext, pFile);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            status = RdrTranscieveDelete(pContext, pFile);
            BAIL_ON_NT_STATUS(status);
        }
    }
   
cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        RdrReleaseFile(pFile);
        pIrp->IoStatusBlock.Status = STATUS_SUCCESS;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;
    
error:
    
    goto cleanup;
}

static
BOOLEAN
RdrFinishClose(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PIRP pIrp = pContext->pIrp;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);
    
    RdrFreePacket(pPacket);

    /*
     * We must release the file before completing the IRP.  This guarantees that any tree
     * this file might have been holding open will have its reference count decremented.
     * This satisfies the assumption in RdrShutdown() that all open trees have a reference
     * count of zero.
     */
    RdrReleaseFile(pFile);

    pIrp->IoStatusBlock.Status = STATUS_SUCCESS;
    IoIrpComplete(pIrp);
    RdrFreeContext(pContext);

    return FALSE;
}

void
RdrReleaseFile(
    PRDR_CCB pFile
    )
{
    if (pFile->pTree)
    {
        RdrTreeRelease(pFile->pTree);
    }
    
    if (pFile->bMutexInitialized)
    {
        pthread_mutex_destroy(&pFile->mutex);
    }
    
    RTL_FREE(&pFile->pwszPath);
    RTL_FREE(&pFile->pwszCanonicalPath);
    RTL_FREE(&pFile->find.pBuffer);
    
    LwIoFreeMemory(pFile);
}

static
NTSTATUS
RdrTranscieveDelete(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSMB_DELETE_REQUEST_HEADER pDeleteHeader = {0};
    PBYTE pCursor = NULL;
    ULONG ulRemainingSpace = 0;
    PBYTE pFileName = NULL;
    PWSTR pwszPath = RDR_CCB_PATH(pFile);

    status = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(status);
    
    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_DELETE,
        0,
        0,
        pFile->pTree->tid,
        gRdrRuntime.SysPid,
        pFile->pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);
    
    if (RDR_CCB_IS_DFS(pFile))
    {
        pContext->Packet.pSMBHeader->flags2 |= FLAG2_DFS;
    }

    pContext->Packet.pSMBHeader->wordCount = 1;

    pCursor = pContext->Packet.pParams;
    ulRemainingSpace = pContext->Packet.bufferLen - (pCursor - pContext->Packet.pRawBuffer);

    pDeleteHeader = (PSMB_DELETE_REQUEST_HEADER) pCursor;
    
    status = Advance(&pCursor, &ulRemainingSpace, sizeof(*pDeleteHeader));
    BAIL_ON_NT_STATUS(status);

    /* Buffer type */
    status = MarshalByte(&pCursor, &ulRemainingSpace, 0x4);
    BAIL_ON_NT_STATUS(status);

    status = Align((PBYTE) pContext->Packet.pSMBHeader, &pCursor, &ulRemainingSpace, sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    pFileName = pCursor;

    status = Advance(&pCursor, &ulRemainingSpace, (LwRtlWC16StringNumChars(pwszPath) + 1) * sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOLWSTR(
        pFileName,
        pwszPath,
        LwRtlWC16StringNumChars(pwszPath) + 1);
        
    pDeleteHeader->usSearchAttributes = 0;
    pDeleteHeader->ByteCount = SMB_HTOL16((pCursor - (PBYTE) pDeleteHeader) - sizeof(*pDeleteHeader));
        
    pContext->Packet.bufferUsed += (pCursor - pContext->Packet.pParams);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrFinishClose;
    
    status = RdrSocketTransceive(
        pFile->pTree->pSession->pSocket,
        pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTranscieveDeleteDirectory(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDELETE_DIRECTORY_REQUEST_HEADER pDeleteHeader = {0};
    PBYTE pCursor = NULL;
    ULONG ulRemainingSpace = 0;
    PBYTE pFileName = NULL;

    status = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(status);
    
    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_DELETE_DIRECTORY,
        0,
        0,
        pFile->pTree->tid,
        gRdrRuntime.SysPid,
        pFile->pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);
    
    pContext->Packet.pSMBHeader->wordCount = 0;

    pCursor = pContext->Packet.pParams;
    ulRemainingSpace = pContext->Packet.bufferLen - (pCursor - pContext->Packet.pRawBuffer);

    pDeleteHeader = (PDELETE_DIRECTORY_REQUEST_HEADER) pCursor;
    
    status = Advance(&pCursor, &ulRemainingSpace, sizeof(*pDeleteHeader));
    BAIL_ON_NT_STATUS(status);

    pDeleteHeader->ucBufferFormat = 0x4;
    
    status = Align((PBYTE) pContext->Packet.pSMBHeader, &pCursor, &ulRemainingSpace, sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    pFileName = pCursor;

    status = Advance(&pCursor, &ulRemainingSpace, (LwRtlWC16StringNumChars(pFile->pwszPath) + 1) * sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOLWSTR(
        pFileName,
        pFile->pwszPath,
        LwRtlWC16StringNumChars(pFile->pwszPath) + 1);
        
    pDeleteHeader->usByteCount = SMB_HTOL16((pCursor - (PBYTE) pDeleteHeader) - sizeof(USHORT));
        
    pContext->Packet.bufferUsed += (pCursor - pContext->Packet.pParams);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrFinishClose;
    
    status = RdrSocketTransceive(
        pFile->pTree->pSession->pSocket,
        pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
