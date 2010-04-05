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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Driver
 *
 *        prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// ccb.c

NTSTATUS
NfsCCBCreate(
    PNFS_IRP_CONTEXT pIrpContext,
    PNFS_CCB * ppCCB
    );

NTSTATUS
NfsCCBGet(
    IO_FILE_HANDLE FileHandle,
    PNFS_CCB*      ppCCB
    );

NTSTATUS
NfsCCBSet(
    IO_FILE_HANDLE FileHandle,
    PNFS_CCB       pCCB
    );

VOID
NfsCCBRelease(
    PNFS_CCB pCCB
    );

// config.c

NTSTATUS
NfsReadConfig(
    PLWIO_NFS_CONFIG pConfig
    );

NTSTATUS
NfsInitConfig(
    PLWIO_NFS_CONFIG pConfig
    );

VOID
NfsFreeConfigContents(
    PLWIO_NFS_CONFIG pConfig
    );

// devicecreate.c

NTSTATUS
NfsDeviceCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NfsAllocateIrpContext(
    PIRP pIrp,
    PNFS_IRP_CONTEXT * ppIrpContext
    );

VOID
NfsFreeIrpContext(
    PNFS_IRP_CONTEXT pIrpContext
    );

// device.c

NTSTATUS
NfsDeviceCreate(
    IO_DEVICE_HANDLE hDevice,
    PIRP      pIrp
    );

NTSTATUS
NfsDeviceClose(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
NfsDeviceRead(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
NfsDeviceWrite(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
NfsDeviceControlIO(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
NfsDeviceControlFS(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
NfsDeviceFlush(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
NfsDeviceQueryInfo(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
NfsDeviceSetInfo(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

// deviceio.c

NTSTATUS
NfsDeviceControlIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP             pIrp
    );

// nfsshares.c

NTSTATUS
NfsShareGetServiceStringId(
    IN  SHARE_SERVICE  service,
    OUT PSTR*          ppszService
    );

NTSTATUS
NfsShareGetServiceId(
    IN  PCSTR          pszService,
    OUT SHARE_SERVICE* pService
    );

NTSTATUS
NfsShareDevCtlAdd(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    );

NTSTATUS
NfsShareDevCtlDelete(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    );

NTSTATUS
NfsShareDevCtlEnum(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    );

NTSTATUS
NfsShareDevCtlGetInfo(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    );

NTSTATUS
NfsShareDevCtlSetInfo(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    );

// nfsstats.c

NTSTATUS
NfsProcessStatistics(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    );

// nfsworker.c

NTSTATUS
NfsWorkerInit(
    PLWIO_NFS_WORKER      pWorker
    );

VOID
NfsWorkerIndicateStop(
    PLWIO_NFS_WORKER pWorker
    );

VOID
NfsWorkerFreeContents(
    PLWIO_NFS_WORKER pWorker
    );

