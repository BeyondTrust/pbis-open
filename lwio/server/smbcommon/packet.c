/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        session_setup.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Packet Marshalling
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

static uchar8_t smbMagic[4] = { 0xFF, 'S', 'M', 'B' };

static
NTSTATUS
SMBPacketAllocatePooled(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT PSMB_PACKET* ppPacket
    );

static
VOID
SMBPacketReleasePooled(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN OUT PSMB_PACKET        pPacket
    );

static
NTSTATUS
SMBPacketBufferAllocatePooled(
    IN  PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN  size_t                 len,
    OUT uint8_t**              ppBuffer,
    OUT size_t*                pAllocatedLen
    );

static
VOID
SMBPacketBufferFreePooled(
    IN  PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT uint8_t*               pBuffer,
    IN  size_t                 bufferLen
    );

BOOLEAN
SMBIsAndXCommand(
    uint8_t command
    )
{
    BOOLEAN bResult = FALSE;

    switch(command)
    {
        case COM_LOCKING_ANDX:
        case COM_OPEN_ANDX:
        case COM_READ_ANDX:
        case COM_WRITE_ANDX:
        case COM_SESSION_SETUP_ANDX:
        case COM_LOGOFF_ANDX:
        case COM_TREE_CONNECT_ANDX:
        case COM_NT_CREATE_ANDX:
        {
            bResult = TRUE;
            break;
        }
    }

    return bResult;
}

NTSTATUS
SMBPacketCreateAllocator(
    IN ULONG ulNumMaxPackets,
    OUT PLWIO_PACKET_ALLOCATOR* ppPacketAllocator
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_PACKET_ALLOCATOR pPacketAllocator = NULL;

    ntStatus = LwIoAllocateMemory(
                    sizeof(LWIO_PACKET_ALLOCATOR),
                    (PVOID*)&pPacketAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pPacketAllocator->mutex, NULL);
    pPacketAllocator->pMutex = &pPacketAllocator->mutex;
    pPacketAllocator->ulNumMaxPackets = ulNumMaxPackets;

    *ppPacketAllocator = pPacketAllocator;

cleanup:

    return ntStatus;

error:

    *ppPacketAllocator = NULL;

    goto cleanup;
}

NTSTATUS
SMBPacketAllocate(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT PSMB_PACKET* ppPacket
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pPacket = NULL;

    if (pPacketAllocator)
    {
        ntStatus = SMBPacketAllocatePooled(pPacketAllocator, &pPacket);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = LwIoAllocateMemory(sizeof(SMB_PACKET), (PVOID *) &pPacket);
        BAIL_ON_NT_STATUS(ntStatus);

        InterlockedIncrement(&pPacket->refCount);
    }

    *ppPacket = pPacket;

cleanup:

    return ntStatus;

error:

    *ppPacket = NULL;

    goto cleanup;
}

static
NTSTATUS
SMBPacketAllocatePooled(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT PSMB_PACKET* ppPacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pPacket = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    if (pPacketAllocator->pFreePacketStack)
    {
        pPacket = (PSMB_PACKET) pPacketAllocator->pFreePacketStack;

        SMBStackPopNoFree(&pPacketAllocator->pFreePacketStack);

        pPacketAllocator->freePacketCount--;

        LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

        memset(pPacket, 0, sizeof(SMB_PACKET));
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

        ntStatus = LwIoAllocateMemory(sizeof(SMB_PACKET), (PVOID *) &pPacket);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    InterlockedIncrement(&pPacket->refCount);

    *ppPacket = pPacket;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    return ntStatus;

error:

    *ppPacket = NULL;

    goto cleanup;
}

VOID
SMBPacketRelease(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN OUT PSMB_PACKET        pPacket
    )
{
    if (pPacketAllocator)
    {
        SMBPacketReleasePooled(pPacketAllocator, pPacket);
    }
    else if (InterlockedDecrement(&pPacket->refCount) == 0)
    {
        if (pPacket->pRawBuffer)
        {
            SMBPacketBufferFree(
                    pPacketAllocator,
                    pPacket->pRawBuffer,
                    pPacket->bufferLen);
        }

        LwIoFreeMemory(pPacket);
    }
}

static
VOID
SMBPacketReleasePooled(
    IN PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN OUT PSMB_PACKET        pPacket
    )
{
    if (InterlockedDecrement(&pPacket->refCount) == 0)
    {
        BOOLEAN bInLock = FALSE;

        if (pPacket->pRawBuffer)
        {
            SMBPacketBufferFree(
                pPacketAllocator,
                pPacket->pRawBuffer,
                pPacket->bufferLen);

            pPacket->pRawBuffer = NULL;
            pPacket->bufferLen = 0;
        }

        LWIO_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

        /* If the len is greater than our current allocator len, adjust */
        /* @todo: make free list configurable */
        if (pPacketAllocator->freePacketCount < pPacketAllocator->ulNumMaxPackets)
        {
            assert(sizeof(SMB_PACKET) > sizeof(SMB_STACK));
            SMBStackPushNoAlloc(&pPacketAllocator->pFreePacketStack,
                                (PSMB_STACK) pPacket);
            pPacketAllocator->freePacketCount++;
        }
        else
        {
            LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

            LwIoFreeMemory(pPacket);
        }

        LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);
    }
}

