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
 *        socket.c
 *
 * Abstract:
 *
 *        Likewise Rdr Subsystem (LWIO)
 *
 *        Common Socket Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrEcho(
    PRDR_SOCKET pSocket,
    PCSTR pszMessage
    );

static
NTSTATUS
RdrSocketAcquireMid(
    PRDR_SOCKET pSocket,
    USHORT* pusMid
    );

static
VOID
RdrSocketInvalidate_InLock(
    PRDR_SOCKET pSocket,
    NTSTATUS status
    );

static
NTSTATUS
RdrSocketReceivePacket(
    IN PRDR_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
    );

static
NTSTATUS
RdrSocketRead(
    PRDR_SOCKET pSocket,
    uint8_t    *buffer,
    uint32_t    dwLen,
    uint32_t   *actualLen
    );

static
int
RdrSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
RdrSocketHashSessionByUID(
    PCVOID vp
    );

static
size_t
RdrSocketHashSession2ById(
    PCVOID vp
    );

static
int
RdrSocketHashSession2CompareById(
    PCVOID vp1,
    PCVOID vp2
    );

static
int
RdrSocketHashSessionCompareByKey(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
RdrSocketHashSessionByKey(
    PCVOID vp
    );

static
NTSTATUS
RdrSocketSendData(
    IN PRDR_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    );

static
VOID
RdrSocketTask(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    );

static
NTSTATUS
RdrSocketDispatchPacket(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

static
VOID
RdrSocketFreeContents(
    PRDR_SOCKET pSocket
    );

static
VOID
RdrSocketFree(
    PRDR_SOCKET pSocket
    );

static
NTSTATUS
RdrTransceiveEcho(
    PRDR_OP_CONTEXT pContext,
    PRDR_SOCKET pSocket,
    PCSTR pszMessage
    );

static
NTSTATUS
RdrTransceiveEcho2(
    PRDR_OP_CONTEXT pContext,
    PRDR_SOCKET pSocket
    );

static
BOOLEAN
RdrEchoComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pData
    );

static
NTSTATUS
RdrSocketDispatchPacket1(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

static
NTSTATUS
RdrSocketDispatchPacket2(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

static
USHORT
RdrSocketCreditsNeeded(
    PRDR_SOCKET pSocket
    );

NTSTATUS
RdrSocketCreate(
    IN PCWSTR pwszHostname,
    OUT PRDR_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    RDR_SOCKET *pSocket = NULL;
    BOOLEAN bDestroyMutex = FALSE;

    ntStatus = LwIoAllocateMemory(
                sizeof(RDR_SOCKET),
                (PVOID*)&pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pSocket->PendingSend);
    LwListInit(&pSocket->PendingResponse);
    LwListInit(&pSocket->StateWaiters);

    pSocket->fd = -1;

    pthread_mutex_init(&pSocket->mutex, NULL);
    bDestroyMutex = TRUE;

    /* Assume SMBv1 to start */
    pSocket->version = SMB_PROTOCOL_VERSION_1;
    pSocket->refCount = 1;

    /* Hostname is trusted */
    ntStatus = LwRtlWC16StringDuplicate(&pSocket->pwszHostname, pwszHostname);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->ulMaxTransactSize = 0;
    pSocket->maxRawSize = 0;
    pSocket->sessionKey = 0;
    pSocket->capabilities = 0;
    pSocket->pSecurityBlob = NULL;
    pSocket->securityBlobLen = 0;
    pSocket->usMaxSlots = 1;
    pSocket->usUsedSlots = 0;

    ntStatus = SMBHashCreate(
                    19,
                    RdrSocketHashSessionCompareByKey,
                    RdrSocketHashSessionByKey,
                    NULL,
                    &pSocket->pSessionHashByPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTask(
        gRdrRuntime.pThreadPool,
        &pSocket->pTask,
        gRdrRuntime.pSocketTaskGroup,
        RdrSocketTask,
        pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSocket = pSocket;

cleanup:

    return ntStatus;

error:

    if (pSocket)
    {
        SMBHashSafeFree(&pSocket->pSessionHashByUID);
        SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);
        LWIO_SAFE_FREE_MEMORY(pSocket->pwszHostname);
        LWIO_SAFE_FREE_MEMORY(pSocket->pwszCanonicalName);

        if (bDestroyMutex)
        {
            pthread_mutex_destroy(&pSocket->mutex);
        }

        LwIoFreeMemory(pSocket);
    }

    *ppSocket = NULL;

    goto cleanup;
}

NTSTATUS
RdrSocketSetProtocol(
    PRDR_SOCKET pSocket,
    SMB_PROTOCOL_VERSION version
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (version)
    {
    case SMB_PROTOCOL_VERSION_1:
        status = SMBHashCreate(
            19,
            RdrSocketHashSessionCompareByUID,
            RdrSocketHashSessionByUID,
            NULL,
            &pSocket->pSessionHashByUID);
        BAIL_ON_NT_STATUS(status);
        break;
    case SMB_PROTOCOL_VERSION_2:
        status = SMBHashCreate(
            19,
            RdrSocketHashSession2CompareById,
            RdrSocketHashSession2ById,
            NULL,
            &pSocket->pSessionHashByUID);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    pSocket->version = version;

error:

    return status;
}


static
size_t
RdrSocketHashSessionByUID(
    PCVOID vp
    )
{
   return *((uint16_t *) vp);
}

static
int
RdrSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    )
{
    uint16_t uid1 = *((uint16_t *) vp1);
    uint16_t uid2 = *((uint16_t *) vp2);

    if (uid1 == uid2)
    {
        return 0;
    }
    else if (uid1 > uid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
RdrSocketHashSession2ById(
    PCVOID vp
    )
{
   return (size_t) *(PULONG64) vp;
}

static
int
RdrSocketHashSession2CompareById(
    PCVOID vp1,
    PCVOID vp2
    )
{
    ULONG64 id1 = *(PULONG64) vp1;
    ULONG64 id2 = *(PULONG64) vp2;

    if (id1 == id2)
    {
        return 0;
    }
    else if (id1 > id2)
    {
        return 1;
    }

    return -1;
}


static
int
RdrSocketHashSessionCompareByKey(
    PCVOID vp1,
    PCVOID vp2
    )
{
    const struct _RDR_SESSION_KEY* pKey1 = (struct _RDR_SESSION_KEY*) vp1;
    const struct _RDR_SESSION_KEY* pKey2 = (struct _RDR_SESSION_KEY*) vp2;
    
    return !(pKey1->uid == pKey2->uid &&
             !strcmp(pKey1->pszPrincipal, pKey2->pszPrincipal) &&
             pKey1->VerifierLength == pKey2->VerifierLength &&
             !memcmp(pKey1->pVerifier, pKey2->pVerifier, pKey1->VerifierLength));
}

static
size_t
RdrSocketHashSessionByKey(
    PCVOID vp
    )
{
    const struct _RDR_SESSION_KEY* pKey = (struct _RDR_SESSION_KEY*) vp;
    size_t hash = SMBHashCaselessString(pKey->pszPrincipal) ^ (pKey->uid ^ (pKey->uid << 16));
    ULONG index = 0;

    for (index = 0; index < pKey->VerifierLength; index++)
    {
        hash = hash * 31 + pKey->pVerifier[index];
    }

    return hash;
}

static
BOOLEAN
RdrSocketIsSignatureRequired(
    IN PRDR_SOCKET pSocket
    )
{
    BOOLEAN bIsRequired = FALSE;
        
    /* We need to sign outgoing packets if we negotiated it and the
       socket is in the ready state -- that is, we have completed the
       negotiate stage.  This causes the initial session setup packet to
       be signed, even though we do not yet have a session key.  This results
       in signing with a zero-length key, which matches Windows behavior.
       Note that no known SMB server actually verifies this signature since it
       is meaningless and probably the result of an implementation quirk. */
    if (pSocket->state == RDR_SOCKET_STATE_READY &&
        (pSocket->ucSecurityMode & SECURITY_MODE_SIGNED_MESSAGES_REQUIRED ||
         (pSocket->ucSecurityMode & SECURITY_MODE_SIGNED_MESSAGES_SUPPORTED && gRdrRuntime.config.bSigningEnabled)))
    {
        bIsRequired = TRUE;
    }

    return bIsRequired;
}

static
ULONG
RdrSocketGetNextSequence_inlock(
    PRDR_SOCKET pSocket
    )
{
    DWORD dwSequence = 0;

    dwSequence = pSocket->dwSequence;
    // Next for response
    // Next for next message
    pSocket->dwSequence += 2; 

    return dwSequence;
}

VOID
RdrSocketBeginSequence(
    PRDR_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);
    
    pSocket->dwSequence = 2;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
NTSTATUS
RdrSocketSendData(
    IN PRDR_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t  writtenLen = 0;

    do
    {
        writtenLen = write(
            pSocket->fd,
            pPacket->pRawBuffer + pSocket->OutgoingWritten,
            pPacket->bufferUsed - pSocket->OutgoingWritten);
        if (writtenLen >= 0)
        {
            pSocket->OutgoingWritten += writtenLen;
        }
    } while (pSocket->OutgoingWritten < pPacket->bufferUsed &&
             (writtenLen >= 0 || (writtenLen < 0 && errno == EINTR)));
    
    if (writtenLen < 0)
    {
        switch (errno)
        {
        case EAGAIN:
            ntStatus = STATUS_PENDING;
            BAIL_ON_NT_STATUS(ntStatus);
        default:
            ntStatus = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

error:
    
    return ntStatus;
}

static
NTSTATUS
RdrSocketPrepareSend(
    IN PRDR_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bIsSignatureRequired = FALSE;
    ULONG64 ullSessionId = 0;
    PRDR_SESSION2 pSession = NULL;

    switch (pPacket->protocolVer)
    {
    case SMB_PROTOCOL_VERSION_1:
        if (pPacket->allowSignature)
        {
            bIsSignatureRequired = RdrSocketIsSignatureRequired(pSocket);
        }

        if (bIsSignatureRequired)
        {
            pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;
        }

        SMBPacketHTOLSmbHeader(pPacket->pSMBHeader);

        pPacket->haveSignature = bIsSignatureRequired;

        if (pPacket->pSMBHeader->command != COM_NEGOTIATE)
        {
            pPacket->sequence = RdrSocketGetNextSequence_inlock(pSocket);
        }

        if (bIsSignatureRequired)
        {
            ntStatus = SMBPacketSign(
                pPacket,
                pPacket->sequence,
                pSocket->pSessionKey,
                pSocket->dwSessionKeyLength);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        pSocket->usUsedSlots++;
        break;
    case SMB_PROTOCOL_VERSION_2:
        ullSessionId = SMB_HTOL64(pPacket->pSMB2Header->ullSessionId);
        if (pSocket->pSessionHashByUID && ullSessionId != 0)
        {
            ntStatus = SMBHashGetValue(
                pSocket->pSessionHashByUID,
                &ullSessionId,
                OUT_PPVOID(&pSession));
            if (ntStatus == STATUS_NOT_FOUND)
            {
                ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* Set credit request */
        pPacket->pSMB2Header->usCredits = SMB_HTOL16(RdrSocketCreditsNeeded(pSocket));

        if (pSession && RdrSmb2ShouldSignPacket(
                pPacket,
                pSocket->ucSecurityMode & RDR_SMB2_SECMODE_SIGNING_ENABLED,
                pSocket->ucSecurityMode & RDR_SMB2_SECMODE_SIGNING_REQUIRED,
                gRdrRuntime.config.bSigningEnabled,
                gRdrRuntime.config.bSigningRequired))
        {
            ntStatus = RdrSmb2Sign(
                pPacket,
                pSession->pSessionKey,
                pSession->dwSessionKeyLength);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        /*
         * FIXME: use 1 slot for each chained command.
         * Even better, preallocate the slots
         */
        pSocket->usUsedSlots++;
        break;
    default:
        break;
    }

    pSocket->pOutgoing = pPacket;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
RdrSocketQueue(
    IN PRDR_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    LwListInsertBefore(&pSocket->PendingSend, &pContext->Link);
    if (pSocket->state >= RDR_SOCKET_STATE_NEGOTIATING)
    {
        LwRtlWakeTask(pSocket->pTask);
    }
}

NTSTATUS
RdrSocketTransceive(
    IN PRDR_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSMB_PACKET pPacket = &pContext->Packet;
    USHORT usMid = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    status = RdrSocketAcquireMid(pSocket, &usMid);
    BAIL_ON_NT_STATUS(status);

    switch(pPacket->protocolVer)
    {
    case SMB_PROTOCOL_VERSION_1:
        pPacket->pSMBHeader->mid = usMid;
        break;
    case SMB_PROTOCOL_VERSION_2:
        pPacket->pSMB2Header->ullCommandSequence = SMB_HTOL64((ULONG64) usMid);
        break;
    default:
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
        break;
    }

    pContext->usMid = usMid;

    RdrSocketQueue(pSocket, pContext);

    status = STATUS_PENDING;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrSocketReceivePacket(
    IN PRDR_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t readLen = 0;

    if (pPacket->bufferUsed < sizeof(NETBIOS_HEADER))
    {
        while (pPacket->bufferUsed < sizeof(NETBIOS_HEADER))
        {
            ntStatus = RdrSocketRead(
                pSocket,
                pPacket->pRawBuffer + pPacket->bufferUsed,
                sizeof(NETBIOS_HEADER) - pPacket->bufferUsed,
                &readLen);
            BAIL_ON_NT_STATUS(ntStatus);
            
            if (readLen == 0)
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            
            pPacket->bufferUsed += readLen;
        }

        pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) pPacket->pRawBuffer;
        pPacket->pNetBIOSHeader->len = htonl(pPacket->pNetBIOSHeader->len);

        if ((uint64_t) pPacket->pNetBIOSHeader->len + sizeof(NETBIOS_HEADER) > (uint64_t) pPacket->bufferLen)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    while (pPacket->bufferUsed < pPacket->pNetBIOSHeader->len + sizeof(NETBIOS_HEADER))
    {
        ntStatus = RdrSocketRead(
            pSocket,
            pPacket->pRawBuffer + pPacket->bufferUsed,
            pPacket->pNetBIOSHeader->len + sizeof(NETBIOS_HEADER) - pPacket->bufferUsed,
            &readLen);
        BAIL_ON_NT_STATUS(ntStatus);
        
        if (readLen == 0)
        {
            ntStatus = STATUS_END_OF_FILE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPacket->bufferUsed += readLen;
    }

    pPacket->pSMBHeader = (SMB_HEADER *) (pPacket->pRawBuffer + sizeof(NETBIOS_HEADER));
    /* Bounds check the first byte of the smb header */
    if ((PBYTE) &pPacket->pSMBHeader->smb[0] >= pPacket->pRawBuffer + pPacket->bufferUsed)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (pPacket->pSMBHeader->smb[0])
    {
    case 0xFF:
        if (pSocket->version == SMB_PROTOCOL_VERSION_2)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if ((PBYTE) pPacket->pSMBHeader + sizeof(SMB_HEADER) >= pPacket->pRawBuffer + pPacket->bufferUsed)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPacket->protocolVer = SMB_PROTOCOL_VERSION_1;
        if (SMBIsAndXCommand(SMB_LTOH8(pPacket->pSMBHeader->command)))
        {
            pPacket->pAndXHeader = (ANDX_HEADER *)
                (pPacket->pRawBuffer + sizeof(SMB_HEADER) + sizeof(NETBIOS_HEADER));

            if (pPacket->pSMBHeader->error == 0 &&
                (PBYTE) pPacket->pAndXHeader + sizeof(ANDX_HEADER) >= pPacket->pRawBuffer + pPacket->bufferUsed)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
        pPacket->pParams = pPacket->pAndXHeader ?
            (PBYTE) pPacket->pAndXHeader + sizeof(ANDX_HEADER) :
            (PBYTE) pPacket->pSMBHeader + sizeof(SMB_HEADER);
        pPacket->pData = NULL;
        break;
    case 0xFE:
        if ((PBYTE) pPacket->pSMB2Header + sizeof(SMB2_HEADER) >= pPacket->pRawBuffer + pPacket->bufferUsed)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPacket->protocolVer = SMB_PROTOCOL_VERSION_2;
        break;
    default:
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
ULONG
RdrSocketReceivePacketSize(
    PRDR_SOCKET pSocket
    )
{
    ULONG ulSize = 0;

    ulSize = pSocket->ulMaxTransactSize;

    if (pSocket->ulMaxReadSize > ulSize)
    {
        ulSize = pSocket->ulMaxReadSize;
    }

    if (ulSize == 0)
    {
        ulSize = 1024 * 60;
    }

    ulSize += 1024 * 4;

    return ulSize;
}

static
USHORT
RdrSocketCreditsNeeded(
    PRDR_SOCKET pSocket
    )
{
    PLW_LIST_LINKS pLink = NULL;
    USHORT usPacketCount = 0;
    USHORT usCredits = 0;
    SHORT sDeficit = 0;

    /*
     * We want to request enough credits to get our
     * maximum number of slots up to the configured minimum.
     */
    if (pSocket->usMaxSlots < gRdrRuntime.config.usMinCreditReserve)
    {
        usCredits = gRdrRuntime.config.usMinCreditReserve - pSocket->usMaxSlots;
    }

    /* Count packets in the queue */
    for (pLink = pSocket->PendingSend.Next; pLink != &pSocket->PendingSend; pLink = pLink->Next)
    {
        usPacketCount++;
    }

    /* Calculate how many packets we have pending above the number of available slots */
    sDeficit = usPacketCount - (pSocket->usMaxSlots - pSocket->usUsedSlots);

    /*
     * If the value calculated above won't cover the deficit,
     * see if the server can float us some extra credits.
     */
    if (sDeficit > 0 && sDeficit > usCredits)
    {
        usCredits = sDeficit;
    }

    /* Add credit for the packet we are about to send */
    usCredits++;

    return usCredits;
}

static
NTSTATUS
RdrSocketTaskConnect(
    PRDR_SOCKET pSocket,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    union
    {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
    } addr;
    SOCKLEN_TYPE len = 0;
    int pf = 0;
    int err = 0;

    /* Keep looping until we exhaust address list */
    do
    {
        if (pSocket->fd < 0)
        {
            /* We don't have a socket yet */
            status = STATUS_PENDING;
        }
        else if (WakeMask & LW_TASK_EVENT_FD_WRITABLE)
        {
            /* Get result of last connect() */
            len = sizeof(err);
            if (getsockopt(pSocket->fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
            {
                status = LwErrnoToNtStatus(errno);
                BAIL_ON_NT_STATUS(status);
            }

            if (err != 0)
            {
                pSocket->AddressIndex++;
                status = LwErrnoToNtStatus(err);
            }
        }
        else if (WakeMask & LW_TASK_EVENT_TIME)
        {
            /* We timed out */
            pSocket->AddressIndex++;
            status = STATUS_IO_TIMEOUT;
        }

        /* Clear WakeMask so we don't use it when looping */
        WakeMask = 0;

        if (status == STATUS_SUCCESS)
        {
            break;
        }
        else if (pSocket->AddressIndex < pSocket->AddressListCount)
        {
            /* We still have addresses to try */

            /* Convert to sockaddr */
            memset(&addr, 0, sizeof(addr));
            switch (pSocket->ppAddressList[pSocket->AddressIndex]->AddressType)
            {
            case LWNET_IP_ADDR_V4:
                pf = PF_INET;
                len = sizeof(addr.v4);
                addr.v4.sin_family = AF_INET;
                addr.v4.sin_port = htons(445);
                memcpy(
                    &addr.v4.sin_addr.s_addr,
                    pSocket->ppAddressList[pSocket->AddressIndex]->Address.Ip4Addr,
                    sizeof(addr.v4.sin_addr.s_addr));
                break;
            case LWNET_IP_ADDR_V6:
                pf = PF_INET6;
                len = sizeof(addr.v6);
                addr.v6.sin6_family = AF_INET6;
                addr.v6.sin6_port = htons(445);
                memcpy(
                    &addr.v6.sin6_addr.s6_addr,
                    pSocket->ppAddressList[pSocket->AddressIndex]->Address.Ip6Addr,
                    sizeof(addr.v6.sin6_addr.s6_addr));
                break;
            default:
                status = STATUS_INTERNAL_ERROR;
                BAIL_ON_NT_STATUS(status);
            }

            /* Close out previous socket if it exists */
            if (pSocket->fd >= 0)
            {
                status = LwRtlSetTaskFd(
                    pSocket->pTask,
                    pSocket->fd,
                    0);
                BAIL_ON_NT_STATUS(status);
                close(pSocket->fd);
                pSocket->fd = -1;
            }

            /* Create socket */
            pSocket->fd = socket(pf, SOCK_STREAM, IPPROTO_TCP);

            if (pSocket->fd < 0)
            {
                status = ErrnoToNtStatus(errno);
                /* Treat error non-fatally and try again */
                continue;
            }

            if (fcntl(pSocket->fd, F_SETFL, O_NONBLOCK) < 0)
            {
                status = ErrnoToNtStatus(errno);
                BAIL_ON_NT_STATUS(status);
            }

            /* Set fd on task */
            status = LwRtlSetTaskFd(
                pSocket->pTask,
                pSocket->fd,
                LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE);
            BAIL_ON_NT_STATUS(status);

            /* Connect */
            err = 0;
            if (connect(pSocket->fd, (struct sockaddr*) &addr, len))
            {
                err = errno;
            }

            switch (err)
            {
            case 0:
                /* Success, move on to negotiate */
                pSocket->state = RDR_SOCKET_STATE_NEGOTIATING;
                break;
            case EINPROGRESS:
                /* Wait for completion or timeout */
                *pWaitMask = LW_TASK_EVENT_FD_WRITABLE | LW_TASK_EVENT_TIME;
                *pllTime = gRdrRuntime.config.usConnectTimeout * RDR_NS_IN_S;
                status = STATUS_PENDING;
                BAIL_ON_NT_STATUS(status);
                break;
            default:
                /* Try again */
                pSocket->AddressIndex++;
                status = LwErrnoToNtStatus(err);
                continue;
            }
        }
        else
        {
            /* Nothing to do but fail */
            BAIL_ON_NT_STATUS(status);
        }
    } while (status != STATUS_SUCCESS);

    /* Success, move on to negotiate */
    pSocket->state = RDR_SOCKET_STATE_NEGOTIATING;
    *pWaitMask = LW_TASK_EVENT_YIELD;

cleanup:

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrSocketTaskTransceive(
    PRDR_SOCKET pSocket,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = TRUE;
    PLW_LIST_LINKS pLink = NULL;
    PRDR_OP_CONTEXT pIrpContext = NULL;

    if (WakeMask & LW_TASK_EVENT_TIME)
    {
        if (pSocket->bEcho)
        {
            /* Server is unresponsive to echo, time out socket */
            status = STATUS_IO_TIMEOUT;
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            /* Send an echo now */
            LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
            status = RdrEcho(pSocket, "Ping!");
            LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);
            if (status == STATUS_PENDING)
            {
                status = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(status);
            pSocket->bEcho = TRUE;
        }
    }

    if (WakeMask & LW_TASK_EVENT_FD_READABLE)
    {
        pSocket->bReadBlocked = FALSE;
    }

    if (WakeMask & LW_TASK_EVENT_FD_WRITABLE)
    {
        pSocket->bWriteBlocked = FALSE;
    }

    if (!pSocket->pOutgoing && !LwListIsEmpty(&pSocket->PendingSend) &&
        pSocket->usUsedSlots < pSocket->usMaxSlots)
    {
        if (pSocket->usUsedSlots == 0)
        {
            /* Reset timeout since we will now have an outstanding request */
            *pllTime = 0;
        }

        pLink = LwListRemoveHead(&pSocket->PendingSend);
        pIrpContext = LW_STRUCT_FROM_FIELD(pLink, RDR_OP_CONTEXT, Link);
        status = RdrSocketPrepareSend(pSocket, &pIrpContext->Packet);
        BAIL_ON_NT_STATUS(status);
        LwListInsertTail(&pSocket->PendingResponse, &pIrpContext->Link);
        pIrpContext = NULL;
    }

    *pWaitMask = LW_TASK_EVENT_EXPLICIT;

    if (!pSocket->bReadBlocked)
    {
        if (!pSocket->pPacket)
        {
            status = RdrAllocatePacket(
                RdrSocketReceivePacketSize(pSocket),
                &pSocket->pPacket);
            BAIL_ON_NT_STATUS(status);
        }

        status = RdrSocketReceivePacket(pSocket, pSocket->pPacket);
        switch(status)
        {
        case STATUS_SUCCESS:
            /* Reset timeout since we successfully received a response */
            *pllTime = 0;

            /* This function should free packet on error */
            status = RdrSocketDispatchPacket(pSocket, pSocket->pPacket);
            pSocket->pPacket = NULL;
            BAIL_ON_NT_STATUS(status);

            *pWaitMask |= LW_TASK_EVENT_YIELD;
            break;
        case STATUS_PENDING:
            pSocket->bReadBlocked = TRUE;
            break;
        default:
            BAIL_ON_NT_STATUS(status);
        }
    }

    if (!pSocket->bWriteBlocked && pSocket->pOutgoing)
    {
        status = RdrSocketSendData(pSocket, pSocket->pOutgoing);
        switch (status)
        {
        case STATUS_SUCCESS:
            pSocket->OutgoingWritten = 0;
            pSocket->pOutgoing = NULL;
            *pWaitMask |= LW_TASK_EVENT_YIELD;
            break;
        case STATUS_PENDING:
            pSocket->bWriteBlocked = TRUE;
            break;
        default:
            BAIL_ON_NT_STATUS(status);
        }
    }

    /* Determine next wake mask */
    if (pSocket->bReadBlocked)
    {
        *pWaitMask |= LW_TASK_EVENT_FD_READABLE;
    }

    if (pSocket->bWriteBlocked && pSocket->pOutgoing)
    {
        *pWaitMask |= LW_TASK_EVENT_FD_WRITABLE;
    }

    *pWaitMask |= LW_TASK_EVENT_TIME;
    /* Determine next timeout */
    if (*pllTime == 0)
    {
        if (pSocket->bEcho)
        {
            *pllTime = gRdrRuntime.config.usEchoTimeout * RDR_NS_IN_S;
        }
        else if (pSocket->usUsedSlots != 0)
        {
            *pllTime = gRdrRuntime.config.usResponseTimeout * RDR_NS_IN_S;
        }
        else
        {
            *pllTime = gRdrRuntime.config.usEchoInterval * RDR_NS_IN_S;
        }
    }

cleanup:

    if (pIrpContext)
    {
        RdrContinueContext(pIrpContext, status, NULL);
    }

    return status;

error:

    goto cleanup;
}

static
VOID
RdrSocketTask(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = 0;
    PRDR_SOCKET pSocket = (PRDR_SOCKET) pContext;
    BOOLEAN bGlobalLock = FALSE;
    BOOLEAN bInLock = FALSE;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        /* Unregister the fd from the task */
        if (pSocket->fd >= 0)
        {
            LwRtlSetTaskFd(pTask, pSocket->fd, 0);
        }

        /* The task holds the last implicit reference to a socket,
           so we can now clean up and free the structure */
        LWIO_LOCK_MUTEX(bGlobalLock, &gRdrRuntime.Lock);
        RdrSocketFreeContents(pSocket);
        LWIO_UNLOCK_MUTEX(bGlobalLock, &gRdrRuntime.Lock);

        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    if (pSocket->state == RDR_SOCKET_STATE_ERROR)
    {
        /* Go back to sleep until we are cancelled */
        *pWaitMask = LW_TASK_EVENT_EXPLICIT;
        goto cleanup;
    }

    if (pSocket->state < RDR_SOCKET_STATE_NEGOTIATING)
    {
        status = RdrSocketTaskConnect(pSocket, WakeMask, pWaitMask, pllTime);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = RdrSocketTaskTransceive(pSocket, WakeMask, pWaitMask, pllTime);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return;

error:

    if (status != STATUS_PENDING)
    {
        LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);
        RdrSocketInvalidate_InLock(pSocket, status);
        *pWaitMask = LW_TASK_EVENT_EXPLICIT;
    }

    goto cleanup;
}

static
NTSTATUS
RdrSocketFindResponseContextByMid(
    PRDR_SOCKET pSocket,
    USHORT usMid,
    PRDR_OP_CONTEXT* ppContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_LIST_LINKS pLink = NULL;
    PRDR_OP_CONTEXT pContext = NULL;

    /*
     * We expect responses to come back in roughly the same order as
     * the requests, so a linear traversal of the list should terminate
     * quickly in the common case.
     */
    while ((pLink = LwListTraverse(&pSocket->PendingResponse, pLink)))
    {
        pContext = LW_STRUCT_FROM_FIELD(pLink, RDR_OP_CONTEXT, Link);

        if (pContext->usMid == usMid)
        {
            break;
        }
        else
        {
            pContext = NULL;
        }
    }

    if (!pContext)
    {
        status = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    *ppContext = pContext;

cleanup:

    return status;

error:

    *ppContext = NULL;

    goto cleanup;
}

static
NTSTATUS
RdrSocketDispatchPacket(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    switch(pPacket->protocolVer)
    {
    case SMB_PROTOCOL_VERSION_1:
        return RdrSocketDispatchPacket1(pSocket, pPacket);
    case SMB_PROTOCOL_VERSION_2:
        return RdrSocketDispatchPacket2(pSocket, pPacket);
    default:
        return STATUS_INTERNAL_ERROR;
    }
}

static
NTSTATUS
RdrSocketDispatchPacket2(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS status = 0;
    USHORT usMid = 0;
    BOOLEAN bLocked = TRUE;
    BOOLEAN bKeep = FALSE;
    PRDR_SESSION2 pSession = NULL;
    ULONG64 ullSessionId = 0;
    PRDR_OP_CONTEXT pContext = NULL;

    /*
     * To verify the signature, we need to look up the session so we
     * can grab the session key.
     */
    ullSessionId = SMB_HTOL64(pPacket->pSMB2Header->ullSessionId);
    if (pSocket->pSessionHashByUID && ullSessionId != 0)
    {
         status = SMBHashGetValue(
             pSocket->pSessionHashByUID,
             &ullSessionId,
             OUT_PPVOID(&pSession));
         if (status == STATUS_NOT_FOUND)
         {
             status = STATUS_SUCCESS;
         }
         BAIL_ON_NT_STATUS(status);
    }

    status = RdrSmb2DecodeHeader(
        pPacket,
        RdrSmb2ShouldVerifyPacket(pPacket, gRdrRuntime.config.bSigningRequired),
        pSession ? pSession->pSessionKey : NULL,
        pSession ? pSession->dwSessionKeyLength : 0);
    BAIL_ON_NT_STATUS(status);

    /*
     * Even if we end up discarding the packet, apply any credits now
     */
    pSocket->usMaxSlots += pPacket->pSMB2Header->usCredits - 1;

    /* Response? */
    if (pPacket->pSMB2Header->ulFlags & SMB2_FLAGS_SERVER_TO_REDIR)
    {
        if ((pPacket->pSMB2Header->ulFlags & SMB2_FLAGS_ASYNC_COMMAND) &&
            pPacket->pSMB2Header->error == STATUS_PENDING)
        {
            LWIO_LOG_DEBUG("Discarding interim response: %u", (unsigned int) pPacket->pSMB2Header->command);
            goto cleanup;
        }

        /*
         * Grab sequence number so we can look up associated request.
         * Note that we only need the lower 16 bits to distinguish between
         * outstanding requests.
         */
        usMid = (USHORT) pPacket->pSMB2Header->ullCommandSequence;

        status = RdrSocketFindResponseContextByMid(
            pSocket,
            usMid,
            &pContext);

        switch(status)
        {
        case STATUS_SUCCESS:
            break;
        case STATUS_NOT_FOUND:
            status = STATUS_SUCCESS;
            goto cleanup;
        default:
            BAIL_ON_NT_STATUS(status);
        }

        LwListRemove(&pContext->Link);
        /*
         * Make sure the response command was what we expected.
         * Handle negotiate responses carefully as the request
         * could have been an SMB1 packet
         */
        if ((pContext->Packet.protocolVer == SMB_PROTOCOL_VERSION_1 &&
             pPacket->pSMB2Header->command != COM2_NEGOTIATE) ||
            (pContext->Packet.protocolVer == SMB_PROTOCOL_VERSION_2 &&
             SMB_HTOL16(pContext->Packet.pSMB2Header->command) != pPacket->pSMB2Header->command))
        {
            status = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(status);
        }

        LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);
        bKeep = RdrContinueContext(pContext, STATUS_SUCCESS, pPacket);
        /* Ownership of packet was transferred to continuation */
        pPacket = NULL;
        LWIO_LOCK_MUTEX(bLocked, &pSocket->mutex);

        if (bKeep)
        {
            LwListInsertTail(&pSocket->PendingResponse, &pContext->Link);
        }
        else
        {
            pSocket->usUsedSlots--;
        }
    }
    else
    {
        /* FIXME: handle echos, oplock breaks, etc. */
        LWIO_LOG_DEBUG("Discarding non-response packet: %u", (unsigned int) pPacket->pSMB2Header->command);
    }

cleanup:

    if (pPacket)
    {
        RdrFreePacket(pPacket);
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrSocketDispatchPacket1(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    USHORT usMid = 0;
    BOOLEAN bLocked = TRUE;
    BOOLEAN bKeep = FALSE;
    PRDR_OP_CONTEXT pContext = NULL;

    if (!(pPacket->pSMBHeader->flags & FLAG_RESPONSE))
    {
        /* Drop non-response packets */
        goto cleanup;
    }

    usMid = SMB_HTOL16(pPacket->pSMBHeader->mid);

    ntStatus = RdrSocketFindResponseContextByMid(
                    pSocket,
                    usMid,
                    &pContext);
    switch(ntStatus)
    {
    case STATUS_SUCCESS:
        break;
    case STATUS_NOT_FOUND:
        /* Drop packets that we gave up waiting for */
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    default:
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LwListRemove(&pContext->Link);

    ntStatus = SMBPacketDecodeHeader(
        pPacket,
        (pContext->Packet.haveSignature &&
         !pSocket->bIgnoreServerSignatures &&
         pSocket->pSessionKey != NULL),
         pContext->Packet.sequence + 1,
         pSocket->pSessionKey,
         pSocket->dwSessionKeyLength);
    if (ntStatus == STATUS_PENDING)
      BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);
    bKeep = RdrContinueContext(pContext, ntStatus, pPacket);
    /* Ownership of packet was transferred to continuation */
    pPacket = NULL;
    LWIO_LOCK_MUTEX(bLocked, &pSocket->mutex);

    BAIL_ON_NT_STATUS(ntStatus);

    if (bKeep)
    {
        LwListInsertTail(&pSocket->PendingResponse, &pContext->Link);
    }
    else
    {
        pSocket->usUsedSlots--;
    }

cleanup:

    if (pPacket)
    {
        RdrFreePacket(pPacket);
    }

    return ntStatus;

error:

    goto cleanup;
}

VOID
RdrSocketCancel(
    IN PRDR_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    /* Currently a no-op */
}

VOID
RdrSocketRevive(
    PRDR_SOCKET pSocket
    )
{
    if (pSocket->pTimeout)
    {
        LwRtlCancelTask(pSocket->pTimeout);
        LwRtlReleaseTask(&pSocket->pTimeout);
    }
}

static
VOID
RdrSocketUnlink(
    PRDR_SOCKET pSocket
    )
{
    if (pSocket->bParentLink)
    {
        SMBHashRemoveKey(gRdrRuntime.pSocketHashByName,
                         pSocket->pwszHostname);
        pSocket->bParentLink = FALSE;
    }
}

static
VOID
RdrSocketTimeout(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    PRDR_SOCKET pSocket = pContext;
    BOOLEAN bInLock = FALSE;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
    }

    if ((WakeMask & LW_TASK_EVENT_TIME) ||
        ((WakeMask & LW_TASK_EVENT_CANCEL) && RdrIsShutdownSet()))
    {
        LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.Lock);

        if (pSocket->refCount == 0)
        {
            RdrSocketUnlink(pSocket);
            RdrSocketFree(pSocket);
            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
        }

        LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
    }
    else if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }
}

static
NTSTATUS
RdrSocketConnectDomain(
    PRDR_SOCKET pSocket,
    PWSTR pwszDomain
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLWNET_DC_INFO pInfo = NULL;
    PSTR pszDomain = NULL;
    PWSTR pwszDomainController = NULL;

    status = LwRtlCStringAllocateFromWC16String(&pszDomain, pwszDomain);
    BAIL_ON_NT_STATUS(status);

    status = LwWin32ErrorToNtStatus(
        LWNetGetDCName(
            NULL,
            pszDomain,
            NULL,
            0,
            &pInfo));
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(&pwszDomainController, pInfo->pszDomainControllerName);
    BAIL_ON_NT_STATUS(status);

    status = LwWin32ErrorToNtStatus(
        LWNetResolveName(
            pwszDomainController,
            &pSocket->pwszCanonicalName,
            &pSocket->ppAddressList,
            &pSocket->AddressListCount));
    BAIL_ON_NT_STATUS(status);

    LwRtlWakeTask(pSocket->pTask);

cleanup:

    if (pInfo)
    {
        LWNetFreeDCInfo(pInfo);
    }

    RTL_FREE(&pszDomain);
    RTL_FREE(&pwszDomainController);

    return status;

error:

    RdrSocketInvalidate(pSocket, status);

    goto cleanup;
}

static
NTSTATUS
RdrSocketConnectHost(
    PRDR_SOCKET pSocket
    )
{
    NTSTATUS status = 0;
        
    status = LwWin32ErrorToNtStatus(
        LWNetResolveName(
            pSocket->pwszHostname,
            &pSocket->pwszCanonicalName,
            &pSocket->ppAddressList,
            &pSocket->AddressListCount));
    BAIL_ON_NT_STATUS(status);

    LwRtlWakeTask(pSocket->pTask);

cleanup:

    return status;

error:

    RdrSocketInvalidate(pSocket, status);

    goto cleanup;
}

NTSTATUS
RdrSocketConnect(
    PRDR_SOCKET pSocket
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszDomain = NULL;

    status = RdrResolveToDomain(pSocket->pwszHostname, &pwszDomain);
    switch (status)
    {
    case STATUS_SUCCESS:
        status = RdrSocketConnectDomain(pSocket, pwszDomain);
        BAIL_ON_NT_STATUS(status);
        break;
    case STATUS_NOT_FOUND:
        status = RdrSocketConnectHost(pSocket);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        BAIL_ON_NT_STATUS(status);
    }

error:

    RTL_FREE(&pwszDomain);

    return status;
}

static
NTSTATUS
RdrSocketRead(
    PRDR_SOCKET pSocket,
    uint8_t *buffer,
    uint32_t len,
    uint32_t *actualLen
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t totalRead = 0;
    ssize_t nRead = 0;

    while (totalRead < len)
    {
        nRead = read(pSocket->fd, buffer + totalRead, len - totalRead);
        if(nRead < 0)
        {
            switch (errno)
            {
            case EINTR:
                continue;
            case EAGAIN:
                if (totalRead == 0)
                {
                    ntStatus = STATUS_PENDING;
                }
                else
                {
                    /* Return a successful partial read */
                    goto cleanup;
                }
                break;
            default:
                ntStatus = ErrnoToNtStatus(errno);
            }

            BAIL_ON_NT_STATUS(ntStatus);
        }
        else if (nRead == 0)
        {
            /* EOF */
            goto cleanup;
        }

        totalRead += nRead;
    }

cleanup:

    *actualLen = totalRead;

    return ntStatus;

error:

    goto cleanup;
}

VOID
RdrSocketInvalidate(
    PRDR_SOCKET pSocket,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    RdrSocketInvalidate_InLock(pSocket, ntStatus);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
VOID
RdrSocketInvalidate_InLock(
    PRDR_SOCKET pSocket,
    NTSTATUS status
    )
{
    BOOLEAN bInGlobalLock = FALSE;

    if (pSocket->state == RDR_SOCKET_STATE_ERROR)
    {
        return;
    }

    pSocket->state = RDR_SOCKET_STATE_ERROR;
    pSocket->error = status;

    LWIO_LOCK_MUTEX(bInGlobalLock, &gRdrRuntime.Lock);
    RdrSocketUnlink(pSocket);
    if (pSocket->pTimeout)
    {
        LwRtlWakeTask(pSocket->pTimeout);
        LwRtlReleaseTask(&pSocket->pTimeout);
    }
    LWIO_UNLOCK_MUTEX(bInGlobalLock, &gRdrRuntime.Lock);

    RdrNotifyContextList(
        &pSocket->PendingSend,
        TRUE,
        &pSocket->mutex,
        status,
        NULL);

    RdrNotifyContextList(
        &pSocket->PendingResponse,
        TRUE,
        &pSocket->mutex,
        status,
        NULL);

    RdrNotifyContextList(
        &pSocket->StateWaiters,
        TRUE,
        &pSocket->mutex,
        status,
        pSocket);

    LwListInit(&pSocket->PendingSend);
    LwListInit(&pSocket->PendingResponse);
    LwListInit(&pSocket->StateWaiters);
}

BOOLEAN
RdrSocketIsValid(
    PRDR_SOCKET pSocket
    )
{
    BOOLEAN bLocked = FALSE;
    BOOLEAN bResult = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &pSocket->mutex);
    bResult = pSocket->state != RDR_SOCKET_STATE_ERROR;
    LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);

    return bResult;
}

VOID
RdrSocketRetain(
    PRDR_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.Lock);

    pSocket->refCount++;

    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
}

VOID
RdrSocketRelease(
    PRDR_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.Lock);

    assert(pSocket->refCount > 0);

    /* If the socket is no longer referenced and
       it is not usable, free it immediately.
       Otherwise, allow the reaper to collect it
       asynchronously */
    if (--pSocket->refCount == 0)
    {
        if (pSocket->state != RDR_SOCKET_STATE_READY)
        {
            RdrSocketUnlink(pSocket);
            RdrSocketFree(pSocket);
            LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
        }
        else
        {
            LWIO_LOG_VERBOSE("Socket %p is eligible for reaping", pSocket);
            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pSocket->pTimeout,
                    gRdrRuntime.pSocketTimerGroup,
                    RdrSocketTimeout,
                    pSocket) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pSocket->pTimeout);
                LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
            }
            else
            {
                LWIO_LOG_VERBOSE("Could not start timer for socket %p; closing immediately", pSocket);
                RdrSocketUnlink(pSocket);
                RdrSocketFree(pSocket);
                LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
    }
}

static
VOID
RdrSocketFree(
    PRDR_SOCKET pSocket
    )
{
    /* 
     * If the task for the socket is alive, cancel it and let
     * it finish freeing the socket after it has cleaned up.
     * Otherwise, free the contents immediately.
     */
    if (pSocket->pTask)
    {
        LwRtlCancelTask(pSocket->pTask);
    }
    else
    {
        RdrSocketFreeContents(pSocket);
    }
}

static
VOID
RdrSocketFreeContents(
    PRDR_SOCKET pSocket
    )
{
    assert(!pSocket->refCount);

    if ((pSocket->fd >= 0) && (close(pSocket->fd) < 0))
    {
        LWIO_LOG_ERROR("Failed to close socket [fd:%d]", pSocket->fd);
    }

    LWIO_SAFE_FREE_MEMORY(pSocket->pwszHostname);
    LWIO_SAFE_FREE_MEMORY(pSocket->pSecurityBlob);

    LWNetResolveNameFree(
        pSocket->pwszCanonicalName,
        pSocket->ppAddressList,
        pSocket->AddressListCount);

    SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);
    SMBHashSafeFree(&pSocket->pSessionHashByUID);

    RdrFreePacket(pSocket->pPacket);

    pthread_mutex_destroy(&pSocket->mutex);

    LWIO_SAFE_FREE_MEMORY(pSocket->pSessionKey);

    if (pSocket->pTimeout)
    {
        LwRtlCancelTask(pSocket->pTimeout);
        LwRtlReleaseTask(&pSocket->pTimeout);
    }

    LwRtlReleaseTask(&pSocket->pTask);

    LwIoFreeMemory(pSocket);
}

VOID
RdrSocketSetIgnoreServerSignatures(
    PRDR_SOCKET pSocket,
    BOOLEAN bValue
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->bIgnoreServerSignatures = bValue;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
NTSTATUS
RdrSocketAcquireMid(
    PRDR_SOCKET pSocket,
    USHORT* pusMid
    )
{
    NTSTATUS ntStatus = 0;

    if (pSocket->state == RDR_SOCKET_STATE_ERROR)
    {
        ntStatus = pSocket->error;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pusMid = pSocket->ullNextMid++;

error:

    return ntStatus;
}

NTSTATUS
RdrSocketFindOrCreate(
    IN PCWSTR pwszHostname,
    OUT PRDR_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;

    PRDR_SOCKET pSocket = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
    
    ntStatus = SMBHashGetValue(
        gRdrRuntime.pSocketHashByName,
        pwszHostname,
        (PVOID *) &pSocket);
    
    if (!ntStatus)
    {
        pSocket->refCount++;
        RdrSocketRevive(pSocket);
    }
    else
    {
        ntStatus = RdrSocketCreate(
            pwszHostname,
            &pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
        
        ntStatus = SMBHashSetValue(
            gRdrRuntime.pSocketHashByName,
            pSocket->pwszHostname,
            pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        pSocket->bParentLink = TRUE;
    }
    
    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);

    *ppSocket = pSocket;
    
cleanup:
    
    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.Lock);
    
    return ntStatus;
    
error:

    *ppSocket = NULL;

    goto cleanup;
}

NTSTATUS
RdrSocketAddSessionByUID(
    PRDR_SOCKET  pSocket,
    PRDR_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashSetValue(
                    pSocket->pSessionHashByUID,
                    &pSession->uid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->bParentLink = TRUE;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
RdrSocketAddSession2ById(
    PRDR_SOCKET  pSocket,
    PRDR_SESSION2 pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashSetValue(
                    pSocket->pSessionHashByUID,
                    &pSession->ullSessionId,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->bParentLink = TRUE;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
RdrEcho(
    PRDR_SOCKET pSocket,
    PCSTR pszMessage
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;

    status = RdrCreateContext(NULL, &pContext);
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrEchoComplete;
    pContext->State.Echo.pSocket = pSocket;

    switch (pSocket->version)
    {
    case SMB_PROTOCOL_VERSION_1:
        status = RdrTransceiveEcho(pContext, pSocket, pszMessage);
        BAIL_ON_NT_STATUS(status);
        break;
    case SMB_PROTOCOL_VERSION_2:
        status = RdrTransceiveEcho2(pContext, pSocket);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(status);
        break;
    }

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        RdrFreeContext(pContext);
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveEcho(
    PRDR_OP_CONTEXT pContext,
    PRDR_SOCKET pSocket,
    PCSTR pszMessage
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PECHO_REQUEST_HEADER pEchoHeader = NULL;
    PBYTE pCursor = NULL;
    ULONG ulRemainingSpace = 0;

    status = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(status);
    
    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_ECHO,
        0,
        0,
        0xFFFF,
        gRdrRuntime.SysPid,
        0,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);
    
    pContext->Packet.pSMBHeader->wordCount = 1;

    pCursor = pContext->Packet.pParams;
    ulRemainingSpace = pContext->Packet.bufferLen - (pCursor - pContext->Packet.pRawBuffer);

    pEchoHeader = (PECHO_REQUEST_HEADER) pCursor;
    
    status = Advance(&pCursor, &ulRemainingSpace, sizeof(*pEchoHeader));
    BAIL_ON_NT_STATUS(status);

    pEchoHeader->echoCount = SMB_HTOL16(1);
    pEchoHeader->byteCount = SMB_HTOL16(strlen(pszMessage) + 1);

    pContext->Packet.pData = pCursor;

    status = Advance(&pCursor, &ulRemainingSpace, strlen(pszMessage) + 1);
    BAIL_ON_NT_STATUS(status);

    strcpy((char*) pContext->Packet.pData, pszMessage);
        
    pContext->Packet.bufferUsed += (pCursor - pContext->Packet.pParams);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveEcho2(
    PRDR_OP_CONTEXT pContext,
    PRDR_SOCKET pSocket
    )
{
    NTSTATUS status = 0;
    PBYTE pCursor = NULL;
    ULONG ulRemaining = 0;

    status = RdrAllocateContextPacket(pContext, RDR_SMB2_STUB_SIZE);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2BeginPacket(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeHeader(
        &pContext->Packet,
        COM2_ECHO,
        0, /* flags */
        gRdrRuntime.SysPid,
        0, /* tid */
        0, /* session id */
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2EncodeStubRequest(
        &pContext->Packet,
        &pCursor,
        &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

 cleanup:

    return status;

 error:

    goto cleanup;
}

static
BOOLEAN
RdrEchoComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pData
    )
{
    PSMB_PACKET pPacket = pData;
    PRDR_SOCKET pSocket = pContext->State.Echo.pSocket;
    BOOLEAN bLocked = FALSE;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    LWIO_LOCK_MUTEX(bLocked, &pSocket->mutex);

    pSocket->bEcho = FALSE;

cleanup:

    LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);

    RdrFreePacket(pPacket);
    RdrFreeContext(pContext);

    return FALSE;

error:

    goto cleanup;
}
