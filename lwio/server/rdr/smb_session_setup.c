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
 *        SMB Client Session Setup Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

/* @todo: clean up */
/* @todo: the first session on a socket is always VC 0; increment
thereafter */
NTSTATUS
SessionSetup(
    IN OUT PSMB_SOCKET pSocket,
    PIO_CREDS pCreds,
    OUT uint16_t* pUID,
    OUT PBYTE* ppSessionKey,
    OUT PDWORD pdwSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;
    SMB_PACKET packet = {0};

    uint32_t packetByteCount = 0;

    HANDLE   hSMBGSSContext = NULL;

    wchar16_t nativeOS[1024];
    wchar16_t nativeLanMan[1024];
    wchar16_t nativeDomain[1024];
    uint8_t*  pSecurityBlob2 = NULL;
    uint32_t  dwSecurityBlobLen2 = 0;
    PBYTE     pSessionKey = NULL;
    DWORD     dwSessionKeyLength = 0;

    SMB_PACKET *pResponsePacket = NULL;
    PSESSION_SETUP_RESPONSE_HEADER_WC_4 pResponseHeader = NULL;
    WORD wUid = 0;

    ntStatus = SMBGSSContextBuild(
        pSocket->pwszCanonicalName,
        pCreds,
        &hSMBGSSContext);
    BAIL_ON_NT_STATUS(ntStatus);
    
    ntStatus = SMBGSSContextNegotiate(
        hSMBGSSContext,
        pSocket->pSecurityBlob,
        pSocket->securityBlobLen,
        &pSecurityBlob2,
        &dwSecurityBlobLen2);
    BAIL_ON_NT_STATUS(ntStatus);
    
    /* @todo: make initial length configurable */
    ntStatus = SMBPacketBufferAllocate(
        pSocket->hPacketAllocator,
        1024*64,
        &packet.pRawBuffer,
        &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    while (!SMBGSSContextNegotiateComplete(hSMBGSSContext))
    {
        PBYTE pSecurityBlob = NULL;
        PWSTR pwszNativeOS = NULL;
        PWSTR pwszNativeLanman = NULL;
        PWSTR pwszNativeDomain = NULL;

        ntStatus = SMBPacketMarshallHeader(
                        packet.pRawBuffer,
                        packet.bufferLen,
                        COM_SESSION_SETUP_ANDX,
                        0,
                        0,
                        0xFFFF,
                        gRdrRuntime.SysPid,
                        wUid,
                        0,
                        TRUE,
                        &packet);
        BAIL_ON_NT_STATUS(ntStatus);

        packet.pData = packet.pParams + sizeof(SESSION_SETUP_REQUEST_HEADER_WC_12);
        packet.bufferUsed += sizeof(SESSION_SETUP_REQUEST_HEADER_WC_12);
        /* If most commands have word counts which are easy to compute, this
           should be folded into a parameter to SMBPacketMarshallHeader() */
        packet.pSMBHeader->wordCount = 12;

        SESSION_SETUP_REQUEST_HEADER_WC_12 *pHeader =
            (SESSION_SETUP_REQUEST_HEADER_WC_12 *) packet.pParams;
        pHeader->maxBufferSize = 12288;
        pHeader->maxMpxCount = 50;
        pHeader->vcNumber = 1;
        pHeader->sessionKey = pSocket->sessionKey;
        pHeader->securityBlobLength = 0;
        pHeader->reserved = 0;
        pHeader->capabilities = CAP_UNICODE | CAP_NT_SMBS | CAP_STATUS32 |
            CAP_EXTENDED_SECURITY;

        pHeader->securityBlobLength = dwSecurityBlobLen2;

        wcstowc16s(nativeOS, L"Unix", sizeof(nativeOS));
        wcstowc16s(nativeLanMan, L"Likewise SMB", sizeof(nativeLanMan));
        /* @todo: change to native domain */
        wcstowc16s(nativeDomain, L"WORKGROUP", sizeof(nativeDomain));

        /* @todo: handle buffer size restart with ERESTART */
        ntStatus = MarshallSessionSetupRequestData(
                        packet.pData,
                        packet.bufferLen - packet.bufferUsed,
                        (packet.pData - (uint8_t *) packet.pSMBHeader) % 2,
                        &packetByteCount,
                        pSecurityBlob2,
                        dwSecurityBlobLen2,
                        nativeOS,
                        nativeLanMan,
                        nativeDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        assert(packetByteCount <= UINT16_MAX);
        pHeader->byteCount = (uint16_t) packetByteCount;
        packet.bufferUsed += packetByteCount;

        // byte order conversions
        SMB_HTOL16_INPLACE(pHeader->maxBufferSize);
        SMB_HTOL16_INPLACE(pHeader->maxMpxCount);
        SMB_HTOL16_INPLACE(pHeader->vcNumber);
        SMB_HTOL32_INPLACE(pHeader->sessionKey);
        SMB_HTOL16_INPLACE(pHeader->securityBlobLength);
        //SMB_HTOL32_INPLACE(pHeader->reserved);
        SMB_HTOL32_INPLACE(pHeader->capabilities);
        SMB_HTOL16_INPLACE(pHeader->byteCount);

        ntStatus = SMBPacketMarshallFooter(&packet);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Because there's no MID, only one SESSION_SETUP_ANDX packet can be
           outstanding. */
        /* FIXME: use flag and condition variable here to wait for outstanding
           session setup to finish */

        /* @todo: on send packet error, the response must be removed from the
           tree. */
        ntStatus = SMBSocketSend(pSocket, &packet);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pResponsePacket)
        {
            SMBPacketRelease(pSocket->hPacketAllocator, pResponsePacket);
            pResponsePacket = NULL;
        }

        ntStatus = SMBSocketReceiveResponse(
                        pSocket,
                        packet.haveSignature && pSocket->pSessionKey != NULL,
                        packet.sequence + 1,
                        &pResponsePacket);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = pResponsePacket->pSMBHeader->error;
        if (ntStatus == STATUS_MORE_PROCESSING_REQUIRED)
        {
            ntStatus = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        /* Extract uid to use for remaining session setup
           requests */
        wUid = pResponsePacket->pSMBHeader->uid;

        ntStatus = UnmarshallSessionSetupResponse_WC_4(
                        pResponsePacket->pParams,
                        pResponsePacket->bufferLen - pResponsePacket->bufferUsed,
                        0,
                        &pResponseHeader,
                        &pSecurityBlob,
                        &pwszNativeOS,
                        &pwszNativeLanman,
                        &pwszNativeDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_SAFE_FREE_MEMORY(pSecurityBlob2);

        ntStatus = SMBGSSContextNegotiate(
                        hSMBGSSContext,
                        pSecurityBlob,
                        pResponseHeader->securityBlobLength,
                        &pSecurityBlob2,
                        &dwSecurityBlobLen2);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Work around issue seen with some Windows servers where
       signing is supported and required, but the server just
       reflects back our signature instead of signing properly.
       In this case, we no longer verify server signatures but
       continue to sign our own packets */
    if (packet.haveSignature && 
        (!memcmp(pResponsePacket->pSMBHeader->extra.securitySignature,
                 packet.pSMBHeader->extra.securitySignature,
                 sizeof(packet.pSMBHeader->extra.securitySignature))))
    {
        LWIO_LOG_WARNING("Server is exhibiting signing bug; ignoring signatures from server");
        RdrSocketSetIgnoreServerSignatures(pSocket, TRUE);
    }
    
    ntStatus = SMBGSSContextGetSessionKey(
                    hSMBGSSContext,
                    &pSessionKey,
                    &dwSessionKeyLength);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pSocket->pSessionKey && pSessionKey)
    {
        ntStatus = LwIoAllocateMemory(
                        dwSessionKeyLength,
                        OUT_PPVOID(&pSocket->pSessionKey));
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSocket->pSessionKey, pSessionKey, dwSessionKeyLength);

        pSocket->dwSessionKeyLength = dwSessionKeyLength;

        SMBSocketBeginSequence(pSocket);
    }

    *pUID = wUid;
    *ppSessionKey = pSessionKey;
    *pdwSessionKeyLength = dwSessionKeyLength;

cleanup:
    if (hSMBGSSContext)
    {
        SMBGSSContextFree(hSMBGSSContext);
    }

    if (pResponsePacket)
    {
        SMBPacketRelease(pSocket->hPacketAllocator, pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(
                pSocket->hPacketAllocator,
                packet.pRawBuffer,
                packet.bufferLen);
    }

    LWIO_SAFE_FREE_MEMORY(pSecurityBlob2);

    return ntStatus;

error:
    *pUID = 0;
    *ppSessionKey = NULL;
    *pdwSessionKeyLength = 0;

    LWIO_SAFE_FREE_MEMORY(pSessionKey);

    goto cleanup;
}
