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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Transport API
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
NfsTransportInit(
    OUT PNFS_TRANSPORT_HANDLE phTransport,
    IN PNFS_TRANSPORT_PROTOCOL_DISPATCH pProtocolDispatch,
    IN OPTIONAL PNFS_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_TRANSPORT_HANDLE_DATA pTransport = NULL;

    ntStatus = NfsAllocateMemory(sizeof(*pTransport), OUT_PPVOID(&pTransport));
    BAIL_ON_NT_STATUS(ntStatus);

    pTransport->Dispatch = *pProtocolDispatch;
    pTransport->pContext = pProtocolDispatchContext;

    ntStatus = NfsListenerInit(&pTransport->Listener, pTransport);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    *phTransport = pTransport;

    return ntStatus;

error:

    NfsTransportShutdown(pTransport);
    pTransport = NULL;

    goto cleanup;
}

VOID
NfsTransportShutdown(
    IN OUT NFS_TRANSPORT_HANDLE hTransport
    )
{
    if (hTransport)
    {
        NfsListenerShutdown(&hTransport->Listener);
        NfsFreeMemory(hTransport);
    }
}

VOID
NfsTransportSocketGetAddress(
    IN PNFS_SOCKET pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT size_t* pAddressLength
    )
{
    NfsSocketGetAddress(pSocket, ppAddress, pAddressLength);
}

PCSTR
NfsTransportSocketGetAddressString(
    IN PNFS_SOCKET pSocket
    )
{
    return NfsSocketGetAddressString(pSocket);
}

int
NfsTransportSocketGetFileDescriptor(
    IN PNFS_SOCKET pSocket
    )
{
    return NfsSocketGetFileDescriptor(pSocket);
}

NTSTATUS
NfsTransportSocketSetBuffer(
    IN PNFS_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    )
{
    return NfsSocketSetBuffer(pSocket, pBuffer, Size, Minimum);
}

NTSTATUS
NfsTransportSocketSendReply(
    IN PNFS_SOCKET pSocket,
    IN PNFS_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    )
{
    return NfsSocketSendReply(pSocket, pSendContext, pBuffer, Size);
}

NTSTATUS
NfsTransportSocketSendZctReply(
    IN PNFS_SOCKET pSocket,
    IN PNFS_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    )
{
    return NfsSocketSendZctReply(pSocket, pSendContext, pZct);
}

VOID
NfsTransportSocketDisconnect(
    IN PNFS_SOCKET pSocket
    )
{
    return NfsSocketDisconnect(pSocket);
}

VOID
NfsTransportSocketClose(
    IN OUT PNFS_SOCKET pSocket
    )
{
    NfsSocketClose(pSocket);
}
