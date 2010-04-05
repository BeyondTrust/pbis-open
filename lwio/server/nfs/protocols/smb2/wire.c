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
 *        wire.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Wire protocol
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SMB2UnmarshalCreateContexts(
    PBYTE                pBuffer,
    ULONG                ulOffset,
    ULONG                ulPacketSize,
    PNFS_CREATE_CONTEXT* ppCreateContexts,
    PULONG               pulNumContexts
    );

NTSTATUS
SMB2InitPacket(
    IN OUT PSMB_PACKET pSmbPacket,
    IN     BOOLEAN     bAllowSignature
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE    pBuffer = pSmbPacket->pRawBuffer;
    ULONG    ulBufferAvailable = pSmbPacket->bufferLen;

    if (ulBufferAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->pNetBIOSHeader = (NETBIOS_HEADER *) (pBuffer);
    pSmbPacket->bufferUsed += sizeof(NETBIOS_HEADER);
    pBuffer += sizeof(NETBIOS_HEADER);

    pSmbPacket->pSMB2Header = (PSMB2_HEADER)pBuffer;

    pSmbPacket->protocolVer = SMB_PROTOCOL_VERSION_2;
    pSmbPacket->allowSignature = bAllowSignature;

error:

    return ntStatus;
}

NTSTATUS
NfsUnmarshalHeader_SMB_V2(
    IN     PBYTE         pBuffer,
    IN     ULONG         ulOffset,
    IN     ULONG         ulBytesAvailable,
    IN OUT PSMB2_HEADER* ppHeader,
    IN OUT PULONG        pulBytesUsed
    )
{
    NTSTATUS     ntStatus    = STATUS_SUCCESS;
    UCHAR        smb2magic[4] = {0xFE, 'S','M','B'};
    PSMB2_HEADER pHeader     = NULL; // Do not free
    ULONG        ulBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB2_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_HEADER)pBuffer;

    if (memcmp(&smb2magic[0], &pHeader->smb[0], sizeof(smb2magic)))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer          += sizeof(SMB2_HEADER);
    ulOffset         += sizeof(SMB2_HEADER);
    ulBytesAvailable -= sizeof(SMB2_HEADER);
    ulBytesUsed      += sizeof(SMB2_HEADER);

    if (ppHeader)
    {
        *ppHeader     = pHeader;
    }

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ppHeader)
    {
        *ppHeader     = NULL;
    }

    *pulBytesUsed = 0;

    goto cleanup;
}

