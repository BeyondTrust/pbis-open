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

#include "rdr.h"

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
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszShare = NULL;
    PWSTR pwszFilePath = NULL;

    if (pContext->State.DfsConnect.OrigStatus == STATUS_PATH_NOT_COVERED)
    {
        /* Look up complete canonical path */
        status = RdrConvertUnicodeStringPath(
            pContext->State.DfsConnect.pPath,
            NULL,
            &pwszShare,
            &pwszFilePath);
        BAIL_ON_NT_STATUS(status);

        status = RdrConstructCanonicalPath(
            pwszShare,
            pwszFilePath,
            &pContext->State.DfsConnect.pwszNamespace);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        /* Look up only the share */
        status = RdrConvertUnicodeStringPath(
            pContext->State.DfsConnect.pPath,
            NULL,
            &pContext->State.DfsConnect.pwszNamespace,
            NULL);
        BAIL_ON_NT_STATUS(status);
    }

    pContext->Continue = RdrQueryDfsReferral1Complete;

    status = RdrTransceiveQueryDfsReferral1(
        pContext,
        pTree,
        pContext->State.DfsConnect.pwszNamespace);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pwszShare);
    RTL_FREE(&pwszFilePath);

    if (pTree)
    {
        RdrTreeRelease(pTree);
    }

    return status;

error:

    if (status != STATUS_PENDING)
    {
        RTL_FREE(&pContext->State.DfsConnect.pwszNamespace);
    }

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
    switch (status)
    {
    case STATUS_NO_SUCH_FILE:
    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NOT_FOUND:
        /* Referral failed -- insert negative cache entry */
        pReplyData = NULL;
        usReplyDataCount = 0;
        status = STATUS_SUCCESS;
        break;
    default:
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
    }

    status = RdrDfsRegisterNamespace(
        pContext->State.DfsConnect.pwszNamespace,
        (PDFS_RESPONSE_HEADER) pReplyData,
        usReplyDataCount);
    BAIL_ON_NT_STATUS(status);

    status = RdrDfsConnectAttempt(pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pResponsePacket);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.DfsConnect.pContinue, status, NULL);
        RTL_FREE(&pContext->State.DfsConnect.pwszNamespace);
        RTL_FREE(pContext->State.DfsConnect.ppwszCanonicalPath);
        RTL_FREE(pContext->State.DfsConnect.ppwszFilePath);
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
    pHeader->maxDataCount        = SMB_HTOL16(DFS_MAX_RESPONSE_SIZE); /* FIXME: magic value */
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
