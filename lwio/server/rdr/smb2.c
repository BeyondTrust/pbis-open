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
 *        smb2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        Common SMB2 Code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

BOOLEAN
RdrSmb2ShouldSignPacket(
    PSMB_PACKET pPacket,
    BOOLEAN bServerSigningEnabled,
    BOOLEAN bServerSigningRequired,
    BOOLEAN bClientSigningEnabled,
    BOOLEAN bClientSigningRequired
    )
{
    switch(SMB_HTOL16(pPacket->pSMB2Header->command))
    {
    case COM2_ECHO:
    case COM2_CANCEL:
        return FALSE;
    default:
        return (bServerSigningRequired || bClientSigningRequired ||
                (bServerSigningEnabled && bClientSigningEnabled));
    }
}

BOOLEAN
RdrSmb2ShouldVerifyPacket(
    PSMB_PACKET pPacket,
    BOOLEAN bClientSigningRequired
    )
{
    if (SMB_HTOL32(pPacket->pSMB2Header->error) == STATUS_PENDING)
    {
        /* Interim responses are not signed */
        return FALSE;
    }

    switch(SMB_HTOL16(pPacket->pSMB2Header->command))
    {
    case COM2_BREAK:
        return FALSE;
    default:
        return (SMB_HTOL32(pPacket->pSMB2Header->ulFlags) & SMB2_FLAGS_SIGNED ||
                bClientSigningRequired);
    }
}

NTSTATUS
RdrSmb2DecodeHeader(
    PSMB_PACKET pPacket,
    BOOLEAN bVerifySignature,
    PBYTE pSessionKey,
    DWORD dwSessionKeyLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pCursor = (PBYTE) pPacket->pSMB2Header;
    ULONG ulRemaining = pPacket->bufferUsed - sizeof(NETBIOS_HEADER);
    USHORT usParamLen = 0;

    /* Advance past basic header */
    status = Advance(&pCursor, &ulRemaining, sizeof(*pPacket->pSMB2Header));
    BAIL_ON_NT_STATUS(status);

    switch (SMB_HTOL16(pPacket->pSMB2Header->command))
    {
    case COM2_ECHO:
        /* These packet types don't need to be verified */
        bVerifySignature = FALSE;
        break;
    default:
        break;
    }

    if (bVerifySignature)
    {
        status = RdrSmb2VerifySignature(pPacket, pSessionKey, dwSessionKeyLength);
        BAIL_ON_NT_STATUS(status);
    }

    SMB_HTOL16_INPLACE(pPacket->pSMB2Header->usHeaderLen);
    SMB_HTOL16_INPLACE(pPacket->pSMB2Header->usEpoch);
    SMB_HTOL32_INPLACE(pPacket->pSMB2Header->error);
    SMB_HTOL16_INPLACE(pPacket->pSMB2Header->command);
    SMB_HTOL16_INPLACE(pPacket->pSMB2Header->usCredits);
    SMB_HTOL32_INPLACE(pPacket->pSMB2Header->ulFlags);
    SMB_HTOL32_INPLACE(pPacket->pSMB2Header->ulChainOffset);
    SMB_HTOL64_INPLACE(pPacket->pSMB2Header->ullCommandSequence);
    SMB_HTOL32_INPLACE(pPacket->pSMB2Header->ulPid);
    SMB_HTOL32_INPLACE(pPacket->pSMB2Header->ulTid);
    SMB_HTOL64_INPLACE(pPacket->pSMB2Header->ullSessionId);

    if (pPacket->pSMB2Header->usHeaderLen != 64)
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }

    pPacket->pParams = pCursor;

    status = UnmarshalUshort(&pCursor, &ulRemaining, &usParamLen);
    BAIL_ON_NT_STATUS(status);

    /* Mask off dynamic bit */
    usParamLen &= (USHORT) ~0x1;

    if (usParamLen < sizeof(USHORT))
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(status);
    }

    status = Advance(&pCursor, &ulRemaining, usParamLen - sizeof(USHORT));
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2BeginPacket(
    PSMB_PACKET pPacket
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    pPacket->protocolVer = SMB_PROTOCOL_VERSION_2;
    pPacket->pNetBIOSHeader = (NETBIOS_HEADER*) pPacket->pRawBuffer;
    pPacket->pSMB2Header = (PSMB2_HEADER) (pPacket->pRawBuffer + sizeof(NETBIOS_HEADER));
    pPacket->bufferUsed = sizeof(NETBIOS_HEADER);

    return status;
}

