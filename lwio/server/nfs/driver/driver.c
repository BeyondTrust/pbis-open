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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - NFS
 *
 *        Driver
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
NfsDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    );

static
VOID
NfsDriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    );

static
NTSTATUS
NfsInitialize(
    IO_DEVICE_HANDLE hDevice
    );

static
NTSTATUS
NfsShareBootstrap(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList
    );

static
NTSTATUS
NfsCreateDefaultSharePath(
    PCSTR pszDefaultSharePath
    );

static
NTSTATUS
NfsShutdown(
    VOID
    );

static
VOID
NfsUnblockOneWorker(
    IN PSMB_PROD_CONS_QUEUE pWorkQueue
    );

NTSTATUS
IO_DRIVER_ENTRY(nfs)(
    IN IO_DRIVER_HANDLE hDriver,
    IN ULONG ulInterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;
    PCSTR    pszName  = "nfs";
    PVOID    pDeviceContext = NULL;
    IO_DEVICE_HANDLE hDevice = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != ulInterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                    hDriver,
                    NULL,
                    NfsDriverShutdown,
                    NfsDriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDeviceCreate(
                    &hDevice,
                    hDriver,
                    pszName,
                    pDeviceContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsInitialize(hDevice);
    BAIL_ON_NT_STATUS(ntStatus);

    hDevice = NULL;

cleanup:

    return ntStatus;

error:

    if (hDevice)
    {
        IoDeviceDelete(&hDevice);
    }

    goto cleanup;
}

static
NTSTATUS
NfsDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:

            ntStatus = NfsDeviceCreate(
                            hDevice,
                            pIrp);
            break;

        case IRP_TYPE_CLOSE:

            ntStatus = NfsDeviceClose(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_READ:

            ntStatus = NfsDeviceRead(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_WRITE:

            ntStatus = NfsDeviceWrite(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_DEVICE_IO_CONTROL:

            ntStatus = NfsDeviceControlIo(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FS_CONTROL:

            ntStatus = NfsDeviceControlFS(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FLUSH_BUFFERS:

            ntStatus = NfsDeviceFlush(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_INFORMATION:

            ntStatus = NfsDeviceQueryInfo(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_SET_INFORMATION:

            ntStatus = NfsDeviceSetInfo(
                            hDevice,
                            pIrp);

            break;

        default:

            ntStatus = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
VOID
NfsDriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = NfsShutdown();
    BAIL_ON_NT_STATUS(ntStatus);

    if (gSMBNfsGlobals.hDevice)
    {
        IoDeviceDelete(&gSMBNfsGlobals.hDevice);
    }

cleanup:

    return;

error:

    if (ntStatus)
    {
        LWIO_LOG_ERROR("[nfs] driver failed to stop. [code: %d]", ntStatus);
    }

    goto cleanup;
}

static
NTSTATUS
NfsInitialize(
    IO_DEVICE_HANDLE hDevice
    )
{
    NTSTATUS ntStatus = 0;
    INT      iWorker = 0;

    memset(&gSMBNfsGlobals, 0, sizeof(gSMBNfsGlobals));

    pthread_mutex_init(&gSMBNfsGlobals.mutex, NULL);
    gSMBNfsGlobals.pMutex = &gSMBNfsGlobals.mutex;

    ntStatus = NfsInitConfig(&gSMBNfsGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsReadConfig(&gSMBNfsGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketCreateAllocator(
                    gSMBNfsGlobals.config.ulMaxNumPackets,
                    &gSMBNfsGlobals.hPacketAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsProdConsInitContents(
                    &gSMBNfsGlobals.workQueue,
                    gSMBNfsGlobals.config.ulMaxNumWorkItemsInQueue,
                    &NfsReleaseExecContextHandle);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsShareInit();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsShareInitList(&gSMBNfsGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsShareBootstrap(&gSMBNfsGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsElementsInit();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsProtocolInit(
                    &gSMBNfsGlobals.workQueue,
                    gSMBNfsGlobals.hPacketAllocator,
                    &gSMBNfsGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAllocateMemory(
                    gSMBNfsGlobals.config.ulNumWorkers * sizeof(LWIO_NFS_WORKER),
                    (PVOID*)&gSMBNfsGlobals.pWorkerArray);
    BAIL_ON_NT_STATUS(ntStatus);

    gSMBNfsGlobals.ulNumWorkers = gSMBNfsGlobals.config.ulNumWorkers;

    for (; iWorker < gSMBNfsGlobals.config.ulNumWorkers; iWorker++)
    {
        PLWIO_NFS_WORKER pWorker = &gSMBNfsGlobals.pWorkerArray[iWorker];

        pWorker->workerId = iWorker + 1;

        ntStatus = NfsWorkerInit(pWorker);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    gSMBNfsGlobals.hDevice = hDevice;

error:

    return ntStatus;
}

static
NTSTATUS
NfsShareBootstrap(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    wchar16_t wszPipeRootName[] = {'I','P','C','$',0};
    wchar16_t wszFileRootName[] = {'C','$',0};
    PSTR  pszFileSystemRoot = NULL;
    PWSTR pwszFileSystemRoot = NULL;
    PNFS_SHARE_INFO pShareInfo = NULL;

    ntStatus = NfsShareFindByName(
                    pShareList,
                    &wszPipeRootName[0],
                    &pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        wchar16_t wszPipeSystemRoot[] = LWIO_NFS_PIPE_SYSTEM_ROOT_W;
        wchar16_t wszServiceType[] = LWIO_NFS_SHARE_STRING_ID_IPC_W;
        wchar16_t wszDesc[] = {'R','e','m','o','t','e',' ','I','P','C',0};

        ntStatus = NfsShareAdd(
                            pShareList,
                            &wszPipeRootName[0],
                            &wszPipeSystemRoot[0],
                            &wszDesc[0],
                            NULL,
                            0,
                            &wszServiceType[0]);
    }
    else
    {
        NfsShareReleaseInfo(pShareInfo);
        pShareInfo = NULL;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsShareFindByName(
                        pShareList,
                        &wszFileRootName[0],
                        &pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        wchar16_t wszDesc[] =
                        {'D','e','f','a','u','l','t',' ','S','h','a','r','e',0};
        wchar16_t wszServiceType[] = LWIO_NFS_SHARE_STRING_ID_DISK_W;
        CHAR      szTmpFSRoot[] = LWIO_NFS_FILE_SYSTEM_ROOT_A;
        CHAR      szDefaultSharePath[] = LWIO_NFS_DEFAULT_SHARE_PATH_A;

        ntStatus = NfsCreateDefaultSharePath(&szDefaultSharePath[0]);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsAllocateStringPrintf(
                            &pszFileSystemRoot,
                            "%s%s%s",
                            &szTmpFSRoot[0],
                            (((szTmpFSRoot[strlen(&szTmpFSRoot[0])-1] == '/') ||
                              (szTmpFSRoot[strlen(&szTmpFSRoot[0])-1] == '\\')) ? "" : "\\"),
                            &szDefaultSharePath[0]);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsMbsToWc16s(pszFileSystemRoot, &pwszFileSystemRoot);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsShareAdd(
                        pShareList,
                        &wszFileRootName[0],
                        pwszFileSystemRoot,
                        &wszDesc[0],
                        NULL,
                        0,
                        &wszServiceType[0]);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    NFS_SAFE_FREE_MEMORY(pszFileSystemRoot);
    NFS_SAFE_FREE_MEMORY(pwszFileSystemRoot);

    return ntStatus;

error:

    LWIO_LOG_ERROR("Failed to bootstrap default shares. [error code: %d]",
                   ntStatus);

    goto cleanup;
}

static
NTSTATUS
NfsCreateDefaultSharePath(
    PCSTR pszDefaultSharePath
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR     pszPath = NULL;
    PSTR     pszCursor = NULL;
    BOOLEAN  bDirExists = FALSE;

    ntStatus = SMBAllocateString(pszDefaultSharePath, &pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    for (pszCursor = pszPath; pszCursor && *pszCursor; pszCursor++)
    {
        if (*pszCursor == '\\')
        {
            *pszCursor = '/';
        }
    }

    ntStatus = SMBCheckDirectoryExists(pszPath, &bDirExists);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!bDirExists)
    {
        ntStatus = SMBCreateDirectory(
                        pszPath,
                        S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pszPath)
    {
        SMBFreeString(pszPath);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
NfsShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    // TODO: All existing requests must be waited on to be completed before
    //       shutting down the worker queues.

    if (gSMBNfsGlobals.pMutex)
    {
        pthread_mutex_lock(gSMBNfsGlobals.pMutex);

        if (gSMBNfsGlobals.pWorkerArray)
        {
            INT iWorker = 0;

            for (iWorker = 0; iWorker < gSMBNfsGlobals.ulNumWorkers; iWorker++)
            {
                PLWIO_NFS_WORKER pWorker = &gSMBNfsGlobals.pWorkerArray[iWorker];

                NfsWorkerIndicateStop(pWorker);
            }

            // Must indicate stop for all workers before queueing the
            // unblocks.
            for (iWorker = 0; iWorker < gSMBNfsGlobals.ulNumWorkers; iWorker++)
            {
                NfsUnblockOneWorker(&gSMBNfsGlobals.workQueue);
            }

            for (iWorker = 0; iWorker < gSMBNfsGlobals.ulNumWorkers; iWorker++)
            {
                PLWIO_NFS_WORKER pWorker = &gSMBNfsGlobals.pWorkerArray[iWorker];

                NfsWorkerFreeContents(pWorker);
            }

            NfsFreeMemory(gSMBNfsGlobals.pWorkerArray);
            gSMBNfsGlobals.pWorkerArray = NULL;
        }

        NfsProtocolShutdown();

        NfsElementsShutdown();

        NfsShareFreeListContents(&gSMBNfsGlobals.shareList);

        NfsShareShutdown();

        NfsProdConsFreeContents(&gSMBNfsGlobals.workQueue);

        if (gSMBNfsGlobals.hPacketAllocator)
        {
            SMBPacketFreeAllocator(gSMBNfsGlobals.hPacketAllocator);
            gSMBNfsGlobals.hPacketAllocator = NULL;
        }

        NfsFreeConfigContents(&gSMBNfsGlobals.config);

        pthread_mutex_unlock(gSMBNfsGlobals.pMutex);
        gSMBNfsGlobals.pMutex = NULL;
    }

    return ntStatus;
}

static
VOID
NfsUnblockOneWorker(
    IN PSMB_PROD_CONS_QUEUE pWorkQueue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT pExecContext = NULL;

    ntStatus = NfsBuildEmptyExecContext(&pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsProdConsEnqueue(pWorkQueue, pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return;

error:

    if (pExecContext)
    {
        NfsReleaseExecContext(pExecContext);
    }

    goto cleanup;
}
