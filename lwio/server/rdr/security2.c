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
 *        security2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 security query/set
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveQuerySecurity2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    SECURITY_INFORMATION SecurityInfo,
    ULONG ulInfoLength
    );

static
BOOLEAN
RdrQuerySecurity2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

NTSTATUS
RdrSetSecurity2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

static
NTSTATUS
RdrTransceiveSetSecurity2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    SECURITY_INFORMATION SecurityInfo,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    ULONG ulInfoLength
    );

static
BOOLEAN
RdrSetSecurity2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
VOID
RdrCancelQuerySecurity2(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQuerySecurity2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB2 pFile = NULL;

    pFile = IoFileGetContext(pIrp->FileHandle);

    if (pFile->pTree->pSession->pSocket->ulMaxTransactSize <
        pIrp->Args.QuerySetSecurity.Length)
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

    IoIrpMarkPending(pIrp, RdrCancelQuerySecurity2, pContext);

    pContext->Continue = RdrQuerySecurity2Complete;

    status = RdrTransceiveQuerySecurity2(
        pContext,
        pFile,
        pIrp->Args.QuerySetSecurity.SecurityInformation,
        pIrp->Args.QuerySetSecurity.Length);
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
RdrTransceiveQuerySecurity2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    SECURITY_INFORMATION SecurityInfo,
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
        SMB2_INFO_TYPE_SECURITY,
        0, /* info class */
        ulInfoLength,
        SecurityInfo,
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
RdrQuerySecurity2Complete(
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

    if (ulOutputSize > pContext->pIrp->Args.QuerySetSecurity.Length)
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(
        pContext->pIrp->Args.QuerySetSecurity.SecurityDescriptor,
        pOutput,
        ulOutputSize);

    pContext->pIrp->IoStatusBlock.BytesTransferred = ulOutputSize;

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

static
VOID
RdrCancelSetSecurity2(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrSetSecurity2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB2 pFile = NULL;

    pFile = IoFileGetContext(pIrp->FileHandle);

    if (pFile->pTree->pSession->pSocket->ulMaxTransactSize <
        pIrp->Args.QuerySetSecurity.Length)
    {
        status = STATUS_BUFFER_OVERFLOW;
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelSetSecurity2, pContext);

    pContext->Continue = RdrSetSecurity2Complete;

    status = RdrTransceiveSetSecurity2(
        pContext,
        pFile,
        pIrp->Args.QuerySetSecurity.SecurityInformation,
        pIrp->Args.QuerySetSecurity.SecurityDescriptor,
        pIrp->Args.QuerySetSecurity.Length);
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
RdrTransceiveSetSecurity2(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB2 pFile,
    SECURITY_INFORMATION SecurityInfo,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    ULONG ulInfoLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;
    PULONG pulLength = 0;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_SET_INFO_SIZE(ulInfoLength));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_SETINFO,
        0, /* flags */
        gRdrRuntime.SysPid,
        pFile->pTree->ulTid, /* tid */
        pFile->pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeSetInfoRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        SMB2_INFO_TYPE_SECURITY,
        0, /* info class */
        SecurityInfo,
        &pFile->Fid,
        &pulLength); /* input buffer length */
    BAIL_ON_NT_STATUS(status);

    status = MarshalData(&pCursor, &ulRemaining, (PBYTE) pSecurityDescriptor, ulInfoLength);
    BAIL_ON_NT_STATUS(status);

    *pulLength = SMB_HTOL32(ulInfoLength);

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
RdrSetSecurity2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMB2Header->error;
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
