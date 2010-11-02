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
 *        socket.h
 *
 * Abstract:
 *
 *        NFS3
 *
 *        Socket interface
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__


// Creates a socket task which will be referenced by the given task group,
// and will use task threads from the given thread pool
NTSTATUS
Nfs3SocketCreate(
    PNFS3_SOCKET*                   ppSocket,
    PLW_THREAD_POOL                 pPool,
    const PNFS3_TRANSPORT_CALLBACKS pCallbacks,
    PLW_TASK_GROUP                  pTaskGroup,
    int                             fd,
    PNFS3_SOCKADDR                  pCliAddr,
    PNFS3_SOCKADDR                  pSrvAddr
    );

// Read buffer is owned always by the caller.
// To avoid races, these two methods must be called only either on
// initialization or from #PFN_NFS3_TRANSPORT_DATA_READY callback.
VOID
Nfs3SocketSetReadBuffer(
    PNFS3_SOCKET            pSocket,
    const PNFS3_READ_BUFFER pReadBuffer
    );

VOID
Nfs3SocketGetReadBuffer(
    const PNFS3_SOCKET pSocket,
    PNFS3_READ_BUFFER  pReadBuffer
    );

#endif  // __SOCKET_H__

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/