NTSTATUS
RdrSmb2EncodeHeader(
    PSMB_PACKET pPacket,
    USHORT usCommand,
    ULONG ulFlags,
    ULONG ulPid,
    ULONG ulTid,
    ULONG64 ullSessionId,
    PBYTE* ppCursor,
    PULONG pulRemaining
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSMB2_HEADER pHeader = pPacket->pSMB2Header;
    PBYTE pCursor = (PBYTE) pHeader;
    ULONG ulRemaining = PACKET_LENGTH_REMAINING(pPacket, pCursor);

    /* Advance cursor past header to ensure buffer space */
    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->smb[0] = 0xFE;
    pHeader->smb[1] = 'S';
    pHeader->smb[2] = 'M';
    pHeader->smb[3] = 'B';

    pHeader->usHeaderLen = SMB_HTOL16(sizeof(*pHeader));
    pHeader->usEpoch = SMB_HTOL16(0);
    pHeader->error = SMB_HTOL32(STATUS_SUCCESS);
    pHeader->command = SMB_HTOL16(usCommand);
    pHeader->usCredits = SMB_HTOL16(0);
    pHeader->ulFlags = SMB_HTOL32(ulFlags);
    /* ullCommandSequence will be filled in when the packet is placed in the send queue */
    pHeader->ulPid = SMB_HTOL32(ulPid);
    pHeader->ulTid = SMB_HTOL32(ulTid);
    pHeader->ullSessionId = SMB_HTOL64(ullSessionId);
    /* signature will be computed when complete message is ready */

    *ppCursor = pCursor;
    *pulRemaining = ulRemaining;

cleanup:

    return status;

error:

    *ppCursor = NULL;
    *pulRemaining = 0;

    goto cleanup;
}

NTSTATUS
RdrSmb2FinishCommand(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pPacket->bufferUsed < *ppCursor - pPacket->pRawBuffer)
    {
        pPacket->bufferUsed = *ppCursor - pPacket->pRawBuffer;
        pPacket->pNetBIOSHeader->len = htonl(pPacket->bufferUsed - sizeof(NETBIOS_HEADER));
    }

    return status;
}

NTSTATUS
RdrSmb2Sign(
    PSMB_PACKET pPacket,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;

    if (pSessionKey)
    {
        BYTE  sessionKey[16];
        PBYTE pBuffer = (PBYTE)pPacket->pSMB2Header;
        ULONG ulBytesAvailable = htonl(pPacket->pNetBIOSHeader->len);
        ULONG ulChainOffset = 0;

        memset(&sessionKey[0], 0, sizeof(sessionKey));
        memcpy(&sessionKey[0],
               pSessionKey,
               SMB_MIN(ulSessionKeyLength, sizeof(sessionKey)));

        while (pBuffer)
        {
            UCHAR ucDigest[EVP_MAX_MD_SIZE];
            ULONG ulDigest = sizeof(ucDigest);
            PSMB2_HEADER pHeader = NULL;
            ULONG ulPacketSize = ulBytesAvailable;

            if (ulBytesAvailable < sizeof(SMB2_HEADER))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pHeader = (PSMB2_HEADER)pBuffer;

            ulChainOffset = SMB_HTOL32(pHeader->ulChainOffset);

            if (ulChainOffset)
            {
                if (ulBytesAvailable < ulChainOffset)
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ulPacketSize = ulChainOffset;
            }

            pHeader->ulFlags = SMB_HTOL32(SMB_HTOL32(pHeader->ulFlags) | SMB2_FLAGS_SIGNED);

            memset(&pHeader->signature[0],
                   0,
                   sizeof(pHeader->signature));

            HMAC(EVP_sha256(),
                 &sessionKey[0],
                 sizeof(sessionKey),
                 (PBYTE)pHeader,
                 ulPacketSize,
                 &ucDigest[0],
                 &ulDigest);

            memcpy(&pHeader->signature[0],
                   &ucDigest[0],
                   sizeof(pHeader->signature));

            if (ulChainOffset)
            {
                pBuffer          += ulChainOffset;
                ulBytesAvailable -= ulChainOffset;
            }
            else
            {
                pBuffer = NULL;
            }
        }
    }

error:

    return ntStatus;
}

