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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _LWIO_NFS_PROTOCOL_WORKER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    BOOLEAN bStop;

    ULONG   workerId;

    PSMB_PROD_CONS_QUEUE pWorkQueue;

} LWIO_NFS_PROTOCOL_WORKER_CONTEXT, *PLWIO_NFS_PROTOCOL_WORKER_CONTEXT;

typedef struct _LWIO_NFS_PROTOCOL_WORKER
{
    pthread_t  worker;
    pthread_t* pWorker;

    ULONG      workerId;

    LWIO_NFS_PROTOCOL_WORKER_CONTEXT context;

} LWIO_NFS_PROTOCOL_WORKER, *PLWIO_NFS_PROTOCOL_WORKER;

typedef struct _NFS_PROTOCOL_CONFIG
{
    BOOLEAN bEnableSmb2;
    BOOLEAN bEnableSigning;
    BOOLEAN bRequireSigning;
} NFS_PROTOCOL_CONFIG, *PNFS_PROTOCOL_CONFIG;

typedef struct _NFS_PROTOCOL_TRANSPORT_CONTEXT
{
    struct _NFS_PROTOCOL_API_GLOBALS* pGlobals; // Initialized on startup
    NFS_TRANSPORT_HANDLE              hTransport;
    NFS_TRANSPORT_PROTOCOL_DISPATCH   dispatch;
    NFS_CONNECTION_SOCKET_DISPATCH    socketDispatch;
    uuid_t                            guid;
    HANDLE                            hGssContext; // Initialized on first use
} NFS_PROTOCOL_TRANSPORT_CONTEXT;

typedef struct _NFS_PROTOCOL_API_GLOBALS
{
    pthread_mutex_t                mutex;

    PSMB_PROD_CONS_QUEUE           pWorkQueue;
    PLWIO_PACKET_ALLOCATOR         hPacketAllocator;
    PLWIO_NFS_SHARE_ENTRY_LIST     pShareList;
    NFS_PROTOCOL_CONFIG            config;
    NFS_PROTOCOL_TRANSPORT_CONTEXT transportContext;

} NFS_PROTOCOL_API_GLOBALS, *PNFS_PROTOCOL_API_GLOBALS;

#endif /* __STRUCTS_H__ */
