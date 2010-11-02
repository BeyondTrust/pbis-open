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
 *        initialize.c
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - nfs3
 *
 *        Initialization routines definition
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#include "includes.h"

#define READ_CONFIG_DWORD(keyName, minValue, maxValue)                        \
    ntStatus = LwIoReadConfigDword(pReg, #keyName, bUsePolicy, (minValue),    \
                                   (maxValue), &pConfig->ul##keyName);        \
    if (ntStatus)                                                             \
    {                                                                         \
        LWIO_LOG_ERROR("Invalid device registry configuration when reading "  \
                       #keyName ". [error code: %u]", ntStatus);              \
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;                         \
    }                                                                         \
    BAIL_ON_NT_STATUS(ntStatus);

/* forward declarations */

static
NTSTATUS
Nfs3ReadConfig(
    PNFS3_CONFIG pConfig
    );

/* implementation */

NTSTATUS
Nfs3Initialize(
    IO_DEVICE_HANDLE hDevice
    )
{
    NTSTATUS ntStatus = 0;
    INT iWorker = 0;
    ULONG ulNumCpus = LwRtlGetCpuCount();
    NFS3_TRANSPORT_CALLBACKS transportCallbacks = {
                    .pfnInitSocket = Nfs3TransportCbInitSocket,
                    .pfnDataReady = Nfs3TransportCbDataReady
                    };

    LWIO_LOG_INFO("%s", __func__);

    memset(&gNfs3Globals, 0, sizeof(gNfs3Globals));

    pthread_mutex_init(&gNfs3Globals.mutex, NULL);
    gNfs3Globals.pMutex = &gNfs3Globals.mutex;

    ntStatus = Nfs3ReadConfig(&gNfs3Globals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = Nfs3ProdConsInit(
                    gNfs3Globals.config.ulMaxWorkQueueItems,
                    &Nfs3ReleaseExecContextHandle,
                    &gNfs3Globals.pWorkQueue);
    BAIL_ON_NT_STATUS(ntStatus);

    // Workers initialization
    gNfs3Globals.ulNumWorkers = gNfs3Globals.config.ulWorkersToCoresRatio *
                                ulNumCpus;

    ntStatus = Nfs3AllocateMemory(
                    gNfs3Globals.ulNumWorkers * sizeof(NFS3_WORKER),
                    (PVOID*)&gNfs3Globals.pWorkerArray);
    BAIL_ON_NT_STATUS(ntStatus);

    for (iWorker = 0; iWorker < gNfs3Globals.ulNumWorkers; ++iWorker)
    {
        PNFS3_WORKER pWorker = &gNfs3Globals.pWorkerArray[iWorker];

        pWorker->workerId = iWorker + 1;

        ntStatus = Nfs3WorkerInit(pWorker, iWorker % ulNumCpus);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    // ... end

    ntStatus = Nfs3TransportCreate(&gNfs3Globals.pTransport, &transportCallbacks);
    BAIL_ON_NT_STATUS(ntStatus);

    gNfs3Globals.hDevice = hDevice;

cleanup:

    return ntStatus;

error:

    Nfs3ProdConsFree(&gNfs3Globals.pWorkQueue);
    Nfs3TransportFree(&gNfs3Globals.pTransport);

    goto cleanup;
}

// Sample registry access
static
NTSTATUS
Nfs3ReadConfig(
    PNFS3_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_CONFIG_REG pReg = NULL;
    BOOLEAN bUsePolicy = TRUE;

    ntStatus = LwIoOpenConfig(
                    "Services\\lwio\\Parameters\\Drivers\\nfs3",
                    "Policy\\Services\\lwio\\Parameters\\Drivers\\nfs3",
                    &pReg);
    if (ntStatus)
    {
        LWIO_LOG_ERROR("Invalid device registry configuration [error code: %u]",
                       ntStatus);
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    READ_CONFIG_DWORD(MaxWorkQueueItems,    1, 1000000);
    READ_CONFIG_DWORD(WorkersToCoresRatio,  1, 1024);
    READ_CONFIG_DWORD(TcpListenQueueLength, 1, 1000000);
    READ_CONFIG_DWORD(TcpServerPort,        0, 65535);

cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    return ntStatus;

error:

    goto cleanup;
}

VOID
Nfs3Shutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    LWIO_LOG_INFO("%s", __func__);

    if (gNfs3Globals.pMutex)
    {
        pthread_mutex_lock(gNfs3Globals.pMutex);

        // Shutdown driver-specific structures here
        // Make sure not to bail out without unlocking

        pthread_mutex_unlock(gNfs3Globals.pMutex);
        gNfs3Globals.pMutex = NULL;
    }

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (gNfs3Globals.hDevice)
    {
        IoDeviceDelete(&gNfs3Globals.hDevice);
    }

    return;

error:

    LWIO_LOG_ERROR("[nfs3] driver failed to stop. [code: %d]", 
                   ntStatus);
    
    goto cleanup;
}

NTSTATUS
Nfs3Refresh(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    LWIO_LOG_INFO("%s", __func__);

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

