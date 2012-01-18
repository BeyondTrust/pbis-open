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
