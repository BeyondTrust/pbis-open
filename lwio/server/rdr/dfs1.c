/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "rdr.h"

static
BOOLEAN
RdrDfsChaseReferral1IpcConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrQueryDfsReferral1Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveQueryDfsReferral1(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    PCWSTR pwszPath
    );

NTSTATUS
RdrDfsChaseReferral1(
    PRDR_SESSION pSession,
    PIO_CREDS pCreds,
    PCWSTR pwszPath,
    PRDR_OP_CONTEXT pContinue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszShare = NULL;
    static WCHAR wszEmpty[] = {'\0'};

    status = RdrCreateContext(NULL, &pContext);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pContext->State.DfsGetReferral.pwszPath, pwszPath);
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrDfsChaseReferral1IpcConnectComplete;
    pContext->State.DfsGetReferral.pContinue = pContinue;

    if (!pSession)
    {
        pContext->State.DfsGetReferral.AnonCreds.type = IO_CREDS_TYPE_PLAIN;
        pContext->State.DfsGetReferral.AnonCreds.payload.plain.pwszDomain = wszEmpty;
        pContext->State.DfsGetReferral.AnonCreds.payload.plain.pwszPassword = wszEmpty;
        pContext->State.DfsGetReferral.AnonCreds.payload.plain.pwszUsername = wszEmpty;

        status = RdrConvertPath(
            pwszPath,
            &pwszServer,
            NULL,
            NULL);
        BAIL_ON_NT_STATUS(status);

        status = LwRtlWC16StringAllocatePrintfW(&pwszShare, L"\\\\%ws\\IPC$", pwszServer);
        BAIL_ON_NT_STATUS(status);

        status = RdrTreeConnect(
            pwszServer,
            pwszShare,
            &pContext->State.DfsGetReferral.AnonCreds,
            getpid(),
            FALSE,
            pContext);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = LwRtlWC16StringAllocatePrintfW(
            &pwszShare,
            L"\\\\%ws\\IPC$",
            pSession->pSocket->pwszCanonicalName);
        BAIL_ON_NT_STATUS(status);

        status = RdrTreeConnect(
            pSession->pSocket->pwszCanonicalName,
            pwszShare,
            pCreds,
            pSession->key.uid,
            FALSE,
            pContext);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    RTL_FREE(&pwszServer);
    RTL_FREE(&pwszShare);

    if (status != STATUS_PENDING && pContext)
    {
        RTL_FREE(&pContext->State.DfsGetReferral.pwszPath);
        RdrFreeContext(pContext);
    }

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrDfsChaseReferral1IpcConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    /* FIXME: what if we get an SMB2 connection when chasing an SMB1 referral? */
    PRDR_TREE pTree = (PRDR_TREE) pParam;

    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrQueryDfsReferral1Complete;

    status = RdrTransceiveQueryDfsReferral1(
        pContext,
        pTree,
        pContext->State.DfsGetReferral.pwszPath);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (pTree)
    {
        /* FIXME: probably better to do this after receiving response */
        RdrTreeRelease(pTree);
    }

    if (status != STATUS_PENDING && pContext)
    {
        RdrContinueContext(pContext->State.DfsGetReferral.pContinue, status, NULL);
        RTL_FREE(&pContext->State.DfsGetReferral.pwszPath);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}

static
BOOLEAN
RdrQueryDfsReferral1Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pResponsePacket = pParam;
    PBYTE pCursor = NULL;
    PBYTE pReplyData = NULL;
    USHORT usReplyDataCount = 0;
    ULONG ulRemainingSpace = 0;

    BAIL_ON_NT_STATUS(status);

    status = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    pCursor = pResponsePacket->pParams;
    ulRemainingSpace = pResponsePacket->pNetBIOSHeader->len -
        ((PBYTE)pResponsePacket->pParams - (PBYTE)pResponsePacket->pSMBHeader);

    status = WireUnmarshalTrans2ReplySetup(
        pResponsePacket->pSMBHeader,
        &pCursor,
        &ulRemainingSpace,
        NULL, /* ppResponseHeader */
        NULL, /* pusTotalParameterCount */
        NULL, /* pusTotalDataCount */
        NULL, /* ppusSetupWords */
        NULL, /* pusSetupWordCount */
        NULL, /* pusByteCount */
        NULL, /* pParameterBlock */
        NULL, /* pusParameterCount */
        &pReplyData,
        &usReplyDataCount);
    BAIL_ON_NT_STATUS(status);

    status = RdrDfsRegisterNamespace(
        pContext->State.DfsGetReferral.pwszPath,
        (PDFS_RESPONSE_HEADER) pReplyData,
        usReplyDataCount);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pResponsePacket);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.DfsGetReferral.pContinue, status, NULL);
        RTL_FREE(&pContext->State.DfsGetReferral.pwszPath);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveQueryDfsReferral1(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree,
    PCWSTR pwszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_GET_DFS_REFERRAL;
    DFS_REQUEST_HEADER request = {0};
    PBYTE pRequestParameters = NULL;
    PBYTE pCursor = NULL;
    PBYTE pByteCount = NULL;
    ULONG ulRemainingSpace = 0;
    PBYTE pPath = NULL;

    status = RdrAllocateContextPacket(pContext, 1024*64);
    BAIL_ON_NT_STATUS(status);

    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_TRANSACTION2,
        0,
        0,
        pTree->tid,
        gRdrRuntime.SysPid,
        pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pCursor = pContext->Packet.pParams;
    ulRemainingSpace = pContext->Packet.bufferLen - (pCursor - pContext->Packet.pRawBuffer);

    status = WireMarshalTrans2RequestSetup(
        pContext->Packet.pSMBHeader,
        &pCursor,
        &ulRemainingSpace,
        &usSetup,
        1,
        &pHeader,
        &pByteCount);
    BAIL_ON_NT_STATUS(status);

    /* Align to WCHAR before we write parameters */
    status = Align((PBYTE) pContext->Packet.pSMBHeader, &pCursor, &ulRemainingSpace, sizeof(WCHAR));
    /* Remember start of the trans2 parameter block */
    pRequestParameters = pCursor;

    request.usMaxReferralLevel = SMB_HTOL16(4);

    status = MarshalData(&pCursor, &ulRemainingSpace, (PBYTE) &request, sizeof(request));
    BAIL_ON_NT_STATUS(status);

    pPath = pCursor;

    status = Advance(&pCursor, &ulRemainingSpace, (LwRtlWC16StringNumChars(pwszPath) + 1) * sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOLWSTR(pPath, pwszPath, LwRtlWC16StringNumChars(pwszPath) + 1);

    /* Update fields in trans request header */
    pHeader->totalParameterCount = SMB_HTOL16(pCursor - pRequestParameters);
    pHeader->totalDataCount      = SMB_HTOL16(0);
    pHeader->maxParameterCount   = SMB_HTOL16(0);
    pHeader->maxDataCount        = SMB_HTOL16(8192); /* FIXME: magic value */
    pHeader->maxSetupCount       = SMB_HTOL8(0);
    pHeader->flags               = SMB_HTOL16(0);
    pHeader->timeout             = SMB_HTOL32(0);
    pHeader->parameterCount      = SMB_HTOL16(pCursor - pRequestParameters);
    pHeader->parameterOffset     = SMB_HTOL16(pRequestParameters - (PBYTE) pContext->Packet.pSMBHeader);
    pHeader->dataCount           = SMB_HTOL16(0);
    pHeader->dataOffset          = SMB_HTOL16(0);
    pHeader->setupCount          = SMB_HTOL8(1);

    /* Update byte count */
    status = MarshalUshort(&pByteCount, NULL, (pCursor - pByteCount) - 2);

    /* Update used length */
    pContext->Packet.bufferUsed += (pCursor - pContext->Packet.pParams);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
