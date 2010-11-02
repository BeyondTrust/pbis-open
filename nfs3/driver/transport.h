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
 *        transport.h
 *
 * Abstract:
 *
 *        NFS3
 *
 *        Transport interface
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__


typedef struct __NFS3_TRANSPORT NFS3_TRANSPORT, *PNFS3_TRANSPORT;
typedef struct __NFS3_LISTENER  NFS3_LISTENER,  *PNFS3_LISTENER;
typedef struct __NFS3_SOCKET    NFS3_SOCKET,    *PNFS3_SOCKET;

typedef struct __NFS3_SEND_ITEM
{
    LW_LIST_LINKS links;
    PVOID         pBuffer;
    ULONG         ulLength;
    ULONG         ulOffset;

} NFS3_SEND_ITEM, *PNFS3_SEND_ITEM;

typedef struct __NFS3_READ_BUFFER
{
    PVOID pBuf;
    ULONG ulBufLen;
    ULONG ulMinimum;
    ULONG ulOffset;

} NFS3_READ_BUFFER, *PNFS3_READ_BUFFER;

typedef NTSTATUS (*PFN_NFS3_TRANSPORT_INIT_SOCKET)  (PNFS3_SOCKET pSocket);
typedef NTSTATUS (*PFN_NFS3_TRANSPORT_DATA_READY)   (PNFS3_SOCKET pSocket);

typedef struct __NFS3_TRANSPORT_CALLBACKS
{
    PFN_NFS3_TRANSPORT_INIT_SOCKET  pfnInitSocket;
    PFN_NFS3_TRANSPORT_DATA_READY   pfnDataReady;

} NFS3_TRANSPORT_CALLBACKS, *PNFS3_TRANSPORT_CALLBACKS;


NTSTATUS
Nfs3TransportCreate(
    PNFS3_TRANSPORT* ppTransport,
    const PNFS3_TRANSPORT_CALLBACKS pCallbacks
    );

VOID
Nfs3TransportFree(
    PNFS3_TRANSPORT* ppTransport
    );


#endif  // __TRANSPORT_H__

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