NTSTATUS
SMBPacketBufferAllocate(
    IN  PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN  size_t                 len,
    OUT uint8_t**              ppBuffer,
    OUT size_t*                pAllocatedLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE    pBuffer = NULL;
    size_t   allocatedLen = len;

    if (pPacketAllocator)
    {
        ntStatus = SMBPacketBufferAllocatePooled(
                        pPacketAllocator,
                        len,
                        &pBuffer,
                        &allocatedLen);
    }
    else
    {
        //
        // Note: The resultant buffer will not be cleared
        //
        ntStatus = LwIoAllocateBuffer(len, (PVOID *) &pBuffer);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppBuffer = pBuffer;
    *pAllocatedLen = allocatedLen;

cleanup:

    return ntStatus;

error:

    *ppBuffer = NULL;
    *pAllocatedLen = 0;

    goto cleanup;
}

static
NTSTATUS
SMBPacketBufferAllocatePooled(
    IN  PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    IN  size_t                 len,
    OUT uint8_t**              ppBuffer,
    OUT size_t*                pAllocatedLen
    )
{
    NTSTATUS ntStatus = 0;
    uint8_t* pBuffer = NULL;
    size_t   allocatedLen  = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    /* If the len is greater than our current allocator len, adjust */
    if (len > pPacketAllocator->freeBufferLen)
    {
        SMBStackFree(pPacketAllocator->pFreeBufferStack);

        pPacketAllocator->pFreeBufferStack = NULL;
        pPacketAllocator->freeBufferLen = len;
    }

    if (pPacketAllocator->pFreeBufferStack)
    {
        pBuffer = (uint8_t *) pPacketAllocator->pFreeBufferStack;
        allocatedLen = pPacketAllocator->freeBufferLen;

        SMBStackPopNoFree(&pPacketAllocator->pFreeBufferStack);

        pPacketAllocator->freeBufferCount--;

        LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

        // memset(pBuffer, 0, allocatedLen);
    }
    else
    {
        allocatedLen = pPacketAllocator->freeBufferLen;

        LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

        ntStatus = LwIoAllocateBuffer(allocatedLen, (PVOID *) &pBuffer);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppBuffer = pBuffer;
    *pAllocatedLen = allocatedLen;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    return ntStatus;

error:

    *ppBuffer = NULL;
    *pAllocatedLen = 0;

    goto cleanup;
}

VOID
SMBPacketBufferFree(
    IN  PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT uint8_t*               pBuffer,
    IN  size_t                 bufferLen
    )
{
    if (pPacketAllocator)
    {
        SMBPacketBufferFreePooled(pPacketAllocator, pBuffer, bufferLen);
    }
    else
    {
        LwIoFreeBuffer(pBuffer);
    }
}

static
VOID
SMBPacketBufferFreePooled(
    IN  PLWIO_PACKET_ALLOCATOR pPacketAllocator,
    OUT uint8_t*               pBuffer,
    IN  size_t                 bufferLen
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    /* If the len is greater than our current allocator len, adjust */
    /* @todo: make free list configurable */
    if (bufferLen == pPacketAllocator->freeBufferLen &&
        pPacketAllocator->freeBufferCount < pPacketAllocator->ulNumMaxPackets)
    {
        assert(bufferLen > sizeof(SMB_STACK));

        SMBStackPushNoAlloc(&pPacketAllocator->pFreeBufferStack, (PSMB_STACK) pBuffer);

        pPacketAllocator->freeBufferCount++;
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

        LwIoFreeBuffer(pBuffer);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);
}

VOID
SMBPacketResetBuffer(
    PSMB_PACKET pPacket
    )
{
    if (pPacket->pRawBuffer && pPacket->bufferLen)
    {
        memset(pPacket->pRawBuffer, 0, pPacket->bufferLen);
    }
}

VOID
SMBPacketFreeAllocator(
    IN OUT PLWIO_PACKET_ALLOCATOR pPacketAllocator
    )
{
    if (pPacketAllocator->pMutex)
    {
        pthread_mutex_destroy(&pPacketAllocator->mutex);
        pPacketAllocator->pMutex = NULL;
    }

    if (pPacketAllocator->pFreeBufferStack)
    {
        SMBStackFree(pPacketAllocator->pFreeBufferStack);
    }

    if (pPacketAllocator->pFreePacketStack)
    {
        SMBStackFree(pPacketAllocator->pFreePacketStack);
    }

    LwIoFreeMemory(pPacketAllocator);
}

static
NTSTATUS
ConsumeBuffer(
    IN PVOID pBuffer,
    IN uint32_t BufferLength,
    IN OUT uint32_t* BufferLengthUsed,
    IN uint32_t BytesNeeded
    )
{
    NTSTATUS ntStatus = 0;

    if (*BufferLengthUsed + BytesNeeded > BufferLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        *BufferLengthUsed += BytesNeeded;
    }

    return ntStatus;
}

VOID
SMBPacketHTOLSmbHeader(
    IN OUT SMB_HEADER* pHeader
    )
{
    SMB_HTOL32_INPLACE(pHeader->error);
    SMB_HTOL16_INPLACE(pHeader->flags2);
    SMB_HTOL16_INPLACE(pHeader->extra.pidHigh);
    SMB_HTOL16_INPLACE(pHeader->tid);
    SMB_HTOL16_INPLACE(pHeader->pid);
    SMB_HTOL16_INPLACE(pHeader->uid);
    SMB_HTOL16_INPLACE(pHeader->mid);
}

static
VOID
SMBPacketLTOHSmbHeader(
    IN OUT SMB_HEADER* pHeader
    )
{
    SMB_LTOH32_INPLACE(pHeader->error);
    SMB_LTOH16_INPLACE(pHeader->flags2);
    SMB_LTOH16_INPLACE(pHeader->extra.pidHigh);
    SMB_LTOH16_INPLACE(pHeader->tid);
    SMB_LTOH16_INPLACE(pHeader->pid);
    SMB_LTOH16_INPLACE(pHeader->uid);
    SMB_LTOH16_INPLACE(pHeader->mid);
}

/* @todo: support AndX */
NTSTATUS
SMBPacketMarshallHeader(
    uint8_t    *pBuffer,
    uint32_t    bufferLen,
    uint8_t     command,
    uint32_t    error,
    uint32_t    isResponse,
    uint16_t    tid,
    uint32_t    pid,
    uint16_t    uid,
    uint16_t    mid,
    BOOLEAN     bCommandAllowsSignature,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t bufferUsed = 0;
    SMB_HEADER* pHeader = NULL;

    pPacket->allowSignature = bCommandAllowsSignature;

    pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) (pBuffer + bufferUsed);

    ntStatus = ConsumeBuffer(pBuffer, bufferLen, &bufferUsed, sizeof(NETBIOS_HEADER));
    BAIL_ON_NT_STATUS(ntStatus);

    pPacket->protocolVer = SMB_PROTOCOL_VERSION_1;

    pPacket->pSMBHeader = (SMB_HEADER *) (pBuffer + bufferUsed);

    ntStatus = ConsumeBuffer(pBuffer, bufferLen, &bufferUsed, sizeof(SMB_HEADER));
    BAIL_ON_NT_STATUS(ntStatus);

    pHeader = pPacket->pSMBHeader;
    memcpy(&pHeader->smb, smbMagic, sizeof(smbMagic));
    pHeader->command = command;
    pHeader->error = error;
    pHeader->flags = isResponse ? FLAG_RESPONSE : 0;
    pHeader->flags |= FLAG_CASELESS_PATHS | FLAG_OBSOLETE_2;
    pHeader->flags2 = ((isResponse ? 0 : FLAG2_KNOWS_LONG_NAMES) |
                       (isResponse ? 0 : FLAG2_IS_LONG_NAME) |
                       FLAG2_KNOWS_EAS | FLAG2_EXT_SEC |
                       FLAG2_ERR_STATUS | FLAG2_UNICODE);
    memset(pHeader->pad, 0, sizeof(pHeader->pad));
    pHeader->extra.pidHigh = pid >> 16;
    pHeader->tid = tid;
    pHeader->pid = pid;
    pHeader->uid = uid;
    pHeader->mid = mid;

    if (SMBIsAndXCommand(command))
    {
        pPacket->pAndXHeader = (ANDX_HEADER *) (pBuffer + bufferUsed);

        ntStatus = ConsumeBuffer(pBuffer, bufferLen, &bufferUsed, sizeof(ANDX_HEADER));
        BAIL_ON_NT_STATUS(ntStatus);

        pPacket->pAndXHeader->andXCommand = 0xFF;
        pPacket->pAndXHeader->andXOffset = 0;
        pPacket->pAndXHeader->andXReserved = 0;
    }
    else
    {
        pPacket->pAndXHeader = NULL;
    }

    pPacket->pParams = pBuffer + bufferUsed;
    pPacket->pData = NULL;
    pPacket->bufferLen = bufferLen;
    pPacket->bufferUsed = bufferUsed;

    assert(bufferUsed <= bufferLen);

cleanup:
    return ntStatus;

error:
    pPacket->pNetBIOSHeader = NULL;
    pPacket->pSMBHeader = NULL;
    pPacket->pAndXHeader = NULL;
    pPacket->pParams = NULL;
    pPacket->pData = NULL;
    pPacket->bufferLen = bufferLen;
    pPacket->bufferUsed = 0;

    goto cleanup;
}

NTSTATUS
SMBPacketMarshallFooter(
    PSMB_PACKET pPacket
    )
{
    if (pPacket->bufferUsed > sizeof(NETBIOS_HEADER))
    {
        pPacket->pNetBIOSHeader->len = htonl(pPacket->bufferUsed - sizeof(NETBIOS_HEADER));
    }
    else
    {
        pPacket->pNetBIOSHeader->len = 0;
    }

    return 0;
}

BOOLEAN
SMBPacketIsSigned(
    PSMB_PACKET pPacket
    )
{
    return (pPacket->pSMBHeader->flags2 & FLAG2_SECURITY_SIG);
}

BOOLEAN
SMB2PacketIsSigned(
    PSMB_PACKET pPacket
    )
{
    return (pPacket->pSMB2Header->ulFlags & SMB2_FLAGS_SIGNED);
}

NTSTATUS
SMBPacketVerifySignature(
    PSMB_PACKET pPacket,
    ULONG       ulExpectedSequence,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;
    uint8_t digest[16];
    uint8_t origSignature[8];
    MD5_CTX md5Value;
    uint32_t littleEndianSequence = SMB_HTOL32(ulExpectedSequence);

    assert (sizeof(origSignature) == sizeof(pPacket->pSMBHeader->extra.securitySignature));

    memcpy(origSignature,
           pPacket->pSMBHeader->extra.securitySignature,
           sizeof(pPacket->pSMBHeader->extra.securitySignature));

    memset(&pPacket->pSMBHeader->extra.securitySignature[0],
           0,
           sizeof(pPacket->pSMBHeader->extra.securitySignature));

    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0],
           &littleEndianSequence,
           sizeof(littleEndianSequence));

    MD5_Init(&md5Value);

    if (pSessionKey)
    {
        MD5_Update(&md5Value, pSessionKey, ulSessionKeyLength);
    }

    MD5_Update(&md5Value,
               (PBYTE)pPacket->pSMBHeader,
               pPacket->pNetBIOSHeader->len);
    MD5_Final(digest, &md5Value);

    if (memcmp(&origSignature[0], &digest[0], sizeof(origSignature)))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
    }

    // restore signature
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0],
           &origSignature[0],
           sizeof(origSignature));

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    LWIO_LOG_WARNING("SMB Packet verification failed (status = 0x%08X)", ntStatus);

    goto cleanup;
}

