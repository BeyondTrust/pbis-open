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
 *        security.c
 *
 * Abstract:
 *
 *        Likewise Redirector (RDR)
 *
 *        Query Security Descriptor
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
RdrTransceiveQuerySecurity(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile
    );

static
BOOLEAN
RdrQuerySecurityComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveSetSecurity(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    ULONG ulLength
    );

static
BOOLEAN
RdrSetSecurityComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
VOID
RdrCancelQuerySecurity(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrQuerySecurity(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB pFile = NULL;

    pFile = IoFileGetContext(pIrp->FileHandle);

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelQuerySecurity, pContext);

    pContext->Continue = RdrQuerySecurityComplete;

    status = RdrTransceiveQuerySecurity(
        pContext,
        pFile);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveQuerySecurity(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t packetByteCount = 0;
    NT_TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    USHORT usSetup[0];
    SMB_NT_TRANS_QUERY_SECURITY_DESC_REQUEST_HEADER queryHeader = {0};
    USHORT usQueryHeaderOffset = 0;
    USHORT usSetDataOffset = 0;

    status = RdrAllocateContextPacket(pContext, 1024*64);
    BAIL_ON_NT_STATUS(status);
    
    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_NT_TRANSACT,
        0,
        0,
        pFile->pTree->tid,
        gRdrRuntime.SysPid,
        pFile->pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);
    
    pContext->Packet.pData = pContext->Packet.pParams + sizeof(NT_TRANSACTION_REQUEST_HEADER);
    pContext->Packet.bufferUsed += sizeof(NT_TRANSACTION_REQUEST_HEADER);
    pContext->Packet.pSMBHeader->wordCount = 19 + sizeof(usSetup)/sizeof(USHORT);

    pHeader = (NT_TRANSACTION_REQUEST_HEADER *) pContext->Packet.pParams;

    queryHeader.usFid = pFile->fid;
    queryHeader.securityInformation = pContext->pIrp->Args.QuerySetSecurity.SecurityInformation;

    status = WireMarshallTransactionRequestData(
        pContext->Packet.pData,
        pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
        &packetByteCount,
        usSetup, 
        sizeof(usSetup)/sizeof(USHORT),
        NULL,
        (PBYTE) &queryHeader,
        sizeof(queryHeader),
        &usQueryHeaderOffset,
        NULL,
        0,
        &usSetDataOffset);
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pContext->Packet.bufferUsed += packetByteCount;

    pHeader->usFunction = SMB_SUB_COMMAND_NT_TRANSACT_QUERY_SECURITY_DESC;
    pHeader->ulTotalParameterCount = sizeof(queryHeader);
    pHeader->ulTotalDataCount = 0;
    pHeader->ulMaxParameterCount = sizeof(queryHeader);
    pHeader->ulMaxDataCount = pContext->pIrp->Args.QuerySetSecurity.Length;
    pHeader->ucMaxSetupCount = sizeof(usSetup)/sizeof(USHORT);
    pHeader->ulParameterCount = sizeof(queryHeader);
    pHeader->ulParameterOffset = usQueryHeaderOffset + (pContext->Packet.pData - (PBYTE) pContext->Packet.pSMBHeader);
    pHeader->ulDataCount = 0;
    pHeader->ulDataOffset = 0;
    pHeader->ucSetupCount = sizeof(usSetup)/sizeof(USHORT);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrQuerySecurityComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pResponsePacket = pParam;
    PNT_TRANSACTION_SECONDARY_RESPONSE_HEADER pResponseHeader = NULL;
    ULONG ulTotalDataBytes = 0;
    ULONG ulDataBytes = 0;
    ULONG ulDataDisplacement = 0;

    status = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    if (pResponsePacket->pSMBHeader->wordCount > 0)
    {
        pResponseHeader = (NT_TRANSACTION_SECONDARY_RESPONSE_HEADER *) pResponsePacket->pParams;

        if ((PBYTE) pResponseHeader + sizeof(*pResponseHeader) >
            pResponsePacket->pRawBuffer + pResponsePacket->bufferUsed)
        {
            status = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(status);
        }

        ulTotalDataBytes = pResponseHeader->ulTotalDataCount;
        ulDataBytes = pResponseHeader->ulDataCount;
        ulDataDisplacement = pResponseHeader->ulDataDisplacement;

        if (ulTotalDataBytes > pContext->pIrp->Args.QuerySetSecurity.Length)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(status);
        }

        if (ulDataDisplacement + ulDataBytes > pContext->pIrp->Args.QuerySetSecurity.Length ||
            pResponseHeader->ulDataOffset > pResponsePacket->pNetBIOSHeader->len)
        {
            status = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(status);
        }

        memcpy(
            (PBYTE) pContext->pIrp->Args.QuerySetSecurity.SecurityDescriptor + ulDataDisplacement,
            (PBYTE) pResponsePacket->pSMBHeader + pResponseHeader->ulDataOffset,
            ulDataBytes);

        /* If we have remaining data, wait for the next response */
        if (ulTotalDataBytes > ulDataDisplacement + ulDataBytes)
        {
            status = STATUS_PENDING;
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            pContext->pIrp->IoStatusBlock.BytesTransferred = ulTotalDataBytes;
        }
    }