NTSTATUS
RdrSmb2VerifySignature(
    PSMB_PACKET pPacket,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;

    if (pSessionKey)
    {
        BYTE    sessionKey[16];
        PBYTE   pBuffer          = pPacket->pRawBuffer + sizeof(NETBIOS_HEADER);
        ULONG   ulBytesAvailable = pPacket->pNetBIOSHeader->len;
        uint8_t origSignature[16];
        UCHAR   ucDigest[EVP_MAX_MD_SIZE];
        ULONG   ulDigest = sizeof(ucDigest);
        ULONG   ulChainOffset = 0;

        if (!pBuffer)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memset(&sessionKey[0], 0, sizeof(sessionKey));
        memcpy(&sessionKey[0],
               pSessionKey,
               SMB_MIN(ulSessionKeyLength, sizeof(sessionKey)));

        while (pBuffer)
        {
            PSMB2_HEADER pHeader      = NULL;
            ULONG        ulPacketSize = ulBytesAvailable;

            if (ulBytesAvailable < sizeof(SMB2_HEADER))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pHeader = (PSMB2_HEADER)pBuffer;

            ulChainOffset = SMB_HTOL32(pHeader->ulChainOffset);

            if (ulChainOffset)
            {
                if (ulBytesAvailable < ulChainOffset)
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ulPacketSize = ulChainOffset;
            }

            memcpy(origSignature,
                   &pHeader->signature[0],
                   sizeof(pHeader->signature));

            memset(&pHeader->signature[0],
                   0,
                   sizeof(pHeader->signature));

            HMAC(EVP_sha256(),
                 &sessionKey[0],
                 sizeof(sessionKey),
                 pBuffer,
                 ulPacketSize,
                 &ucDigest[0],
                 &ulDigest);

            if (memcmp(&origSignature[0], &ucDigest[0], sizeof(origSignature)))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            }

            // restore signature
            memcpy(&pHeader->signature[0],
                   &origSignature[0],
                   sizeof(origSignature));

            BAIL_ON_NT_STATUS(ntStatus);

            if (ulChainOffset)
            {
                pBuffer          += ulChainOffset;
                ulBytesAvailable -= ulChainOffset;
            }
            else
            {
                pBuffer = NULL;
            }
        }
    }

cleanup:

    return ntStatus;

error:

    LWIO_LOG_WARNING("SMB2 Packet verification failed (status = 0x%08X)", ntStatus);

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeStubRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    /*
     * Marshal a request that has no useful payload:
     * Length 4 with a reserved USHORT field set to 0
     */
    status = MarshalUshort(ppCursor, pulRemaining, 4);
    BAIL_ON_NT_STATUS(status);

    status = MarshalUshort(ppCursor, pulRemaining, 0);
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