NTSTATUS
SMB2PacketVerifySignature(
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

            if (pHeader->ulChainOffset)
            {
                if (ulBytesAvailable < pHeader->ulChainOffset)
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ulPacketSize = pHeader->ulChainOffset;
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

            if (pHeader->ulChainOffset)
            {
                pBuffer          += pHeader->ulChainOffset;
                ulBytesAvailable -= pHeader->ulChainOffset;
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
SMBPacketDecodeHeader(
    IN OUT PSMB_PACKET pPacket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN OPTIONAL PBYTE pSessionKey,
    IN DWORD dwSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t packetStatus = 0;

    if (bVerifySignature)
    {
        ntStatus = SMBPacketVerifySignature(
                        pPacket,
                        dwExpectedSequence,
                        pSessionKey,
                        dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SMBPacketLTOHSmbHeader(pPacket->pSMBHeader);

    packetStatus = pPacket->pSMBHeader->error;
    if (!LW_NT_SUCCESS_OR_NOT(packetStatus) && STATUS_PENDING != packetStatus)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:
    return ntStatus;
}

NTSTATUS
SMBPacketSign(
    PSMB_PACKET pPacket,
    ULONG       ulSequence,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;
    uint8_t digest[16];
    MD5_CTX md5Value;
    uint32_t littleEndianSequence = SMB_HTOL32(ulSequence);

    memset(&pPacket->pSMBHeader->extra.securitySignature[0],
           0,
           sizeof(pPacket->pSMBHeader->extra.securitySignature));

    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0],
           &littleEndianSequence,
           sizeof(littleEndianSequence));

    MD5_Init(&md5Value);

    if (pSessionKey)
    {
        MD5_Update(&md5Value, pSessionKey, ulSessionKeyLength);
    }

    MD5_Update(&md5Value, (PBYTE)pPacket->pSMBHeader, ntohl(pPacket->pNetBIOSHeader->len));
    MD5_Final(digest, &md5Value);

    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0],
           &digest[0],
           sizeof(pPacket->pSMBHeader->extra.securitySignature));

    return ntStatus;
}

NTSTATUS
SMB2PacketSign(
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

            if (pHeader->ulChainOffset)
            {
                if (ulBytesAvailable < pHeader->ulChainOffset)
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ulPacketSize = pHeader->ulChainOffset;
            }

            pHeader->ulFlags |= SMB2_FLAGS_SIGNED;

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

            if (pHeader->ulChainOffset)
            {
                pBuffer          += pHeader->ulChainOffset;
                ulBytesAvailable -= pHeader->ulChainOffset;
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
SMBPacketUpdateAndXOffset(
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!SMBIsAndXCommand(pPacket->pSMBHeader->command)) {
        /* No op */
        return STATUS_SUCCESS;
    }

    pPacket->pAndXHeader->andXOffset = pPacket->bufferUsed - sizeof(NETBIOS_HEADER);

    return ntStatus;
}

NTSTATUS
SMBPacketAppendUnicodeString(
    OUT PBYTE pBuffer,
    IN ULONG BufferLength,
    IN OUT PULONG BufferUsed,
    IN PCWSTR pwszString
    )
{
    NTSTATUS ntStatus = 0;
    ULONG bytesNeeded = 0;
    ULONG bufferUsed = *BufferUsed;
    wchar16_t* pOutputBuffer = NULL;
    size_t writeLength = 0;

    bytesNeeded = sizeof(pwszString[0]) * (wc16slen(pwszString) + 1);
    if (bufferUsed + bytesNeeded > BufferLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOutputBuffer = (wchar16_t *) (pBuffer + bufferUsed);
    writeLength = wc16stowc16les(pOutputBuffer,
                                 pwszString,
                                 bytesNeeded / sizeof(pwszString[0]));

    // Verify that expected write length was returned.  Note that the
    // returned length does not include the NULL though the NULL gets
    // written out.
    if (writeLength == (size_t) -1)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if (((writeLength + 1) * sizeof(wchar16_t)) != bytesNeeded)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    bufferUsed += bytesNeeded;

error:
    *BufferUsed = bufferUsed;
    return ntStatus;
}

NTSTATUS
SMBPacketAppendString(
    OUT PBYTE pBuffer,
    IN ULONG BufferLength,
    IN OUT PULONG BufferUsed,
    IN PCSTR pszString
    )
{
    NTSTATUS ntStatus = 0;
    ULONG bytesNeeded = 0;
    ULONG bufferUsed = *BufferUsed;
    char* pOutputBuffer = NULL;
    char* pszCursor = NULL;

    bytesNeeded = sizeof(pszString[0]) * (strlen(pszString) + 1);
    if (bufferUsed + bytesNeeded > BufferLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOutputBuffer = (char *) (pBuffer + bufferUsed);

    pszCursor = lsmb_stpncpy(pOutputBuffer, pszString, bytesNeeded / sizeof(pszString[0]));
    *pszCursor = 0;
    if ((pszCursor - pOutputBuffer) != (bytesNeeded - 1))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    bufferUsed += bytesNeeded;

error:
    *BufferUsed = bufferUsed;
    return ntStatus;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