cleanup:

    RdrFreePacket(pResponsePacket);

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RdrFreeContext(pContext);
        return FALSE;
    }
    else
    {
        return TRUE;
    }

error:

    goto cleanup;
}

static
VOID
RdrCancelSetSecurity(
    PIRP pIrp,
    PVOID pParam
    )
{
}

NTSTATUS
RdrSetSecurity(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PRDR_CCB pFile = NULL;

    pFile = IoFileGetContext(pIrp->FileHandle);

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelSetSecurity, pContext);

    pContext->Continue = RdrSetSecurityComplete;

    status = RdrTransceiveSetSecurity(
        pContext,
        pFile,
        pIrp->Args.QuerySetSecurity.SecurityInformation,
        pIrp->Args.QuerySetSecurity.SecurityDescriptor,
        pIrp->Args.QuerySetSecurity.Length);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveSetSecurity(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    ULONG ulLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t packetByteCount = 0;
    NT_TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    USHORT usSetup[0];
    SMB_NT_TRANS_QUERY_SECURITY_DESC_REQUEST_HEADER setHeader = {0};
    USHORT usSetHeaderOffset = 0;
    USHORT usSetDataOffset = 0;

    status = RdrAllocateContextPacket(pContext, 1024*64);
    BAIL_ON_NT_STATUS(status);

    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_NT_TRANSACT,
        0,
        0,
        pFile->pTree->tid,
        gRdrRuntime.SysPid,
        pFile->pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(NT_TRANSACTION_REQUEST_HEADER);
    pContext->Packet.bufferUsed += sizeof(NT_TRANSACTION_REQUEST_HEADER);
    pContext->Packet.pSMBHeader->wordCount = 19 + sizeof(usSetup)/sizeof(USHORT);

    pHeader = (NT_TRANSACTION_REQUEST_HEADER *) pContext->Packet.pParams;

    setHeader.usFid = SMB_HTOL16(pFile->fid);
    setHeader.securityInformation = SMB_HTOL32(SecurityInformation);
    setHeader.reserved = 0;

    status = WireMarshallTransactionRequestData(
        pContext->Packet.pData,
        pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
        &packetByteCount,
        usSetup,
        sizeof(usSetup)/sizeof(USHORT),
        NULL,
        (PBYTE) &setHeader,
        sizeof(setHeader),
        &usSetHeaderOffset,
        (PBYTE) pSecurityDescriptor,
        ulLength,
        &usSetDataOffset);
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pContext->Packet.bufferUsed += packetByteCount;

    pHeader->usFunction = SMB_SUB_COMMAND_NT_TRANSACT_SET_SECURITY_DESC;
    pHeader->ulTotalParameterCount = sizeof(setHeader);
    pHeader->ulTotalDataCount = ulLength;
    pHeader->ulMaxParameterCount = sizeof(setHeader);
    pHeader->ulMaxDataCount = 0;
    pHeader->ucMaxSetupCount = sizeof(usSetup)/sizeof(USHORT);
    pHeader->ulParameterCount = sizeof(setHeader);
    pHeader->ulParameterOffset = usSetHeaderOffset + (pContext->Packet.pData - (PBYTE) pContext->Packet.pSMBHeader);
    pHeader->ulDataCount = ulLength;
    pHeader->ulDataOffset = usSetDataOffset + (pContext->Packet.pData - (PBYTE) pContext->Packet.pSMBHeader);
    pHeader->ucSetupCount = sizeof(usSetup)/sizeof(USHORT);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrSetSecurityComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pResponsePacket = pParam;

    BAIL_ON_NT_STATUS(status);

    status = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

 cleanup:

    RdrFreePacket(pResponsePacket);

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RdrFreeContext(pContext);
        return FALSE;
    }
    else
    {
        return TRUE;
    }

error:

    goto cleanup;
}


