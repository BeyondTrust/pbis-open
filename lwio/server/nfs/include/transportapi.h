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
 *        transportapi.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Transport
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __TRANSPORT_API_H__
#define __TRANSPORT_API_H__

#include <sys/socket.h>
#include "lwthreads.h"
#include "lwzct.h"

// Provided by Transport layer -- Opaque to Protocol layer:
typedef struct _NFS_TRANSPORT_HANDLE_DATA *NFS_TRANSPORT_HANDLE, **PNFS_TRANSPORT_HANDLE;
typedef struct _NFS_SOCKET *PNFS_SOCKET;

// Provided by Protocol layer's Protocol Transport Driver -- opaque to Transport layer:
// TODO: Perhaps rename PNFS_PROTOCOL_TRANSPORT_CONTEXT to PNFS_PTD_CONTEXT for Protocol Transport Driver.
//       If so, perhaps use _PTD_ for these three.  Plus perhaps change any "_TRANSPORT_PROTOCOL_" stuff to "_PTD_".
typedef struct _NFS_PROTOCOL_TRANSPORT_CONTEXT *PNFS_PROTOCOL_TRANSPORT_CONTEXT;
typedef struct _NFS_CONNECTION *PNFS_CONNECTION;
typedef struct _NFS_SEND_CONTEXT *PNFS_SEND_CONTEXT;

//
// Callbacks for protocol driver layer
//

///
/// Transport indication that there is a new connection.
///
/// This is called when a new connection arrives.  It should return
/// STATUS_ACCESS_DENIED or any other error to reject the connection.
///
/// If the connection is accepted, #PFN_NFS_TRANSPORT_CONNECTION_DONE
/// will be called unless the socket is explicitly closed with
/// NfsTransportSocketClose().
///
typedef NTSTATUS (*PFN_NFS_TRANSPORT_CONNECTION_NEW)(
    OUT PNFS_CONNECTION* ppConnection,
    IN PNFS_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext,
    IN PNFS_SOCKET pSocket
    );

///
/// Transport indication that there is data available.
///
/// This is called when at least the minimum requested number of bytes are
/// available in the buffer specified via NfsTransportSocketSetBuffer().
///
/// This call is always asynchronous.
///
typedef NTSTATUS (*PFN_NFS_TRANSPORT_CONNECTION_DATA)(
    IN PNFS_CONNECTION pConnection,
    IN ULONG BytesAvailable
    );

///
/// Transport indication that the connection is done.
///
/// This is called if #PFN_NFS_TRANSPORT_CONNECTION_NEW succeeded
/// and NfsTransportSocketClose() was not called to indicate
/// that the transport is done with the socket.
///
/// Note that any #PFN_NFS_TRANSPORT_SEND_DONE will happen
/// before this is called.
///
/// This call is always asynchronous.
///
/// The PTD cannot call back into the driver while and after processing
/// this.
///
typedef VOID (*PFN_NFS_TRANSPORT_CONNECTION_DONE)(
    IN PNFS_CONNECTION pConnection,
    IN NTSTATUS Status
    );

///
/// Transport indication that the send reply is queued.
///
/// This is called from within NfsTransportSocketSendReply() and
/// NfsTransportSocketSendZctReply() so that the PTD can perform signing,
/// etc. in the same order that the replies are queued.  If this returns
/// an error, NfsTransportSocketSendReply()/NfsTransportSocketSendZctReply()
/// will also return an error.
///
typedef NTSTATUS (*PFN_NFS_TRANSPORT_SEND_PREPARE)(
    IN PNFS_SEND_CONTEXT pSendContext
    );

///
/// Transport indication that the send reply is done.
///
/// This is called with a status code indicating whether or not
/// sending the reply was successful.  This called IFF
/// NfsTransportSocketSendReply()/NfsTransportSocketSendZctReply()
/// succeeded and NfsTransportSocketClose() has not been called.
///
/// This can be called synchronously from inside the send reply
/// functions or asynchronously.
///
typedef VOID (*PFN_NFS_TRANSPORT_SEND_DONE)(
    IN PNFS_SEND_CONTEXT pSendContext,
    IN NTSTATUS Status
    );

