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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _LWIO_NFS_CONFIG
{
    ULONG ulNumWorkers;
    ULONG ulMaxNumPackets;
    ULONG ulMaxNumWorkItemsInQueue;

} LWIO_NFS_CONFIG, *PLWIO_NFS_CONFIG;

typedef struct _NFS_CCB
{
    LONG                    refCount;

    CCB_TYPE                CcbType;
    UNICODE_STRING          AbsolutePathName;
    ACCESS_MASK             DesiredAccess;
    LONG64                  AllocationSize;
    FILE_ATTRIBUTES         FileAttributes;
    FILE_SHARE_FLAGS        ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS     CreateOptions;

    struct _NFS_CCB *       pNext;

} NFS_CCB, *PNFS_CCB;

typedef struct _NFS_IRP_CONTEXT
{
    PIRP             pIrp;
    IO_DEVICE_HANDLE targetDeviceHandle;
    UNICODE_STRING   rootPathName;
    UNICODE_STRING   relativePathName;
    UNICODE_STRING   absolutePathName;

} NFS_IRP_CONTEXT, *PNFS_IRP_CONTEXT;

typedef struct _LWIO_NFS_WORKER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    BOOLEAN bStop;

    ULONG   workerId;

} LWIO_NFS_WORKER_CONTEXT, *PLWIO_NFS_WORKER_CONTEXT;

typedef struct _LWIO_NFS_WORKER
{
    pthread_t  worker;
    pthread_t* pWorker;

    ULONG      workerId;

    LWIO_NFS_WORKER_CONTEXT context;

} LWIO_NFS_WORKER, *PLWIO_NFS_WORKER;

typedef struct _LWIO_NFS_RUNTIME_GLOBALS
{
    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    LWIO_NFS_CONFIG           config;

    LWIO_NFS_SHARE_ENTRY_LIST shareList;

    SMB_PROD_CONS_QUEUE       workQueue;
    ULONG                     ulMaxNumWorkItemsInQueue;

    PLWIO_NFS_WORKER          pWorkerArray;
    ULONG                     ulNumWorkers;

    PLWIO_PACKET_ALLOCATOR    hPacketAllocator;

    IO_DEVICE_HANDLE          hDevice;

} LWIO_NFS_RUNTIME_GLOBALS, *PLWIO_NFS_RUNTIME_GLOBALS;


#endif /* __STRUCTS_H__ */
