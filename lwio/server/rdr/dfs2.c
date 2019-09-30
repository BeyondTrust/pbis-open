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
