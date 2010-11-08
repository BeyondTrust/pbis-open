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
RdrDfsChaseReferral2IpcConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrQueryDfsReferral2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveQueryDfsReferral2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree,
    PCWSTR pwszPath
    );

NTSTATUS
RdrDfsChaseReferral2(
    PRDR_SESSION2 pSession,
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

    pContext->Continue = RdrDfsChaseReferral2IpcConnectComplete;
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
RdrDfsChaseReferral2IpcConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_TREE2 pTree = (PRDR_TREE2) pParam;

    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrQueryDfsReferral2Complete;

    status = RdrTransceiveQueryDfsReferral2(
        pContext,
        pTree,
        pContext->State.DfsGetReferral.pwszPath);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (pTree)
    {
        /* FIXME: probably better to do this after receiving response */
        RdrTree2Release(pTree);
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
RdrQueryDfsReferral2Complete(
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
    switch (status)
    {
    case STATUS_NO_SUCH_FILE:
        /* Referral failed -- insert negative cache entry */
        pOutput = NULL;
        ulOutputSize = 0;
        status = STATUS_SUCCESS;
        break;
    default:
        BAIL_ON_NT_STATUS(status);

        status = RdrSmb2DecodeIoctlResponse(pPacket, &pOutput, &ulOutputSize);
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrDfsRegisterNamespace(
        pContext->State.DfsGetReferral.pwszPath,
        (PDFS_RESPONSE_HEADER) pOutput,
        (USHORT) ulOutputSize);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pPacket);

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
RdrTransceiveQueryDfsReferral2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree,
    PCWSTR pwszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;
    PULONG pulInputSize = NULL;
    PDFS_REQUEST_HEADER pRequest = NULL;
    ULONG ulDataSize = sizeof(*pRequest) + (LwRtlWC16StringNumChars(pwszPath) + 1) * sizeof(WCHAR);

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_IOCTL_SIZE(ulDataSize));
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_IOCTL,
        0, /* flags */
        gRdrRuntime.SysPid,
        pTree->ulTid, /* tid */
        pTree->pSession->ullSessionId,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeIoctlRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining,
        IO_FSCTL_GET_DFS_REFERRALS,
        NULL,
        0,
        4096,
        TRUE,
        &pulInputSize);
    BAIL_ON_NT_STATUS(status);

    pRequest = (PDFS_REQUEST_HEADER) pCursor;
    status = Advance(&pCursor, &ulRemaining, sizeof(*pRequest));
    BAIL_ON_NT_STATUS(status);

    pRequest->usMaxReferralLevel = SMB_HTOL16(4);

    status = MarshalPwstr(&pCursor, &ulRemaining, pwszPath, -1);
    BAIL_ON_NT_STATUS(status);

    *pulInputSize = SMB_HTOL32(ulDataSize);

    status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
