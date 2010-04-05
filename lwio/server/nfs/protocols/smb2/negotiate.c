/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        negotiate.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Negotiate
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#include "includes.h"

static
NTSTATUS
NfsMarshalNegotiateResponse_SMB_V2(
    PLWIO_NFS_CONNECTION pConnection,
    PBYTE                pSessionKey,
    ULONG                ulSessionKeyLength,
    PNFS_MESSAGE_SMB_V2  pSmbResponse
    );

NTSTATUS
NfsProcessNegotiate_SMB_V2(
    IN OUT PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION pConnection = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSMB2_NEGOTIATE_REQUEST_HEADER pNegotiateRequestHeader = NULL;// Do not free
    PUSHORT pusDialects      = NULL; // Do not free
    USHORT  iDialect         = 0;

    ntStatus = SMB2UnmarshalNegotiateRequest(
                        pSmbRequest,
                        &pNegotiateRequestHeader,
                        &pusDialects);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pNegotiateRequestHeader->usDialectCount)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (; iDialect < pNegotiateRequestHeader->usDialectCount; iDialect++)
    {
        USHORT usDialect = pusDialects[iDialect];

        if (usDialect == 0x0202)
        {
            break;
        }
    }

    if (iDialect < pNegotiateRequestHeader->usDialectCount)
    {
        PBYTE pNegHintsBlob = NULL; /* Do not free */
        ULONG ulNegHintsLength = 0;

        ntStatus = NfsGssNegHints(
                       pConnection->hGssContext,
                       &pNegHintsBlob,
                       &ulNegHintsLength);

        /* Microsoft clients ignore the security blob on the neg prot response
           so don't fail here if we can't get a negHintsBlob */

        if (ntStatus == STATUS_SUCCESS)
        {
            ntStatus = NfsMarshalNegotiateResponse_SMB_V2(
                            pConnection,
                            pNegHintsBlob,
                            ulNegHintsLength,
                            pSmbResponse);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }
    else
    {
        // TODO: Figure out the right response here
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsBuildNegotiateResponse_SMB_V2(
    IN  PLWIO_NFS_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS    ntStatus         = STATUS_SUCCESS;
    PSMB_PACKET pSmbResponse     = NULL;
    PBYTE       pNegHintsBlob    = NULL; /* Do not free */
    ULONG       ulNegHintsLength = 0;
    NFS_MESSAGE_SMB_V2 response  = {0};

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2InitPacket(pSmbResponse, FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsGssNegHints(
                   pConnection->hGssContext,
                   &pNegHintsBlob,
                   &ulNegHintsLength);

    /* Microsoft clients ignore the security blob on the neg prot response
       so don't fail here if we can't get a negHintsBlob */

    if (ntStatus == STATUS_SUCCESS)
    {
        response.pBuffer = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
        response.ulBytesAvailable =
                        pSmbResponse->bufferLen - pSmbResponse->bufferUsed;

        ntStatus = NfsMarshalNegotiateResponse_SMB_V2(
                        pConnection,
                        pNegHintsBlob,
                        ulNegHintsLength,
                        &response);
        BAIL_ON_NT_STATUS(ntStatus);

        pSmbResponse->bufferUsed += response.ulMessageSize;

        ntStatus = SMB2MarshalFooter(pSmbResponse);
    }
    else
    {
        ntStatus = STATUS_NOT_SUPPORTED;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
NfsMarshalNegotiateResponse_SMB_V2(
    PLWIO_NFS_CONNECTION pConnection,
    PBYTE                pSessionKey,
    ULONG                ulSessionKeyLength,
    PNFS_MESSAGE_SMB_V2  pSmbResponse
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PSMB2_NEGOTIATE_RESPONSE_HEADER pNegotiateHeader = NULL; // Do not free
    PNFS_PROPERTIES pServerProperties = &pConnection->serverProperties;
    PBYTE  pDataCursor      = NULL;
    PBYTE  pOutBuffer       = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset         = 0;
    ULONG  ulBytesUsed      = 0;
    ULONG  ulTotalBytesUsed = 0;
    LONG64 llCurTime        = 0LL;

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM2_NEGOTIATE,
                1, /* usEpoch      */
                1, /* usCredits    */
                0, /* usPid        */
                0, /* ullMid       */
                0, /* usTid        */
                0, /* ullSessionId */
                0, /* Async Id     */
                0, /* status       */
                TRUE, /* response */
                FALSE,
                &pSmbResponse->pHeader,
                &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNegotiateHeader = (PSMB2_NEGOTIATE_RESPONSE_HEADER)pOutBuffer;

    ulBytesUsed       = sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    pOutBuffer       += sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);

    pNegotiateHeader->usDialect = 0x0202;

    pNegotiateHeader->ucFlags = 0;

    if (pServerProperties->bEnableSecuritySignatures)
    {
        pNegotiateHeader->ucFlags |= 0x1;
    }

    if (pServerProperties->bRequireSecuritySignatures)
    {
        pNegotiateHeader->ucFlags |= 0x2;
    }

    pNegotiateHeader->ulMaxReadSize = pServerProperties->MaxBufferSize;
    pNegotiateHeader->ulMaxWriteSize = pServerProperties->MaxBufferSize;
    pNegotiateHeader->ulCapabilities = 0;
    pNegotiateHeader->ulMaxTxSize = pServerProperties->MaxBufferSize;

    ntStatus = WireGetCurrentNTTime(&llCurTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pNegotiateHeader->ullCurrentTime = llCurTime;

    ntStatus = NfsElementsGetBootTime(&pNegotiateHeader->ullBootTime);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(&pNegotiateHeader->serverGUID[0],
            pServerProperties->GUID,
            sizeof(pServerProperties->GUID));

    if (ulOffset % 8)
    {
        USHORT usAlign = (8 - (ulOffset % 8));

        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pOutBuffer       += usAlign;
        ulBytesUsed      += usAlign;
        ulOffset         += usAlign;
        ulBytesAvailable -= usAlign;
        ulTotalBytesUsed += usAlign;
    }

    pNegotiateHeader->usBlobOffset = ulOffset;
    pNegotiateHeader->usBlobLength = ulSessionKeyLength;

    if (ulBytesAvailable < ulSessionKeyLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDataCursor = pSmbResponse->pBuffer + ulOffset;

    memcpy(pDataCursor, pSessionKey, ulSessionKeyLength);

    pNegotiateHeader->usLength = ulBytesUsed + 1; /* add one for dynamic part */

    ulTotalBytesUsed += ulSessionKeyLength;
    // ulBytesUsed += ulSessionKeyLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader      = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}
