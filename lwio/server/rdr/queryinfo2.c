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
 * Module Name:
 *
 *        queryinfo2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 query info
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveQueryInfoFile2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS infoClass,
    ULONG ulInfoLength
    );

static
BOOLEAN
RdrQueryInfoFile2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
VOID
RdrCancelQueryInfo2(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQueryInformation2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB2 pFile = NULL;
    ULONG ulInfoLength = 0;

    pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.QuerySetInformation.FileInformationClass)
    {
    case FileBasicInformation:
        ulInfoLength = sizeof(FILE_BASIC_INFORMATION);
        break;
    case FileStandardInformation:
        ulInfoLength = sizeof(FILE_STANDARD_INFORMATION);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    if (pFile->pTree->pSession->pSocket->ulMaxTransactSize < ulInfoLength)
    {
        /*
         * Uh-oh.  We are requesting an info structure larger than
         * the maximum allowed by the connection.
         */
        status = STATUS_BUFFER_OVERFLOW;
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelQueryInfo2, pContext);

    pContext->Continue = RdrQueryInfoFile2Complete;

    status = RdrTransceiveQueryInfoFile2(
        pContext,
        pFile,
        pIrp->Args.QuerySetInformation.FileInformationClass,
        ulInfoLength);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveQueryInfoFile2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    FILE_INFORMATION_CLASS infoClass,
    ULONG ulInfoLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_QUERY_INFO_SIZE(0));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_GETINFO,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeQueryInfoRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        SMB2_INFO_TYPE_FILE,
        (UCHAR) infoClass,
        ulInfoLength,
        0, /* additional info */
        0, /* flags */
        &pFile->Fid,
        NULL); /* input buffer length */
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrQueryInfoFile2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PBYTE pOutput = NULL;
    ULONG ulOutputSize = 0;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMB2Header->error;
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2DecodeQueryInfoResponse(
        pPacket,
        &pOutput,
        &ulOutputSize);
    BAIL_ON_NT_STATUS(status);

    status = RdrUnmarshalQueryFileInfoReply(
        pContext->pIrp->Args.QuerySetInformation.FileInformationClass,
        pOutput,
        ulOutputSize,
        pContext->pIrp->Args.QuerySetInformation.FileInformation,
        pContext->pIrp->Args.QuerySetInformation.Length,
        &pContext->pIrp->IoStatusBlock.BytesTransferred);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pPacket);

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}