NTSTATUS
RdrSmb2DecodeNegotiateResponse(
    PSMB_PACKET pPacket,
    PRDR_SMB2_NEGOTIATE_RESPONSE_HEADER* ppHeader,
    PBYTE* ppNegHint,
    PULONG pulNegHintLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_NEGOTIATE_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    /* FIXME: this calculation is incorrect for chaining */
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);

    pHeader = (PRDR_SMB2_NEGOTIATE_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL16_INPLACE(pHeader->usDialect);
    SMB_HTOL32_INPLACE(pHeader->ulCapabilities);
    SMB_HTOL32_INPLACE(pHeader->ulMaxTransactionSize);
    SMB_HTOL32_INPLACE(pHeader->ulMaxReadSize);
    SMB_HTOL32_INPLACE(pHeader->ulMaxWriteSize);
    SMB_HTOL64_INPLACE(pHeader->llTime);
    SMB_HTOL64_INPLACE(pHeader->llBootTime);
    SMB_HTOL16_INPLACE(pHeader->usHintOffset);
    SMB_HTOL16_INPLACE(pHeader->usHintLength);

    status = AdvanceTo(&pCursor, &ulRemaining, (PBYTE) pPacket->pSMB2Header + pHeader->usHintOffset);
    BAIL_ON_NT_STATUS(status);

    *ppHeader = pHeader;
    *ppNegHint = pCursor;
    *pulNegHintLength = pHeader->usHintLength;

cleanup:

    return status;

error:

    *ppNegHint = NULL;
    *pulNegHintLength = 0;

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeSessionSetupRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    BOOLEAN bSigningEnabled,
    BOOLEAN bSigningRequired,
    BOOLEAN bDfsEnabled,
    PBYTE pBlob,
    ULONG ulBlobLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_SESSION_SETUP_REQUEST_HEADER pHeader = NULL;

    pHeader = (PRDR_SMB2_SESSION_SETUP_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->ucVcNumber = 0;
    pHeader->ucSecurityMode =
        (bSigningEnabled ? RDR_SMB2_SECMODE_SIGNING_ENABLED : 0) |
        (bSigningRequired ? RDR_SMB2_SECMODE_SIGNING_REQUIRED : 0);
    pHeader->ulCapabilities = bDfsEnabled ? RDR_SMB2_CAP_DFS : 0;
    pHeader->ulChannel = SMB_HTOL32(0);
    pHeader->usBlobLength = SMB_HTOL16((USHORT) ulBlobLength);
    pHeader->ullPrevSessionId = SMB_HTOL32(0);

    /* Fill in offset field */
    pHeader->usBlobOffset = SMB_HTOL16((USHORT) PACKET_HEADER_OFFSET(pPacket, *ppCursor));

    /* Fill in blob */
    status = MarshalData(ppCursor, pulRemaining, pBlob, ulBlobLength);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeSessionSetupResponse(
    PSMB_PACKET pPacket,
    PUSHORT pusSessionFlags,
    PBYTE* ppBlob,
    PULONG pulBlobLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_SESSION_SETUP_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = PACKET_USED_REMAINING(pPacket, pPacket->pParams);

    pHeader = (PRDR_SMB2_SESSION_SETUP_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL16_INPLACE(pHeader->usSessionFlags);
    SMB_HTOL16_INPLACE(pHeader->usBlobOffset);
    SMB_HTOL16_INPLACE(pHeader->usBlobLength);

    status = AdvanceTo(&pCursor, &ulRemaining, (PBYTE) pPacket->pSMB2Header + pHeader->usBlobOffset);
    BAIL_ON_NT_STATUS(status);

    if (pusSessionFlags)
    {
        *pusSessionFlags = pHeader->usSessionFlags;
    }

    if (ppBlob)
    {
        *ppBlob = pCursor;
    }

    if (pulBlobLength)
    {
        *pulBlobLength = pHeader->usBlobLength;
    }

 cleanup:

    return status;

 error:

    if (pusSessionFlags)
    {
        *pusSessionFlags = 0;
    }

    if (ppBlob)
    {
        *ppBlob = 0;
    }

    if (pulBlobLength)
    {
        *pulBlobLength = 0;
    }

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeTreeConnectRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    PCWSTR pwszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_TREE_CONNECT_REQUEST_HEADER pHeader = NULL;
    ULONG ulPathLength = LwRtlWC16StringNumChars(pwszPath);
    PBYTE pFilename = NULL;

    if (ulPathLength > 256)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    pHeader = (PRDR_SMB2_TREE_CONNECT_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->usPathLength = SMB_HTOL16(ulPathLength * sizeof(WCHAR));

    /* Align to WCHAR */
    status = Align((PBYTE) pPacket->pSMB2Header, ppCursor, pulRemaining, sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    pFilename = *ppCursor;

    /* Fill in offset field */
    pHeader->usPathOffset = SMB_HTOL16((USHORT) PACKET_HEADER_OFFSET(pPacket, pFilename));

    /* Fill in data */
    status = Advance(ppCursor, pulRemaining, ulPathLength * sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOLWSTR(
        pFilename,
        pwszPath,
        ulPathLength);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

 error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeTreeConnectResponse(
    PSMB_PACKET pPacket,
    PRDR_SMB2_TREE_CONNECT_RESPONSE_HEADER* ppHeader
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_TREE_CONNECT_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);

    pHeader = (PRDR_SMB2_TREE_CONNECT_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL16_INPLACE(pHeader->usShareType);
    SMB_HTOL32_INPLACE(pHeader->ulShareFlags);
    SMB_HTOL32_INPLACE(pHeader->ulShareCapabilities);
    SMB_HTOL32_INPLACE(pHeader->ulShareAccessMask);

    *ppHeader = pHeader;

cleanup:

    return status;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeCreateRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucOplockLevel,
    ULONG ulImpersonationLevel,
    ULONG ulDesiredAccess,
    ULONG ulFileAttributes,
    ULONG ulShareAccess,
    ULONG ulCreateDisposition,
    ULONG ulCreateOptions,
    PCWSTR pwszPath,
    PULONG* ppulCreateContextsOffset,
    PULONG* ppulCreateContextsLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_CREATE_REQUEST_HEADER pHeader = NULL;
    ULONG ulPathLength = 0;
    PBYTE pFilename = NULL;

    while (pwszPath[0] == '\\')
    {
        pwszPath++;
    }

    ulPathLength = LwRtlWC16StringNumChars(pwszPath);

    pHeader = (PRDR_SMB2_CREATE_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->ucOplockLevel = ucOplockLevel;
    pHeader->ulImpersonationLevel = SMB_HTOL32(ulImpersonationLevel);
    pHeader->ulDesiredAccess = SMB_HTOL32(ulDesiredAccess);
    pHeader->ulFileAttributes = SMB_HTOL32(ulFileAttributes);
    pHeader->ulShareAccess = SMB_HTOL32(ulShareAccess);
    pHeader->ulCreateDisposition = SMB_HTOL32(ulCreateDisposition);
    pHeader->ulCreateOptions = SMB_HTOL32(ulCreateOptions);
    pHeader->usNameLength = SMB_HTOL16((USHORT) (ulPathLength * sizeof(WCHAR)));
    pHeader->ulCreateContextLength = 0;
    pHeader->ulCreateContextOffset = 0;

    pFilename = *ppCursor;

    /* Fill in offset field */
    pHeader->usNameOffset = SMB_HTOL16((USHORT) PACKET_HEADER_OFFSET(pPacket, pFilename));

    /* Fill in data */
    if (ulPathLength)
    {
        status = Advance(ppCursor, pulRemaining, ulPathLength * sizeof(WCHAR));
        BAIL_ON_NT_STATUS(status);

        SMB_HTOLWSTR(
            pFilename,
            pwszPath,
            ulPathLength);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        /*
         * There must be at least 1 byte in the buffer.  We use 2
         * for the sake of alignment.
         */
        status = MarshalUshort(ppCursor, pulRemaining, 0);
        BAIL_ON_NT_STATUS(status);
    }

    if (ppulCreateContextsOffset)
    {
        *ppulCreateContextsOffset = &pHeader->ulCreateContextOffset;
    }

    if (ppulCreateContextsLength)
    {
        *ppulCreateContextsLength = &pHeader->ulCreateContextLength;
    }

cleanup:

    return status;

 error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeCreateResponse(
    PSMB_PACKET pPacket,
    PRDR_SMB2_CREATE_RESPONSE_HEADER* ppHeader
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_CREATE_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);

    pHeader = (PRDR_SMB2_CREATE_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL32_INPLACE(pHeader->ulCreateAction);
    SMB_HTOL64_INPLACE(pHeader->ullCreationTime);
    SMB_HTOL64_INPLACE(pHeader->ullLastAccessTime);
    SMB_HTOL64_INPLACE(pHeader->ullLastWriteTime);
    SMB_HTOL64_INPLACE(pHeader->ullLastChangeTime);
    SMB_HTOL64_INPLACE(pHeader->ullAllocationSize);
    SMB_HTOL64_INPLACE(pHeader->ullEndOfFile);
    SMB_HTOL32_INPLACE(pHeader->ulFileAttributes);
    SMB_HTOL64_INPLACE(pHeader->fid.ullPersistentId);
    SMB_HTOL64_INPLACE(pHeader->fid.ullVolatileId);
    SMB_HTOL32_INPLACE(pHeader->ulCreateContextOffset);
    SMB_HTOL32_INPLACE(pHeader->ulCreateContextLength);

    *ppHeader = pHeader;

cleanup:

    return status;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeCloseRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    USHORT usFlags,
    PRDR_SMB2_FID pFid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_CLOSE_REQUEST_HEADER pHeader = NULL;

    pHeader = (PRDR_SMB2_CLOSE_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader));
    pHeader->ulReserved = 0;
    pHeader->usFlags = SMB_HTOL16(usFlags);
    pHeader->fid.ullPersistentId = SMB_HTOL64(pFid->ullPersistentId);
    pHeader->fid.ullVolatileId = SMB_HTOL64(pFid->ullVolatileId);

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeQueryInfoRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucInfoType,
    UCHAR ucFileInfoClass,
    ULONG ulOutputBufferLen,
    ULONG ulAdditionalInfo,
    ULONG ulFlags,
    PRDR_SMB2_FID pFid,
    PULONG* ppulInputBufferLen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_QUERY_INFO_REQUEST_HEADER pHeader = NULL;

    pHeader = (PRDR_SMB2_QUERY_INFO_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->ucInfoType = ucInfoType;
    pHeader->ucInfoClass = ucFileInfoClass;
    pHeader->ulOutputBufferLen = SMB_HTOL32(ulOutputBufferLen);
    pHeader->usReserved = 0;
    pHeader->usInputBufferOffset = SMB_HTOL16((USHORT) PACKET_HEADER_OFFSET(pPacket, *ppCursor));
    pHeader->ulInputBufferLen = SMB_HTOL32(0);
    pHeader->ulAdditionalInfo = SMB_HTOL32(ulAdditionalInfo);
    pHeader->ulFlags = SMB_HTOL32(ulFlags);
    pHeader->fid.ullPersistentId = SMB_HTOL64(pFid->ullPersistentId);
    pHeader->fid.ullVolatileId = SMB_HTOL64(pFid->ullVolatileId);

    if (ppulInputBufferLen)
    {
        *ppulInputBufferLen = &pHeader->ulInputBufferLen;
    }

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeQueryInfoResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppOutputBuffer,
    PULONG pulOutputBufferLen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_QUERY_INFO_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);
    PBYTE pOutputBuffer = NULL;

    pHeader = (PRDR_SMB2_QUERY_INFO_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL16_INPLACE(pHeader->usOutBufferOffset);
    SMB_HTOL32_INPLACE(pHeader->ulOutBufferLength);

    /* Advance to beginning of buffer */
    status = AdvanceTo(&pCursor, &ulRemaining, (PBYTE) pPacket->pSMB2Header + pHeader->usOutBufferOffset);
    BAIL_ON_NT_STATUS(status);

    pOutputBuffer = pCursor;

    /* Advance to end of buffer to check bounds */
    status = Advance(&pCursor, &ulRemaining, pHeader->ulOutBufferLength);

    *ppOutputBuffer = pOutputBuffer;
    *pulOutputBufferLen = pHeader->ulOutBufferLength;

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeQueryDirectoryRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucFileInfoClass,
    UCHAR ucFlags,
    ULONG ulFileIndex,
    PRDR_SMB2_FID pFid,
    PCWSTR pwszPattern,
    ULONG ulOutputBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_QUERY_DIRECTORY_REQUEST_HEADER pHeader = NULL;
    WCHAR wszAllPattern[] = {'*', 0};
    ULONG ulPatternLength = 0;
    PBYTE pFilename = NULL;

    if (!pwszPattern)
    {
        pwszPattern = wszAllPattern;
    }

    ulPatternLength = LwRtlWC16StringNumChars(pwszPattern);

    pHeader = (PRDR_SMB2_QUERY_DIRECTORY_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->ucInfoClass = ucFileInfoClass;
    pHeader->ucSearchFlags = ucFlags;
    pHeader->ulFileIndex = SMB_HTOL32(ulFileIndex);
    pHeader->fid.ullPersistentId = SMB_HTOL64(pFid->ullPersistentId);
    pHeader->fid.ullVolatileId = SMB_HTOL64(pFid->ullVolatileId);
    pHeader->usFilenameOffset = /*pwszPattern ?*/ SMB_HTOL16(PACKET_HEADER_OFFSET(pPacket, *ppCursor)) /* : 0 */;
    pHeader->usFilenameLength = SMB_HTOL16(ulPatternLength * sizeof(WCHAR));
    pHeader->ulOutBufferLength = SMB_HTOL32(ulOutputBufferLength);

    /* Fill in data */
    if (ulPatternLength)
    {
        pFilename = *ppCursor;

        status = Advance(ppCursor, pulRemaining, ulPatternLength * sizeof(WCHAR));
        BAIL_ON_NT_STATUS(status);

        SMB_HTOLWSTR(
            pFilename,
            pwszPattern,
            ulPatternLength);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeQueryDirectoryResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppOutputBuffer,
    PULONG pulOutputBufferLen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_QUERY_DIRECTORY_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);
    PBYTE pOutputBuffer = NULL;

    pHeader = (PRDR_SMB2_QUERY_DIRECTORY_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL16_INPLACE(pHeader->usOutBufferOffset);
    SMB_HTOL32_INPLACE(pHeader->ulOutBufferLength);

    /* Advance to beginning of buffer */
    status = AdvanceTo(&pCursor, &ulRemaining, (PBYTE) pPacket->pSMB2Header + pHeader->usOutBufferOffset);
    BAIL_ON_NT_STATUS(status);

    pOutputBuffer = pCursor;

    /* Advance to end of buffer to check bounds */
    status = Advance(&pCursor, &ulRemaining, pHeader->ulOutBufferLength);

    *ppOutputBuffer = pOutputBuffer;
    *pulOutputBufferLen = pHeader->ulOutBufferLength;

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeReadRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    ULONG ulDataLength,
    LONG64 llDataOffset,
    PRDR_SMB2_FID pFid,
    ULONG ulMinimumCount,
    ULONG ulRemainingBytes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_READ_REQUEST_HEADER pHeader = NULL;

    pHeader = (PRDR_SMB2_READ_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->ucDataOffset = 0;
    pHeader->ucReserved = 0;
    pHeader->ulDataLength = SMB_HTOL32(ulDataLength);
    pHeader->ullFileOffset = SMB_HTOL64(llDataOffset);
    pHeader->fid.ullPersistentId = SMB_HTOL64(pFid->ullPersistentId);
    pHeader->fid.ullVolatileId = SMB_HTOL64(pFid->ullVolatileId);
    pHeader->ulMinimumCount = SMB_HTOL32(ulMinimumCount);
    pHeader->ulRemainingBytes = SMB_HTOL32(ulRemainingBytes);
    pHeader->usReadChannelInfoOffset = 0;
    pHeader->usReadChannelInfoLength = 0;

    /* We must put at least 1 byte into the read channel buffer */
    status = MarshalByte(ppCursor, pulRemaining, 0);
    BAIL_ON_NT_STATUS(status);

 cleanup:

    return status;

 error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeReadResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppDataBuffer,
    PULONG pulDataLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_READ_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);
    PBYTE pDataBuffer = NULL;

    pHeader = (PRDR_SMB2_READ_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL16_INPLACE(pHeader->usDataOffset);
    SMB_HTOL32_INPLACE(pHeader->ulDataLength);

    /* Advance to beginning of buffer */
    status = AdvanceTo(&pCursor, &ulRemaining, (PBYTE) pPacket->pSMB2Header + pHeader->usDataOffset);
    BAIL_ON_NT_STATUS(status);

    pDataBuffer = pCursor;

    /* Advance to end of buffer to check bounds */
    status = Advance(&pCursor, &ulRemaining, pHeader->ulDataLength);

    *ppDataBuffer = pDataBuffer;
    *pulDataLength = pHeader->ulDataLength;

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeWriteRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    LONG64 llDataOffset,
    PRDR_SMB2_FID pFid,
    ULONG ulRemainingBytes,
    ULONG ulFlags,
    PULONG* ppulDataLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_WRITE_REQUEST_HEADER pHeader = NULL;

    pHeader = (PRDR_SMB2_WRITE_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->usDataOffset = SMB_HTOL16((USHORT) PACKET_HEADER_OFFSET(pPacket, *ppCursor));
    pHeader->ullFileOffset = SMB_HTOL64(llDataOffset);
    pHeader->fid.ullPersistentId = SMB_HTOL64(pFid->ullPersistentId);
    pHeader->fid.ullVolatileId = SMB_HTOL64(pFid->ullVolatileId);
    pHeader->ulRemainingBytes = SMB_HTOL32(ulRemainingBytes);
    pHeader->ulFlags = SMB_HTOL32(ulFlags);
    pHeader->usWriteChannelInfoOffset = 0;
    pHeader->usWriteChannelInfoLength = 0;

    if (ppulDataLength)
    {
        *ppulDataLength = &pHeader->ulDataLength;
    }

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeWriteResponse(
    PSMB_PACKET pPacket,
    PULONG pulDataCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_WRITE_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);

    pHeader = (PRDR_SMB2_WRITE_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL32_INPLACE(pHeader->ulBytesWritten);

    *pulDataCount = pHeader->ulBytesWritten;

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeSetInfoRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucInfoType,
    UCHAR ucFileInfoClass,
    ULONG ulAdditionalInfo,
    PRDR_SMB2_FID pFid,
    PULONG* ppulBufferLen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_SET_INFO_REQUEST_HEADER pHeader = NULL;

    pHeader = (PRDR_SMB2_SET_INFO_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->ucInfoType = ucInfoType;
    pHeader->ucInfoClass = ucFileInfoClass;
    pHeader->usReserved = 0;
    pHeader->usInputBufferOffset = SMB_HTOL16((USHORT) PACKET_HEADER_OFFSET(pPacket, *ppCursor));
    pHeader->ulAdditionalInfo = SMB_HTOL32(ulAdditionalInfo);
    pHeader->fid.ullPersistentId = SMB_HTOL64(pFid->ullPersistentId);
    pHeader->fid.ullVolatileId = SMB_HTOL64(pFid->ullVolatileId);

    if (ppulBufferLen)
    {
        *ppulBufferLen = &pHeader->ulInputBufferLen;
    }

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2EncodeIoctlRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    ULONG ulControlCode,
    PRDR_SMB2_FID pFid,
    ULONG ulMaxInputResponse,
    ULONG ulMaxOutputResponse,
    BOOLEAN bIsFsctl,
    PULONG* ppulInputSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_IOCTL_REQUEST_HEADER pHeader = NULL;

    pHeader = (PRDR_SMB2_IOCTL_REQUEST_HEADER) *ppCursor;
    /* Advance cursor past header to ensure buffer space */
    status = Advance(ppCursor, pulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    pHeader->usLength = SMB_HTOL16(sizeof(*pHeader) | 0x1);
    pHeader->usReserved = 0;
    pHeader->ulFunctionCode = SMB_HTOL32(ulControlCode);

    if (pFid)
    {
        pHeader->fid.ullPersistentId = SMB_HTOL64(pFid->ullPersistentId);
        pHeader->fid.ullVolatileId = SMB_HTOL64(pFid->ullVolatileId);
    }
    else
    {
        memset(&pHeader->fid, 0xFF, sizeof(pHeader->fid));
    }

    pHeader->ulInOffset = SMB_HTOL32((ULONG) PACKET_HEADER_OFFSET(pPacket, *ppCursor));
    pHeader->ulInLength = 0;
    pHeader->ulMaxInLength = SMB_HTOL32(ulMaxInputResponse);
    pHeader->ulOutOffset = 0;
    pHeader->ulOutLength = 0;
    pHeader->ulMaxOutLength = SMB_HTOL32(ulMaxOutputResponse);
    pHeader->ulFlags = SMB_HTOL32(bIsFsctl);
    pHeader->ulReserved = 0;

    if (ppulInputSize)
    {
        *ppulInputSize = &pHeader->ulInLength;
    }

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSmb2DecodeIoctlResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppOutput,
    PULONG pulOutputSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SMB2_IOCTL_RESPONSE_HEADER pHeader = NULL;
    PBYTE pCursor = pPacket->pParams;
    ULONG ulRemaining = pPacket->bufferUsed - (pPacket->pParams - pPacket->pRawBuffer);

    pHeader = (PRDR_SMB2_IOCTL_RESPONSE_HEADER) pCursor;

    status = Advance(&pCursor, &ulRemaining, sizeof(*pHeader));
    BAIL_ON_NT_STATUS(status);

    SMB_HTOL16_INPLACE(pHeader->usLength);
    SMB_HTOL32_INPLACE(pHeader->ulFunctionCode);
    SMB_HTOL64_INPLACE(pHeader->fid.ullPersistentId);
    SMB_HTOL64_INPLACE(pHeader->fid.ullVolatileId);
    SMB_HTOL32_INPLACE(pHeader->ulInOffset);
    SMB_HTOL32_INPLACE(pHeader->ulInLength);
    SMB_HTOL32_INPLACE(pHeader->ulOutOffset);
    SMB_HTOL32_INPLACE(pHeader->ulOutLength);
    SMB_HTOL32_INPLACE(pHeader->ulFlags);

    status = AdvanceTo(&pCursor, &ulRemaining, (PBYTE) pPacket->pSMB2Header + pHeader->ulOutOffset);
    BAIL_ON_NT_STATUS(status);

    *ppOutput = pCursor;

    status = Advance(&pCursor, &ulRemaining, pHeader->ulOutLength);
    BAIL_ON_NT_STATUS(status);

    *pulOutputSize = pHeader->ulOutLength;

cleanup:

    return status;

error:

    *ppOutput = NULL;
    *pulOutputSize = 0;

    goto cleanup;
}