typedef struct _NFS_TRANSPORT_PROTOCOL_DISPATCH {
    PFN_NFS_TRANSPORT_CONNECTION_NEW pfnConnectionNew;
    PFN_NFS_TRANSPORT_CONNECTION_DATA pfnConnectionData;
    PFN_NFS_TRANSPORT_CONNECTION_DONE pfnConnectionDone;
    PFN_NFS_TRANSPORT_SEND_PREPARE pfnSendPrepare;
    PFN_NFS_TRANSPORT_SEND_DONE pfnSendDone;
} NFS_TRANSPORT_PROTOCOL_DISPATCH, *PNFS_TRANSPORT_PROTOCOL_DISPATCH;

NTSTATUS
NfsTransportInit(
    OUT PNFS_TRANSPORT_HANDLE phTransport,
    IN PNFS_TRANSPORT_PROTOCOL_DISPATCH pProtocolDispatch,
    IN OPTIONAL PNFS_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext
    );

VOID
NfsTransportShutdown(
    IN OUT NFS_TRANSPORT_HANDLE hTransport
    );

VOID
NfsTransportSocketGetAddress(
    IN PNFS_SOCKET pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT size_t* pAddressLength
    );

PCSTR
NfsTransportSocketGetAddressString(
    IN PNFS_SOCKET pSocket
    );

// For logging only.
int
NfsTransportSocketGetFileDescriptor(
    IN PNFS_SOCKET pSocket
    );

///
/// Set the socket buffer to use to receive data.
///
/// This will set the buffer into which to receive data.  If a NULL
/// or zero-size buffer is specified, no more data will be accepted.
/// Normally, this function can only be called from within
/// #PFN_NFS_TRANSPORT_CONNECTION_DATA.  However, if the buffer is
/// already NULL or zero, this can be called from outside to resume
/// receiving data.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
NTSTATUS
NfsTransportSocketSetBuffer(
    IN PNFS_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    );

///
/// Send a reply.
///
/// This will send a reply, queueing it as needed.  Once the reply is queued,
/// tt will synchronously call #PFN_NFS_TRANSPORT_SEND_PREPARE.  It may also
/// call #PFN_NFS_TRANSPORT_SEND_DONE if the send is done synchronously.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
NTSTATUS
NfsTransportSocketSendReply(
    IN PNFS_SOCKET pSocket,
    IN PNFS_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    );

///
/// Send a reply.
///
/// This will send a reply, queueing it as needed.  Once the reply is queued,
/// tt will synchronously call #PFN_NFS_TRANSPORT_SEND_PREPARE.  It may also
/// call #PFN_NFS_TRANSPORT_SEND_DONE if the send is done synchronously.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
NTSTATUS
NfsTransportSocketSendZctReply(
    IN PNFS_SOCKET pSocket,
    IN PNFS_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    );

///
/// Disconnect a socket.
///
/// This will disconnect a socket while keeping the memory referece valid.
/// It will asynchronously call #PFN_NFS_TRANSPORT_SEND_DONE and
/// #PFN_NFS_TRANSPORT_CONNECTION_DONE.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
VOID
NfsTransportSocketDisconnect(
    IN PNFS_SOCKET pSocket
    );

///
/// Close the socket.
///
/// This should be done when the PTD is completely done with the socket.
/// No #PFN_NFS_TRANSPORT_SEND_DONE or #PFN_NFS_TRANSPORT_CONNECTION_DONE
/// callbacks are triggered by this.  So the the PTD needs those callbacks,
/// it must call NfsTransportSocketDisconnect() and wait for the callbacks.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
VOID
NfsTransportSocketClose(
    IN OUT PNFS_SOCKET pSocket
    );

#endif /* __TRANSPORT_API_H__ */
