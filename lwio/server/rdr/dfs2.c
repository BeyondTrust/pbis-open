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
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree
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

    pContext->Continue = RdrQueryDfsReferral2Complete;

    status = RdrTransceiveQueryDfsReferral2(
        pContext,
        pTree,
        pContext->State.DfsConnect.pwszNamespace);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pwszShare);
    RTL_FREE(&pwszFilePath);

    if (pTree)
    {
        RdrTree2Release(pTree);
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
    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NOT_FOUND:
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
        pContext->State.DfsConnect.pwszNamespace,
        (PDFS_RESPONSE_HEADER) pOutput,
        (USHORT) ulOutputSize);
    BAIL_ON_NT_STATUS(status);
    status = RdrDfsConnectAttempt(pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pPacket);

    RTL_FREE(&pContext->State.DfsConnect.pwszNamespace);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.DfsConnect.pContinue, status, NULL);
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