NTSTATUS
SMB2MarshalHeader(
    IN OUT          PBYTE         pBuffer,
    IN              ULONG         ulOffset,
    IN              ULONG         ulBytesAvailable,
    IN              USHORT        usCommand,
    IN              USHORT        usEpoch,
    IN              USHORT        usCredits,
    IN              ULONG         ulPid,
    IN              ULONG64       ullMid,
    IN              ULONG         ulTid,
    IN              ULONG64       ullSessionId,
    IN              ULONG64       ullAsyncId,
    IN              NTSTATUS      status,
    IN              BOOLEAN       bIsResponse,
    IN              BOOLEAN       bIsPartOfCompoundMessage,
    IN OUT OPTIONAL PSMB2_HEADER* ppSMB2Header,
    IN OUT          PULONG        pulBytesUsed
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    PSMB2_HEADER    pSMB2Header = NULL;
    static uchar8_t smb2Magic[4] = { 0xFE, 'S', 'M', 'B' };
    ULONG           ulBytesUsed = 0;
    PBYTE           pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSMB2Header = (PSMB2_HEADER)pDataCursor;

    ulBytesUsed      += sizeof(SMB2_HEADER);
    ulBytesAvailable -= sizeof(SMB2_HEADER);
    pDataCursor      += sizeof(SMB2_HEADER);

    memcpy(&pSMB2Header->smb[0], &smb2Magic[0], sizeof(smb2Magic));

    pSMB2Header->command        = usCommand;
    pSMB2Header->usEpoch        = usEpoch;
    pSMB2Header->usCredits      = usCredits;
    pSMB2Header->ullCommandSequence = ullMid;

    if (!ullAsyncId)
    {
        pSMB2Header->ulPid = ulPid;
        pSMB2Header->ulTid = ulTid;
    }
    else
    {
        memcpy( (PBYTE)&pSMB2Header->ulPid,
                (PBYTE)&ullAsyncId,
                sizeof(ullAsyncId));

        pSMB2Header->ulFlags |= SMB2_FLAGS_ASYNC_COMMAND;
    }

    pSMB2Header->ullSessionId   = ullSessionId;
    pSMB2Header->error         = status;
    pSMB2Header->usHeaderLen = sizeof(SMB2_HEADER);

    if (bIsResponse)
    {
        pSMB2Header->ulFlags |= SMB2_FLAGS_SERVER_TO_REDIR;
    }
    if (bIsPartOfCompoundMessage)
    {
        pSMB2Header->ulFlags |= SMB2_FLAGS_RELATED_OPERATION;
    }

    *pulBytesUsed = ulBytesUsed;
    if (ppSMB2Header)
    {
        *ppSMB2Header = pSMB2Header;
    }

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;
    if (ppSMB2Header)
    {
        *ppSMB2Header = NULL;
    }

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2GetAsyncId(
    PSMB2_HEADER pHeader,
    PULONG64     pullAsyncId
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG64  ullAsyncId = 0LL;

    if (!(pHeader->ulFlags & SMB2_FLAGS_ASYNC_COMMAND))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy((PBYTE)&ullAsyncId, (PBYTE)&pHeader->ulPid, sizeof(ullAsyncId));

    *pullAsyncId = ullAsyncId;

cleanup:

    return ntStatus;

error:

    *pullAsyncId = 0LL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalNegotiateRequest(
    PNFS_MESSAGE_SMB_V2             pRequest,
    PSMB2_NEGOTIATE_REQUEST_HEADER* ppHeader,
    PUSHORT*                        ppusDialects
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pRequest->pBuffer + pRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pRequest->ulMessageSize - pRequest->ulHeaderSize;
    ULONG ulOffset = pRequest->ulHeaderSize;
    PSMB2_NEGOTIATE_REQUEST_HEADER pHeader = NULL; // Do not free
    PUSHORT pusDialects = NULL;

    if (ulBytesAvailable < sizeof(SMB2_NEGOTIATE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_NEGOTIATE_REQUEST_HEADER)pDataCursor;

    pDataCursor += sizeof(SMB2_NEGOTIATE_REQUEST_HEADER);
    ulOffset += sizeof(SMB2_NEGOTIATE_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_NEGOTIATE_REQUEST_HEADER);

    if (ulBytesAvailable < sizeof(USHORT) * pHeader->usDialectCount)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pusDialects = (PUSHORT)pDataCursor;

    *ppHeader = pHeader;
    *ppusDialects = pusDialects;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppusDialects = NULL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshallSessionSetup(
    PNFS_MESSAGE_SMB_V2                 pRequest,
    PSMB2_SESSION_SETUP_REQUEST_HEADER* ppHeader,
    PBYTE*                              ppSecurityBlob,
    PULONG                              pulSecurityBlobLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pRequest->pBuffer + pRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pRequest->ulMessageSize - pRequest->ulHeaderSize;
    ULONG ulPacketSize = pRequest->ulMessageSize;
    ULONG ulOffset = pRequest->ulHeaderSize;
    PSMB2_SESSION_SETUP_REQUEST_HEADER pSessionSetupHeader = NULL;
    PBYTE pSecurityBlob = NULL;
    ULONG ulSecurityBlobLen = 0;

    if (ulBytesAvailable < sizeof(SMB2_SESSION_SETUP_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSessionSetupHeader = (PSMB2_SESSION_SETUP_REQUEST_HEADER)pDataCursor;
    ulOffset += sizeof(SMB2_SESSION_SETUP_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_SESSION_SETUP_REQUEST_HEADER);
    pDataCursor += sizeof(SMB2_SESSION_SETUP_REQUEST_HEADER);

    // Is dynamic part present?
    if (pSessionSetupHeader->usLength & 0x1)
    {
        USHORT usAlign = ulOffset %8;
        if (usAlign)
        {
            if (ulBytesAvailable < usAlign)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulOffset += usAlign;
            ulBytesAvailable -= usAlign;
        }

        if (!pSessionSetupHeader->usBlobLength ||
            (pSessionSetupHeader->usBlobOffset < ulOffset) ||
            (pSessionSetupHeader->usBlobOffset % 8) ||
            (pSessionSetupHeader->usBlobOffset > ulPacketSize) ||
            ((pSessionSetupHeader->usBlobOffset +
              pSessionSetupHeader->usBlobLength) > pRequest->ulMessageSize))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pSecurityBlob = pRequest->pBuffer + pSessionSetupHeader->usBlobOffset;
        ulSecurityBlobLen = pSessionSetupHeader->usBlobLength;
    }

    *ppHeader = pSessionSetupHeader;
    *ppSecurityBlob = pSecurityBlob;
    *pulSecurityBlobLen = ulSecurityBlobLen;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppSecurityBlob = NULL;
    *pulSecurityBlobLen = 0;

    goto cleanup;
}

NTSTATUS
SMB2MarshalSessionSetup(
    IN OUT PBYTE              pBuffer,
    IN     ULONG              ulOffset,
    IN     ULONG              ulBytesAvailable,
    IN     SMB2_SESSION_FLAGS usFlags,
    IN     PBYTE              pSecurityBlob,
    IN     ULONG              ulSecurityBlobLen,
    IN OUT PULONG             pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_SESSION_SETUP_RESPONSE_HEADER pSessionSetupHeader = NULL;
    PBYTE pDataCursor = pBuffer;
    ULONG ulBytesUsed = 0;
    USHORT usAlign = 0;

    if (ulBytesAvailable < sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSessionSetupHeader = (PSMB2_SESSION_SETUP_RESPONSE_HEADER)pDataCursor;
    ulOffset += sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);
    ulBytesUsed += sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);
    pDataCursor += sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);

    if ((usAlign = ulOffset % 8))
    {
        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset += usAlign;
        ulBytesUsed += usAlign;
        ulBytesAvailable -= usAlign;
        pDataCursor += usAlign;
    }

    pSessionSetupHeader->usLength = ulBytesUsed;

    if (ulSecurityBlobLen)
    {
        if (ulBytesAvailable < ulSecurityBlobLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pSecurityBlob, ulSecurityBlobLen);

        pSessionSetupHeader->usLength++;

        ulBytesUsed += ulSecurityBlobLen;
    }

    pSessionSetupHeader->usSessionFlags = usFlags;
    pSessionSetupHeader->usBlobOffset = ulOffset;
    pSessionSetupHeader->usBlobLength = ulSecurityBlobLen;

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalLogoffRequest(
    IN     PNFS_MESSAGE_SMB_V2          pRequest,
    IN OUT PSMB2_LOGOFF_REQUEST_HEADER* ppHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_LOGOFF_REQUEST_HEADER pLogoffHeader = NULL; // Do not free
    ULONG ulBytesAvailable = pRequest->ulMessageSize;
    PBYTE pDataCursor = pRequest->pBuffer + pRequest->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_LOGOFF_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pLogoffHeader = (PSMB2_LOGOFF_REQUEST_HEADER)pDataCursor;

    *ppHeader = pLogoffHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalLogoffResponse(
    IN OUT PBYTE              pBuffer,
    IN     ULONG              ulOffset,
    IN     ULONG              ulBytesAvailable,
    IN OUT PULONG             pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_LOGOFF_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesUsed = 0;
    PBYTE pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_LOGOFF_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_LOGOFF_RESPONSE_HEADER)pDataCursor;
    ulBytesUsed += sizeof(SMB2_LOGOFF_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(SMB2_LOGOFF_RESPONSE_HEADER);
    // pDataCursor += sizeof(SMB2_LOGOFF_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalTreeConnect(
    IN     PNFS_MESSAGE_SMB_V2                pRequest,
    IN OUT PSMB2_TREE_CONNECT_REQUEST_HEADER* ppHeader,
    IN OUT PUNICODE_STRING                    pwszPath
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PBYTE    pDataCursor      = pRequest->pBuffer + pRequest->ulHeaderSize;
    ULONG    ulBytesAvailable = pRequest->ulMessageSize - pRequest->ulHeaderSize;
    ULONG    ulPacketSize     = pRequest->ulMessageSize;
    ULONG    ulOffset         = pRequest->ulHeaderSize;
    PSMB2_TREE_CONNECT_REQUEST_HEADER pHeader = NULL; // Do not free
    UNICODE_STRING wszPath = {0};

    if (ulBytesAvailable < sizeof(SMB2_TREE_CONNECT_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_TREE_CONNECT_REQUEST_HEADER)pDataCursor;
    ulBytesAvailable -= sizeof(SMB2_TREE_CONNECT_REQUEST_HEADER);
    ulOffset += sizeof(SMB2_TREE_CONNECT_REQUEST_HEADER);

    if (pHeader->usLength & 0x1)
    {
        if ((pHeader->usPathOffset < ulOffset) ||
            (pHeader->usPathOffset % 2) ||
            (pHeader->usPathLength % 2) ||
            (pHeader->usPathOffset > ulPacketSize) ||
            (pHeader->usPathLength + pHeader->usPathOffset) > ulPacketSize)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        wszPath.Buffer = (PWSTR)(pRequest->pBuffer + pHeader->usPathOffset);
        wszPath.Length = wszPath.MaximumLength = pHeader->usPathLength;
    }

    if (!wszPath.Length)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppHeader = pHeader;
    *pwszPath = wszPath;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalTreeConnectResponse(
    IN OUT PBYTE                               pBuffer,
    IN     ULONG                               ulOffset,
    IN     ULONG                               ulBytesAvailable,
    IN     PLWIO_NFS_CONNECTION                pConnection,
    IN     PLWIO_NFS_TREE_2                    pTree,
    IN OUT PSMB2_TREE_CONNECT_RESPONSE_HEADER* ppResponseHeader,
    IN OUT PULONG                              pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_TREE_CONNECT_RESPONSE_HEADER pResponseHeader = NULL;
    PBYTE  pDataCursor = pBuffer;
    ULONG  ulBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_TREE_CONNECT_RESPONSE_HEADER)pDataCursor;
    ulOffset += sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);
    ulBytesUsed += sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);
    pDataCursor += sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);

    pResponseHeader->usLength = ulBytesUsed;

    ntStatus = NfsGetMaximalShareAccessMask(
                    pTree->pShareInfo,
                    &pResponseHeader->ulShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pTree->pShareInfo->service)
    {
        case SHARE_SERVICE_DISK_SHARE:

            pResponseHeader->usShareType = SMB2_SHARE_TYPE_DISK;

            break;

        case SHARE_SERVICE_NAMED_PIPE:

            pResponseHeader->usShareType = SMB2_SHARE_TYPE_NAMED_PIPE;

            break;

        default:

            LWIO_LOG_DEBUG("Unrecognized share type %d", pTree->pShareInfo->service);

            break;
    }

    *ppResponseHeader = pResponseHeader;
    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;
    *ppResponseHeader = NULL;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalTreeDisconnectRequest(
    IN  PNFS_MESSAGE_SMB_V2                   pSmbRequest,
    OUT PSMB2_TREE_DISCONNECT_REQUEST_HEADER* ppTreeDisconnectHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_TREE_DISCONNECT_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppTreeDisconnectHeader = (PSMB2_TREE_DISCONNECT_REQUEST_HEADER)pDataCursor;

cleanup:

    return ntStatus;

error:

    *ppTreeDisconnectHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalTreeDisconnectResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN OUT PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_TREE_DISCONNECT_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB2_TREE_DISCONNECT_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_TREE_DISCONNECT_RESPONSE_HEADER)pBuffer;

    ulBytesUsed += sizeof(SMB2_TREE_DISCONNECT_RESPONSE_HEADER);

    pHeader->usLength = sizeof(SMB2_TREE_DISCONNECT_RESPONSE_HEADER);

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalCreateRequest(
    IN     PNFS_MESSAGE_SMB_V2          pSmbRequest,
    IN OUT PSMB2_CREATE_REQUEST_HEADER* ppCreateRequestHeader,
    IN OUT PUNICODE_STRING              pwszFileName,
    OUT    PNFS_CREATE_CONTEXT*         ppCreateContexts,
    IN OUT PULONG                       pulNumContexts
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulOffset    = pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    ULONG ulPacketSize     = pSmbRequest->ulMessageSize;
    PSMB2_CREATE_REQUEST_HEADER pHeader = NULL; // Do not free
    UNICODE_STRING              wszFileName = {0}; // Do not free
    PNFS_CREATE_CONTEXT pCreateContexts = NULL;
    ULONG               ulNumContexts = 0;

    if (ulBytesAvailable < sizeof(SMB2_CREATE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_CREATE_REQUEST_HEADER)pDataCursor;

    ulOffset += sizeof(SMB2_CREATE_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_CREATE_REQUEST_HEADER);
    pDataCursor += sizeof(SMB2_CREATE_REQUEST_HEADER);

    if (pHeader->usLength & 0x1)
    {
        if (pHeader->usNameOffset)
        {
           if ( (pHeader->usNameOffset < ulOffset) ||
                (pHeader->usNameOffset % 2) ||
                (pHeader->usNameLength % 2) ||
                (pHeader->usNameOffset > ulPacketSize) ||
                (pHeader->usNameLength + pHeader->usNameOffset) > ulPacketSize)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            wszFileName.Buffer = (PWSTR)(pSmbRequest->pBuffer +
                                     pHeader->usNameOffset);
            wszFileName.Length = wszFileName.MaximumLength = pHeader->usNameLength;

            ulOffset = pHeader->usNameOffset + pHeader->usNameLength;
        }
    }

    if (pHeader->ulCreateContextOffset && pHeader->ulCreateContextLength)
    {
        PBYTE pCreateContextBuffer = NULL;

        if ((pHeader->ulCreateContextOffset < ulOffset) ||
            ((pHeader->ulCreateContextOffset + pHeader->ulCreateContextLength) >
             ulPacketSize))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pCreateContextBuffer = pSmbRequest->pBuffer +
                               pHeader->ulCreateContextOffset;

        ntStatus = SMB2UnmarshalCreateContexts(
                        pCreateContextBuffer,
                        pHeader->ulCreateContextOffset,
                        ulPacketSize,
                        &pCreateContexts,
                        &ulNumContexts);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppCreateRequestHeader = pHeader;
    *pwszFileName = wszFileName;
    *ppCreateContexts = pCreateContexts;
    *pulNumContexts = ulNumContexts;

cleanup:

    return ntStatus;

error:

    *ppCreateRequestHeader = NULL;
    memset(pwszFileName, 0, sizeof(UNICODE_STRING));
    *ppCreateContexts = NULL;
    *pulNumContexts = 0;

    NFS_SAFE_FREE_MEMORY(pCreateContexts);

    goto cleanup;
}

static
NTSTATUS
SMB2UnmarshalCreateContexts(
    PBYTE                pBuffer,
    ULONG                ulOffset,
    ULONG                ulPacketSize,
    PNFS_CREATE_CONTEXT* ppCreateContexts,
    PULONG               pulNumContexts
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG    iContext = 0;
    ULONG    ulNumContexts = 0;
    ULONG    ulCurrentOffset = ulOffset;
    PSMB2_CREATE_CONTEXT pCContext = (PSMB2_CREATE_CONTEXT)pBuffer;
    PNFS_CREATE_CONTEXT pCreateContexts = NULL;

    while (pCContext)
    {
        ULONG ulNextOffset = 0;

        ulNumContexts++;

        ulNextOffset = pCContext->ulNextContextOffset;

        if (!ulNextOffset)
        {
            pCContext = NULL;
        }
        else
        {
            if ((ulCurrentOffset + ulNextOffset) > ulPacketSize)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pCContext = (PSMB2_CREATE_CONTEXT)((PBYTE)pCContext + ulNextOffset);
            ulCurrentOffset += ulNextOffset;
        }
    }

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_CREATE_CONTEXT) * ulNumContexts,
                    (PVOID*)&pCreateContexts);
    BAIL_ON_NT_STATUS(ntStatus);

    pCContext = (PSMB2_CREATE_CONTEXT)pBuffer;
    for (iContext = 0; iContext < ulNumContexts; iContext++)
    {
        PNFS_CREATE_CONTEXT pNfsCContext = &pCreateContexts[iContext];

        pNfsCContext->pszName = (PCSTR)((PBYTE)pCContext +
                                        pCContext->usNameOffset);
        pNfsCContext->usNameLen = pCContext->usNameLength;

        pNfsCContext->pData = (PBYTE)pCContext + pCContext->usDataOffset;
        pNfsCContext->ulDataLength = pCContext->ulDataLength;

        pNfsCContext->contextItemType = SMB2_CONTEXT_ITEM_TYPE_UNKNOWN;

        if (pNfsCContext->usNameLen)
        {
            if (!strncmp(pNfsCContext->pszName,
                         SMB2_CONTEXT_NAME_DURABLE_HANDLE,
                         sizeof(SMB2_CONTEXT_NAME_DURABLE_HANDLE) - 1))
            {
                pNfsCContext->contextItemType =
                                SMB2_CONTEXT_ITEM_TYPE_DURABLE_HANDLE;
            }
            else if (!strncmp(pNfsCContext->pszName,
                              SMB2_CONTEXT_NAME_MAX_ACCESS,
                              sizeof(SMB2_CONTEXT_NAME_MAX_ACCESS) - 1))
            {
                pNfsCContext->contextItemType =
                                SMB2_CONTEXT_ITEM_TYPE_MAX_ACCESS;
            }
            else if (!strncmp(pNfsCContext->pszName,
                            SMB2_CONTEXT_NAME_QUERY_DISK_ID,
                            sizeof(SMB2_CONTEXT_NAME_QUERY_DISK_ID) - 1))
            {
                pNfsCContext->contextItemType =
                                SMB2_CONTEXT_ITEM_TYPE_QUERY_DISK_ID;
            }
            else if (!strncmp(pNfsCContext->pszName,
                            SMB2_CONTEXT_NAME_EXT_ATTRS,
                            sizeof(SMB2_CONTEXT_NAME_EXT_ATTRS) - 1))
            {
                pNfsCContext->contextItemType =
                                SMB2_CONTEXT_ITEM_TYPE_EXT_ATTRS;
            }
            else if (!strncmp(pNfsCContext->pszName,
                            SMB2_CONTEXT_NAME_SHADOW_COPY,
                            sizeof(SMB2_CONTEXT_NAME_SHADOW_COPY) - 1))
            {
                pNfsCContext->contextItemType =
                                SMB2_CONTEXT_ITEM_TYPE_SHADOW_COPY;
            }
            else if (!strncmp(pNfsCContext->pszName,
                        SMB2_CONTEXT_NAME_SEC_DESC,
                        sizeof(SMB2_CONTEXT_NAME_SEC_DESC) - 1))
            {
                pNfsCContext->contextItemType =
                                SMB2_CONTEXT_ITEM_TYPE_SEC_DESC;
            }
        }

        pCContext = (PSMB2_CREATE_CONTEXT)((PBYTE)pCContext +
                                           pCContext->ulNextContextOffset);
    }

    *ppCreateContexts = pCreateContexts;
    *pulNumContexts   = ulNumContexts;

cleanup:

    return ntStatus;

error:

    *ppCreateContexts = NULL;
    *pulNumContexts   = 0;

    NFS_SAFE_FREE_MEMORY(pCreateContexts);

    goto cleanup;
}

NTSTATUS
SMB2MarshalCreateContext(
    IN OUT PBYTE                 pBuffer,
    IN     ULONG                 ulOffset,
    IN     PBYTE                 pName,
    IN     USHORT                usNameSize,
    IN     PBYTE                 pData,
    IN     ULONG                 ulDataSize,
    IN     ULONG                 ulBytesAvailable,
    IN OUT PULONG                pulBytesUsed,
    IN OUT PSMB2_CREATE_CONTEXT* ppCreateContext
    )
{
    NTSTATUS ntStatus    = STATUS_SUCCESS;
    PBYTE    pDataCursor = pBuffer;
    ULONG    ulBytesUsed = 0;
    ULONG    ulBytesAvailable1 = ulBytesAvailable;
    USHORT   usOffset_struct = 0;
    PSMB2_CREATE_CONTEXT pCreateContext = NULL;

    if (ulOffset % 4)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulBytesAvailable1 < sizeof(SMB2_CREATE_CONTEXT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pCreateContext = (PSMB2_CREATE_CONTEXT)pDataCursor;

    ulOffset          += sizeof(SMB2_CREATE_CONTEXT);
    ulBytesUsed       += sizeof(SMB2_CREATE_CONTEXT);
    ulBytesAvailable1 -= sizeof(SMB2_CREATE_CONTEXT);
    usOffset_struct   += sizeof(SMB2_CREATE_CONTEXT);
    pDataCursor       += sizeof(SMB2_CREATE_CONTEXT);

    if (ulBytesAvailable1 < usNameSize)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pCreateContext->usNameOffset = usOffset_struct;
    pCreateContext->usNameLength = usNameSize;
    memcpy(pDataCursor, pName, usNameSize);

    ulOffset          += usNameSize;
    ulBytesUsed       += usNameSize;
    ulBytesAvailable1 -= usNameSize;
    usOffset_struct   += usNameSize;
    pDataCursor       += usNameSize;

    if (pData)
    {
        if (usOffset_struct % 8)
        {
            USHORT usAlign = 8 - (usOffset_struct % 8);

            if (ulBytesAvailable1 < usAlign)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulOffset          += usAlign;
            ulBytesUsed       += usAlign;
            ulBytesAvailable1 -= usAlign;
            usOffset_struct   += usAlign;
            pDataCursor       += usAlign;
        }

        if (ulBytesAvailable1 < ulDataSize)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pCreateContext->usDataOffset = usOffset_struct;
        pCreateContext->ulDataLength = ulDataSize;
        memcpy(pDataCursor, pData, ulDataSize);

        // ulOffset          += ulDataSize;
        ulBytesUsed       += ulDataSize;
        // ulBytesAvailable1 -= ulDataSize;
        // usOffset_struct   += ulDataSize;
        // pDataCursor       += ulDataSize;
    }

    *pulBytesUsed    = ulBytesUsed;
    *ppCreateContext = pCreateContext;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed    = 0;
    *ppCreateContext = NULL;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalCloseRequest(
   IN     PNFS_MESSAGE_SMB_V2         pSmbRequest,
   IN OUT PSMB2_CLOSE_REQUEST_HEADER* ppHeader
   )
{
    NTSTATUS ntStatus      = STATUS_SUCCESS;
    PBYTE pDataCursor      = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_CLOSE_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_CLOSE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_CLOSE_REQUEST_HEADER)pDataCursor;

    *ppHeader = pHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalFlushRequest(
   IN     PNFS_MESSAGE_SMB_V2 pSmbRequest,
   IN OUT PSMB2_FID*          ppFid
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_FLUSH_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_FLUSH_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_FLUSH_REQUEST_HEADER)pDataCursor;

    *ppFid = &pHeader->fid;

cleanup:

    return ntStatus;

error:

    *ppFid = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalFlushResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN OUT PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_FLUSH_RESPONSE_HEADER pHeader = NULL; // Do not free
    ULONG ulBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB2_FLUSH_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_FLUSH_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_FLUSH_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_FLUSH_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalEchoRequest(
   IN     PNFS_MESSAGE_SMB_V2        pSmbRequest,
   IN OUT PSMB2_ECHO_REQUEST_HEADER* ppHeader
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_ECHO_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_ECHO_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_ECHO_REQUEST_HEADER)pDataCursor;

    *ppHeader = pHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalEchoResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN OUT PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_ECHO_RESPONSE_HEADER pHeader = NULL; // Do not free
    ULONG ulBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB2_ECHO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_ECHO_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_ECHO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_ECHO_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalGetInfoRequest(
    IN     PNFS_MESSAGE_SMB_V2            pSmbRequest,
    IN OUT PSMB2_GET_INFO_REQUEST_HEADER* ppHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_GET_INFO_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_GET_INFO_REQUEST_HEADER)pDataCursor;

    *ppHeader = pHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalSetInfoRequest(
    IN     PNFS_MESSAGE_SMB_V2            pSmbRequest,
    IN OUT PSMB2_SET_INFO_REQUEST_HEADER* ppHeader,
    IN OUT PBYTE*                         ppData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    ULONG ulPacketSize = pSmbRequest->ulMessageSize;
    PSMB2_SET_INFO_REQUEST_HEADER pHeader = NULL; // Do not free
    PBYTE pData = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_SET_INFO_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_SET_INFO_REQUEST_HEADER)pDataCursor;

    if ((pHeader->usInputBufferOffset > ulPacketSize) ||
        ((pHeader->usInputBufferOffset + pHeader->ulInputBufferLen) > ulPacketSize))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pData = pSmbRequest->pBuffer + pHeader->usInputBufferOffset;

    *ppHeader = pHeader;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppData = NULL;

    goto cleanup;
}


NTSTATUS
SMB2UnmarshalReadRequest(
    IN     PNFS_MESSAGE_SMB_V2        pSmbRequest,
    IN OUT PSMB2_READ_REQUEST_HEADER* ppRequestHeader
    )
{
    NTSTATUS ntStatus  = STATUS_SUCCESS;
    ULONG ulOffset = pSmbRequest->ulHeaderSize;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_READ_REQUEST_HEADER pRequestHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_READ_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB2_READ_REQUEST_HEADER)pDataCursor;
    ulOffset += sizeof(SMB2_READ_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_READ_REQUEST_HEADER);

    *ppRequestHeader = pRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalReadResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN     PBYTE  pData,
    IN     ULONG  ulBytesRead,
    IN     ULONG  ulBytesRemaining,
    IN OUT PULONG pulDataOffset,
    IN OUT PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_READ_RESPONSE_HEADER pHeader = NULL;
    ULONG ulDataOffset = ulOffset;
    ULONG ulBytesUsed = 0;
    PBYTE pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_READ_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_READ_RESPONSE_HEADER)pDataCursor;
    ulBytesUsed += sizeof(SMB2_READ_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_READ_RESPONSE_HEADER);
    ulDataOffset += sizeof(SMB2_READ_RESPONSE_HEADER);
    pDataCursor += sizeof(SMB2_READ_RESPONSE_HEADER);

    pHeader->ulDataLength = ulBytesRead;
    pHeader->ulRemaining  = ulBytesRemaining;

    if (ulBytesAvailable < ulBytesRead)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader->usDataOffset = ulDataOffset;

    pHeader->usLength = ulBytesUsed + 1;

    if (pData)
    {
        memcpy(pDataCursor, pData, ulBytesRead);
        ulBytesUsed += ulBytesRead;
        ulBytesAvailable -= ulBytesRead;
        pDataCursor += ulBytesRead;
    }

    *pulDataOffset = ulDataOffset;
    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulDataOffset = 0;
    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalWriteRequest(
    IN     PNFS_MESSAGE_SMB_V2         pSmbRequest,
    IN OUT PSMB2_WRITE_REQUEST_HEADER* ppRequestHeader,
    IN OUT PBYTE*                      ppData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = pSmbRequest->ulHeaderSize;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    ULONG ulPacketSize = pSmbRequest->ulMessageSize;
    PSMB2_WRITE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE                      pData = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_WRITE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB2_WRITE_REQUEST_HEADER)pDataCursor;
    ulOffset += sizeof(SMB2_WRITE_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_WRITE_REQUEST_HEADER);

    if (pRequestHeader->usLength & 0x1)
    {
        if ((pRequestHeader->usDataOffset < ulOffset) ||
            (pRequestHeader->usDataOffset > ulPacketSize) ||
            (pRequestHeader->ulDataLength + pRequestHeader->usDataOffset) > ulPacketSize)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pData = pSmbRequest->pBuffer + pRequestHeader->usDataOffset;
    }

    if (pRequestHeader->ulDataLength && !pData)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRequestHeader = pRequestHeader;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalWriteResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN     ULONG  ulBytesWritten,
    IN     ULONG  ulBytesRemaining,
    IN OUT PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_WRITE_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesUsed = 0;
    PBYTE pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_WRITE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_WRITE_RESPONSE_HEADER)pDataCursor;
    ulBytesUsed += sizeof(SMB2_WRITE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_WRITE_RESPONSE_HEADER);
    pDataCursor += sizeof(SMB2_WRITE_RESPONSE_HEADER);

    pHeader->ulBytesWritten = ulBytesWritten;
    pHeader->ulBytesRemaining = ulBytesRemaining;

    pHeader->usLength = ulBytesUsed + 1;

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalLockRequest(
    IN     PNFS_MESSAGE_SMB_V2        pSmbRequest,
    IN OUT PSMB2_LOCK_REQUEST_HEADER* ppRequestHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = pSmbRequest->ulHeaderSize;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_LOCK_REQUEST_HEADER pRequestHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_LOCK_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader    = (PSMB2_LOCK_REQUEST_HEADER)pDataCursor;
    ulBytesAvailable -= sizeof(SMB2_LOCK_REQUEST_HEADER);
    ulOffset         += sizeof(SMB2_LOCK_REQUEST_HEADER);

    if (!pRequestHeader->usLockCount)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pRequestHeader->usLength != sizeof(SMB2_LOCK_REQUEST_HEADER))
    {
        USHORT usLocksRemaining = pRequestHeader->usLockCount - 1;

        if (!usLocksRemaining ||
            (ulBytesAvailable < (usLocksRemaining * sizeof(SMB2_LOCK))))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppRequestHeader = pRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalLockResponse(
    IN OUT PBYTE                  pBuffer,
    IN     ULONG                  ulOffset,
    IN     ULONG                  ulBytesAvailable,
    IN OUT PULONG                 pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_LOCK_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB2_LOCK_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_LOCK_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_LOCK_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_LOCK_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalIOCTLRequest(
    IN     PNFS_MESSAGE_SMB_V2         pSmbRequest,
    IN OUT PSMB2_IOCTL_REQUEST_HEADER* ppRequestHeader,
    IN OUT PBYTE*                      ppData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = pSmbRequest->ulHeaderSize;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    ULONG ulPacketSize = pSmbRequest->ulMessageSize;
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE                      pData = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_IOCTL_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB2_IOCTL_REQUEST_HEADER)pDataCursor;
    ulOffset += sizeof(SMB2_IOCTL_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_IOCTL_REQUEST_HEADER);

    if (pRequestHeader->usLength & 0x1)
    {
        if (((pRequestHeader->ulInOffset > 0) &&
             (pRequestHeader->ulInOffset < ulOffset)) ||
            ((pRequestHeader->ulOutOffset > 0) &&
             (pRequestHeader->ulOutOffset < ulOffset)) ||
            (pRequestHeader->ulInOffset > ulPacketSize) ||
            (pRequestHeader->ulOutOffset > ulPacketSize) ||
            ((pRequestHeader->ulInLength + pRequestHeader->ulInOffset) > ulPacketSize) ||
            ((pRequestHeader->ulOutLength + pRequestHeader->ulOutOffset) > ulPacketSize))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pData = pSmbRequest->pBuffer + pRequestHeader->ulInOffset;
    }

    if (pRequestHeader->ulInLength && !pData)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRequestHeader = pRequestHeader;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalIOCTLResponse(
    IN OUT PBYTE                      pBuffer,
    IN     ULONG                      ulOffset,
    IN     ULONG                      ulBytesAvailable,
    IN     PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    IN     PBYTE                      pOutBuffer,
    IN     ULONG                      ulOutLength,
    IN OUT PULONG                     pulBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_IOCTL_RESPONSE_HEADER pResponseHeader = NULL;
    ULONG ulBytesUsed = 0;
    ULONG ulDataOffset = ulOffset;
    PBYTE pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_IOCTL_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_IOCTL_RESPONSE_HEADER)pDataCursor;
    ulBytesUsed += sizeof(SMB2_IOCTL_RESPONSE_HEADER);
    ulDataOffset += sizeof(SMB2_IOCTL_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_IOCTL_RESPONSE_HEADER);
    pDataCursor += sizeof(SMB2_IOCTL_RESPONSE_HEADER);

    pResponseHeader->usLength       = ulBytesUsed + 1;
    pResponseHeader->ulFunctionCode = pRequestHeader->ulFunctionCode;
    pResponseHeader->fid            = pRequestHeader->fid;
    pResponseHeader->ulInOffset     = ulDataOffset;
    pResponseHeader->ulInLength     = 0;
    pResponseHeader->ulOutOffset    = ulDataOffset;
    pResponseHeader->ulOutLength    = ulOutLength;
    pResponseHeader->ulFlags        = pRequestHeader->ulFlags;

    // TODO: Check against max buffer size
    if (ulOutLength)
    {
        memcpy(pDataCursor, pOutBuffer, ulOutLength);
        ulBytesUsed += ulOutLength;
    }

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalFindRequest(
    IN     PNFS_MESSAGE_SMB_V2        pSmbRequest,
    IN OUT PSMB2_FIND_REQUEST_HEADER* ppRequestHeader,
    IN OUT PUNICODE_STRING            pwszFilename
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = pSmbRequest->ulHeaderSize;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    ULONG ulPacketSize = pSmbRequest->ulMessageSize;
    PSMB2_FIND_REQUEST_HEADER pHeader = NULL; // Do not free
    UNICODE_STRING wszFilename = {0}; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_FIND_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_FIND_REQUEST_HEADER)pDataCursor;

    ulBytesAvailable -= sizeof(SMB2_FIND_REQUEST_HEADER);
    ulOffset += sizeof(SMB2_FIND_REQUEST_HEADER);

    if (pHeader->usLength & 0x1)
    {
        if ((pHeader->usFilenameOffset < ulOffset) ||
            (pHeader->usFilenameOffset % 2) ||
            (pHeader->usFilenameOffset % 2) ||
            (pHeader->usFilenameOffset > ulPacketSize) ||
            (pHeader->usFilenameLength + pHeader->usFilenameOffset) > ulPacketSize)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        wszFilename.Buffer = (PWSTR)(pSmbRequest->pBuffer + pHeader->usFilenameOffset);
        wszFilename.Length = wszFilename.MaximumLength = pHeader->usFilenameLength;
    }

    if (!wszFilename.Length)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRequestHeader = pHeader;
    *pwszFilename = wszFilename;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalOplockBreakRequest(
    IN     PNFS_MESSAGE_SMB_V2        pSmbRequest,
    IN OUT PSMB2_OPLOCK_BREAK_HEADER* ppRequestHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_OPLOCK_BREAK_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_OPLOCK_BREAK_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_OPLOCK_BREAK_HEADER)pDataCursor;

    if (pHeader->usLength != sizeof(SMB2_OPLOCK_BREAK_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRequestHeader = pHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalFindResponse(
    IN OUT PBYTE                       pBuffer,
    IN     ULONG                       ulOffset,
    IN     ULONG                       ulBytesAvailable,
    IN OUT PBYTE                       pData,
    IN     ULONG                       ulDataLength,
    IN OUT PULONG                      pulDataOffset,
    IN OUT PSMB2_FIND_RESPONSE_HEADER* ppHeader,
    IN OUT PULONG                      pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_FIND_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    ULONG  ulBytesUsed = 0;
    ULONG  ulDataOffset = ulOffset;
    PBYTE  pOutBufferRef = pBuffer;
    PBYTE  pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_FIND_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_FIND_RESPONSE_HEADER)pDataCursor;
    ulBytesUsed += sizeof(SMB2_FIND_RESPONSE_HEADER);
    ulDataOffset += sizeof(SMB2_FIND_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_FIND_RESPONSE_HEADER);
    pDataCursor += sizeof(SMB2_FIND_RESPONSE_HEADER);

    pResponseHeader->usLength = sizeof(SMB2_FIND_RESPONSE_HEADER) + 1;

    if (ulDataOffset % 8)
    {
        USHORT usAlign =  8 - (ulDataOffset % 8);

        if (ulBytesAvailable < ulDataLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulBytesUsed += usAlign;
        ulDataOffset += usAlign;
        ulBytesAvailable -= usAlign;
        pDataCursor += usAlign;
    }

    // TODO: Check against max buffer size
    if (pData)
    {
        if (ulBytesAvailable < ulDataLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pData, ulDataLength);

        ulBytesUsed += ulDataLength;
        // ulBytesAvailable -= ulDataLength;
        // pDataCursor += ulDataLength;
    }

    *pulDataOffset = ulDataOffset;
    *ppHeader = pResponseHeader;
    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulDataOffset = 0;
    *ppHeader = NULL;
    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pOutBufferRef, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalNotifyRequest(
    IN     PNFS_MESSAGE_SMB_V2         pSmbRequest,
    IN OUT PSMB2_NOTIFY_CHANGE_HEADER* ppNotifyRequestHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pDataCursor = pSmbRequest->pBuffer + pSmbRequest->ulHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->ulHeaderSize;
    PSMB2_NOTIFY_CHANGE_HEADER pNotifyRequestHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_NOTIFY_CHANGE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNotifyRequestHeader = (PSMB2_NOTIFY_CHANGE_HEADER)pDataCursor;

    if (pNotifyRequestHeader->usLength != sizeof(SMB2_NOTIFY_CHANGE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppNotifyRequestHeader = pNotifyRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppNotifyRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalNotifyResponse(
    IN OUT PBYTE                         pBuffer,
    IN     ULONG                         ulOffset,
    IN     ULONG                         ulBytesAvailable,
    IN OUT PBYTE                         pData,
    IN     ULONG                         ulDataLength,
    IN OUT PULONG                        pulDataOffset,
    IN OUT PSMB2_NOTIFY_RESPONSE_HEADER* ppHeader,
    IN OUT PULONG                        pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_NOTIFY_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    ULONG  ulBytesUsed = 0;
    ULONG  ulDataOffset = ulOffset;
    PBYTE  pOutBufferRef = pBuffer;
    PBYTE  pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_NOTIFY_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_NOTIFY_RESPONSE_HEADER)pDataCursor;
    ulBytesUsed      += sizeof(SMB2_NOTIFY_RESPONSE_HEADER);
    ulDataOffset     += sizeof(SMB2_NOTIFY_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_NOTIFY_RESPONSE_HEADER);
    pDataCursor      += sizeof(SMB2_NOTIFY_RESPONSE_HEADER);

    pResponseHeader->usLength = sizeof(SMB2_NOTIFY_RESPONSE_HEADER) + 1;

    if (ulDataOffset % 8)
    {
        USHORT usAlign =  8 - (ulDataOffset % 8);

        if (ulBytesAvailable < ulDataLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulBytesUsed      += usAlign;
        ulDataOffset     += usAlign;
        ulBytesAvailable -= usAlign;
        pDataCursor      += usAlign;
    }

    // TODO: Check against max buffer size
    if (pData)
    {
        if (ulBytesAvailable < ulDataLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pData, ulDataLength);

        ulBytesUsed += ulDataLength;
        // ulBytesAvailable -= ulDataLength;
        // pDataCursor += ulDataLength;
    }


    pResponseHeader->ulOutBufferLength = ulDataLength;
    pResponseHeader->usOutBufferOffset = (USHORT)ulDataOffset;

    *pulDataOffset = ulDataOffset;
    *ppHeader      = pResponseHeader;
    *pulBytesUsed  = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulDataOffset = 0;
    *ppHeader = NULL;
    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pOutBufferRef, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2MarshalError(
    IN OUT PBYTE    pBuffer,
    IN     ULONG    ulOffset,
    IN     ULONG    ulBytesAvailable,
    IN     PBYTE    pMessage,
    IN     ULONG    ulMessageLength,
    IN OUT PULONG   pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_ERROR_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesUsed = 0;
    PBYTE pDataCursor = pBuffer;

    if (ulBytesAvailable < sizeof(SMB2_ERROR_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_ERROR_RESPONSE_HEADER)pDataCursor;
    ulBytesUsed += sizeof(SMB2_ERROR_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_ERROR_RESPONSE_HEADER);
    pDataCursor += sizeof(SMB2_ERROR_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    if (ulMessageLength)
    {
        if (ulBytesAvailable < ulMessageLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulBytesUsed += ulMessageLength;
        pHeader->usLength++;

        pHeader->ulByteCount = ulMessageLength;
        memcpy(pDataCursor, pMessage, ulMessageLength);
    }
    else
    {
        if (ulBytesAvailable < 1)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulBytesUsed++;
        pHeader->usLength++;

        pHeader->ulByteCount = 0;
    }

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (ulBytesUsed)
    {
        memset(pBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SMB2MarshalFooter(
    PSMB_PACKET pPacket
    )
{
    pPacket->pNetBIOSHeader->len = htonl(pPacket->bufferUsed - sizeof(NETBIOS_HEADER));

    return 0;
}
