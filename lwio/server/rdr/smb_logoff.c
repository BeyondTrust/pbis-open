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
 *        logoff.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Client Logoff Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

uint32_t
Logoff(
    PSMB_SESSION pSession
    )
{
    uint32_t ntStatus = 0;
    SMB_PACKET packet = {0};
    PSMB_PACKET pResponsePacket = NULL;

    /* @todo: make initial length configurable */
    ntStatus = SMBPacketBufferAllocate(
                    pSession->pSocket->hPacketAllocator,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_LOGOFF_ANDX,
                0,
                0,
                0,
                gRdrRuntime.SysPid,
                pSession->uid,
                0,
                TRUE,
                &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.pSMBHeader->wordCount = 2;

    packet.pData = packet.pParams;
    packet.bufferUsed += sizeof(uint16_t); /* ByteCount */
    memset(packet.pData, 0, sizeof(uint16_t));

    // no byte order conversions necessary (due to zeros)

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSocketSend(pSession->pSocket, &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSocketReceiveResponse(
                    pSession->pSocket,
                    packet.haveSignature,
                    packet.sequence + 1,
                    &pResponsePacket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (pResponsePacket)
    {
        SMBPacketRelease(pSession->pSocket->hPacketAllocator, pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(
                pSession->pSocket->hPacketAllocator,
                packet.pRawBuffer,
                packet.bufferLen);
    }

    return ntStatus;

error:
    goto cleanup;
}
