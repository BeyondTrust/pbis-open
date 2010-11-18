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
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Driver Entry Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "rdr.h"

static IO_DEVICE_HANDLE gDeviceHandle = NULL;

static
NTSTATUS
RdrReadConfig(
    PRDR_CONFIG pConfig
    );

static
NTSTATUS
RdrInitialize(
    VOID
    );

static
NTSTATUS
RdrShutdown(
    VOID
    );

static
VOID
RdrDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    RdrShutdown();

    if (gDeviceHandle)
    {
        IoDeviceDelete(&gDeviceHandle);
    }
}

static
NTSTATUS
RdrDriverDispatch1(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (pIrp->Type)
    {
    case IRP_TYPE_CLOSE:
        status = RdrClose(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_READ:
        status = RdrRead(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_WRITE:
        status = RdrWrite(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_DEVICE_IO_CONTROL:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    case IRP_TYPE_FS_CONTROL:
        status = RdrFsctl(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_FLUSH_BUFFERS:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    case IRP_TYPE_QUERY_INFORMATION:
        status = RdrQueryInformation(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_QUERY_DIRECTORY:
        status = RdrQueryDirectory(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_QUERY_VOLUME_INFORMATION:
        status = RdrQueryVolumeInformation(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_SET_INFORMATION:
        status = RdrSetInformation(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_QUERY_SECURITY:
        status = RdrQuerySecurity(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_SET_SECURITY:
        status =  RdrSetSecurity(DeviceHandle, pIrp);
        break;
    default:
        status = STATUS_UNSUCCESSFUL;
    }

    if (status != STATUS_PENDING)
    {
        pIrp->IoStatusBlock.Status = status;
    }

    return status;
}

static
NTSTATUS
RdrDriverDispatch2(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (pIrp->Type)
    {
    case IRP_TYPE_CLOSE:
        status = RdrClose2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_READ:
        status = RdrRead2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_WRITE:
        status = RdrWrite2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_DEVICE_IO_CONTROL:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    case IRP_TYPE_FS_CONTROL:
        status = RdrFsctl2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_FLUSH_BUFFERS:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    case IRP_TYPE_QUERY_INFORMATION:
        status = RdrQueryInformation2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_QUERY_DIRECTORY:
        status = RdrQueryDirectory2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_QUERY_VOLUME_INFORMATION:
        status = RdrQueryVolumeInformation2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_SET_INFORMATION:
        status = RdrSetInformation2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_QUERY_SECURITY:
        status = RdrQuerySecurity2(DeviceHandle, pIrp);
        break;
    case IRP_TYPE_SET_SECURITY:
        status = RdrSetSecurity2(DeviceHandle, pIrp);
        break;
    default:
        status = STATUS_UNSUCCESSFUL;
    }

    if (status != STATUS_PENDING)
    {
        pIrp->IoStatusBlock.Status = status;
    }

    return status;
}

static
NTSTATUS
RdrDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PVOID pFile = IoFileGetContext(pIrp->FileHandle);

    if (pFile)
    {
        switch(RDR_OBJECT_PROTOCOL(pFile))
        {
        case SMB_PROTOCOL_VERSION_1:
            return RdrDriverDispatch1(DeviceHandle, pIrp);
        case SMB_PROTOCOL_VERSION_2:
            return RdrDriverDispatch2(DeviceHandle, pIrp);
        default:
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }
    
    switch (pIrp->Type)
    {
    case IRP_TYPE_CREATE:
        ntStatus = RdrCreate(DeviceHandle, pIrp);
        break;
    default:
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    
error:

    return ntStatus;
}

extern NTSTATUS IO_DRIVER_ENTRY(rdr)(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    );

NTSTATUS
IO_DRIVER_ENTRY(rdr)(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(DriverHandle,
                                  NULL,
                                  RdrDriverShutdown,
                                  RdrDriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDeviceCreate(&gDeviceHandle,
                              DriverHandle,
                              "rdr",
                              NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RdrInitialize();
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
RdrInitialize(
    VOID
    )
{
    NTSTATUS status = 0;
    PLW_THREAD_POOL_ATTRIBUTES pAttrs = NULL;

    memset(&gRdrRuntime, 0, sizeof(gRdrRuntime));

    pthread_mutex_init(&gRdrRuntime.Lock, NULL);
    gRdrRuntime.bLockConstructed = TRUE;

    /* Pid used for SMB Header */
    gRdrRuntime.SysPid = getpid();

    /* Default config values */
    gRdrRuntime.config.bSmb2Enabled = FALSE;
    gRdrRuntime.config.bSigningEnabled = TRUE;
    gRdrRuntime.config.bSigningRequired = FALSE;
    gRdrRuntime.config.usIdleTimeout = RDR_IDLE_TIMEOUT;
    gRdrRuntime.config.usResponseTimeout = RDR_RESPONSE_TIMEOUT;
    gRdrRuntime.config.usEchoTimeout = RDR_ECHO_TIMEOUT;
    gRdrRuntime.config.usEchoInterval = RDR_ECHO_INTERVAL;
    gRdrRuntime.config.usConnectTimeout = RDR_CONNECT_TIMEOUT;
    gRdrRuntime.config.usMinCreditReserve = RDR_MIN_CREDIT_RESERVE;
    
    status = RdrReadConfig(&gRdrRuntime.config);
    BAIL_ON_NT_STATUS(status);

    status = SMBHashCreate(
                    19,
                    SMBHashCaselessWc16StringCompare,
                    SMBHashCaselessWc16String,
                    NULL,
                    &gRdrRuntime.pSocketHashByName);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCreateThreadPoolAttributes(&pAttrs);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCreateThreadPool(&gRdrRuntime.pThreadPool, pAttrs);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pSocketTaskGroup);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pSocketTimerGroup);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pSessionTimerGroup);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pTreeTimerGroup);
    BAIL_ON_NT_STATUS(status);

error:

    LwRtlFreeThreadPoolAttributes(&pAttrs);

    if (status)
    {
        RdrShutdown();
    }

    return status;
}

static
NTSTATUS
RdrShutdown(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    /* We need to run down all cached tress/session/sockets sitting around.
     * Set the global shutdown flag, then notify and wait for each task group
     * in turn.  Note that we do not cancel them because cancellation indicates
     * our intention to revive the object.
     */

    RdrSetShutdown();

    if (gRdrRuntime.pTreeTimerGroup)
    {
        LwRtlWakeTaskGroup(gRdrRuntime.pTreeTimerGroup);
        LwRtlWaitTaskGroup(gRdrRuntime.pTreeTimerGroup);
        LwRtlFreeTaskGroup(&gRdrRuntime.pTreeTimerGroup);
    }

    if (gRdrRuntime.pSessionTimerGroup)
    {
        LwRtlWakeTaskGroup(gRdrRuntime.pSessionTimerGroup);
        LwRtlWaitTaskGroup(gRdrRuntime.pSessionTimerGroup);
        LwRtlFreeTaskGroup(&gRdrRuntime.pSessionTimerGroup);
    }

    if (gRdrRuntime.pSocketTimerGroup)
    {
        LwRtlWakeTaskGroup(gRdrRuntime.pSocketTimerGroup);
        LwRtlWaitTaskGroup(gRdrRuntime.pSocketTimerGroup);
        LwRtlFreeTaskGroup(&gRdrRuntime.pSocketTimerGroup);
    }

    /* All socket tasks should have been canceled by this point,
     * so wait for them to finish shutting down
     */
    if (gRdrRuntime.pSocketTaskGroup)
    {
        LwRtlWaitTaskGroup(gRdrRuntime.pSocketTaskGroup);
    }

    /* All sockets are now gone, so free the socket hash */
    SMBHashSafeFree(&gRdrRuntime.pSocketHashByName);

    /* Free the thread pool */
    LwRtlFreeThreadPool(&gRdrRuntime.pThreadPool);

    /* Free the global mutex */
    if (gRdrRuntime.bLockConstructed)
    {
        pthread_mutex_destroy(&gRdrRuntime.Lock);
        gRdrRuntime.bLockConstructed = FALSE;
    }

    return status;
}

NTSTATUS
RdrCreateContext(
    PIRP pIrp,
    PRDR_OP_CONTEXT* ppContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;

    status = LW_RTL_ALLOCATE_AUTO(&pContext);
    BAIL_ON_NT_STATUS(status);

    LwListInit(&pContext->Link);

    pContext->pIrp = pIrp;
    
    if (pIrp)
    {
        LWIO_LOG_DEBUG("Created op context %p for IRP %p", pContext, pIrp);
    }
    else
    {
        LWIO_LOG_DEBUG("Created op context %p", pContext);
    }

    *ppContext = pContext;

error:

    return status;
}

NTSTATUS
RdrCreateContextArray(
    PIRP pIrp,
    ULONG ulCount,
    PRDR_OP_CONTEXT* ppContexts
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContexts = NULL;
    ULONG ulIndex = 0;

    status = LW_RTL_ALLOCATE_ARRAY_AUTO(&pContexts, ulCount);
    BAIL_ON_NT_STATUS(status);

    for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
    {
        LwListInit(&pContexts[ulIndex].Link);
        pContexts[ulIndex].pIrp = pIrp;
    }

    *ppContexts = pContexts;

error:

    return status;
}

VOID
RdrFreeContext(
    PRDR_OP_CONTEXT pContext
    )
{
    if (pContext)
    {
        LWIO_LOG_DEBUG("Freed op context %p", pContext);
        RTL_FREE(&pContext->Packet.pRawBuffer);
        RTL_FREE(&pContext);
    }
}

VOID
RdrFreeContextArray(
    PRDR_OP_CONTEXT pContexts,
    ULONG ulCount
    )
{
    ULONG ulIndex = 0;

    if (pContexts)
    {
        for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
        {
            RTL_FREE(&pContexts[ulIndex].Packet.pRawBuffer);
        }

        RTL_FREE(&pContexts);
    }
}

BOOLEAN
RdrContinueContext(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    if (pContext->Continue)
    {
        LWIO_LOG_DEBUG("Continuing context %p", pContext);
        return pContext->Continue(pContext, status, pParam);
    }
    else
    {
        return FALSE;
    }
}

NTSTATUS
RdrAllocatePacketBuffer(
    PSMB_PACKET pPacket,
    ULONG ulSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = RTL_ALLOCATE(&pPacket->pRawBuffer, BYTE, ulSize);
    BAIL_ON_NT_STATUS(status);

    pPacket->bufferLen = ulSize;

error:

    return status;
}

NTSTATUS
RdrAllocatePacket(
    ULONG ulSize,
    PSMB_PACKET* ppPacket
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSMB_PACKET pPacket = NULL;

    status = LW_RTL_ALLOCATE_AUTO(&pPacket);
    BAIL_ON_NT_STATUS(status);

    pPacket->refCount = 1;

    status = RdrAllocatePacketBuffer(pPacket, ulSize);
    BAIL_ON_NT_STATUS(status);

    *ppPacket = pPacket;

cleanup:

    return status;

error:

    RdrFreePacket(pPacket);

    goto cleanup;
}

NTSTATUS
RdrAllocateContextPacket(
    PRDR_OP_CONTEXT pContext,
    ULONG ulSize
    )
{
    RTL_FREE(&pContext->Packet.pRawBuffer);

    return RdrAllocatePacketBuffer(&pContext->Packet, ulSize);
}

VOID
RdrFreePacket(
    PSMB_PACKET pPacket
    )
{
    if (pPacket)
    {
        RTL_FREE(&pPacket->pRawBuffer);
        RTL_FREE(&pPacket);
    }
}

VOID
RdrContinueContextList(
    PLW_LIST_LINKS pList,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_OP_CONTEXT pContext = NULL;
    PLW_LIST_LINKS pLink = NULL;
    PLW_LIST_LINKS pNext = NULL;

    for (pLink = pList->Next; pLink != pList; pLink = pNext)
    {
        pNext = pLink->Next;
        pContext = LW_STRUCT_FROM_FIELD(pLink, RDR_OP_CONTEXT, Link);

        LwListRemove(pLink);

        if (RdrContinueContext(pContext, status, pParam))
        {
            LwListInsertBefore(pNext, pLink);
        }
    }
}

VOID
RdrNotifyContextList(
    PLW_LIST_LINKS pList,
    BOOLEAN bLocked,
    pthread_mutex_t* pMutex,
    NTSTATUS status,
    PVOID pParam
    )
{
    LW_LIST_LINKS List;
    PLW_LIST_LINKS pLink = NULL;
    BOOLEAN bWasLocked = bLocked;

    LWIO_LOCK_MUTEX(bLocked, pMutex);

    LwListInit(&List);

    while ((pLink = LwListRemoveHead(pList)))
    {
        LwListInsertTail(&List, pLink);
    }

    LWIO_UNLOCK_MUTEX(bLocked, pMutex);
    RdrContinueContextList(&List, status, pParam);
    LWIO_LOCK_MUTEX(bLocked, pMutex);

    while ((pLink = LwListRemoveHead(&List)))
    {
        LwListInsertTail(pList, pLink);
    }

    if (!bWasLocked)
    {
        LWIO_UNLOCK_MUTEX(bLocked, pMutex);
    }
}

BOOLEAN
RdrIsShutdownSet(
    VOID
    )
{
    BOOLEAN bLocked = FALSE;
    BOOLEAN bResult = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &gRdrRuntime.Lock);
    bResult = gRdrRuntime.bShutdown;
    LWIO_UNLOCK_MUTEX(bLocked, &gRdrRuntime.Lock);

    return bResult;
}

VOID
RdrSetShutdown(
    VOID
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &gRdrRuntime.Lock);
    gRdrRuntime.bShutdown = TRUE;
    LWIO_UNLOCK_MUTEX(bLocked, &gRdrRuntime.Lock);
}

static
NTSTATUS
RdrReadConfig(
    PRDR_CONFIG pConfig
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLWIO_CONFIG_REG pReg = NULL;
    BOOLEAN bUsePolicy = TRUE;
    DWORD dwValue = 0;

    status = LwIoOpenConfig(
        "Services\\lwio\\Parameters\\Drivers\\rdr",
        "Policy\\Services\\lwio\\Parameters\\Drivers\\rdr",
        &pReg);
    if (status)
    {
        LWIO_LOG_ERROR("Failed to access device configuration [error code: %u]",
                       status);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(status);

    LwIoReadConfigBoolean(
        pReg,
        "Smb2Enabled",
        bUsePolicy,
        &pConfig->bSmb2Enabled);

    LwIoReadConfigBoolean(
        pReg,
        "SigningEnabled",
        bUsePolicy,
        &pConfig->bSigningEnabled);

    LwIoReadConfigBoolean(
        pReg,
        "SigningRequired",
        bUsePolicy,
        &pConfig->bSigningRequired);

    if (LwIoReadConfigDword(
        pReg,
        "IdleTimeout",
        bUsePolicy,
        1,
        300,
        &dwValue) == STATUS_SUCCESS)
    {
        pConfig->usIdleTimeout = (USHORT) dwValue;
    }

    if (LwIoReadConfigDword(
        pReg,
        "ResponseTimeout",
        bUsePolicy,
        10,
        900,
        &dwValue) == STATUS_SUCCESS)
    {
        pConfig->usResponseTimeout = (USHORT) dwValue;
    }

    if (LwIoReadConfigDword(
        pReg,
        "EchoTimeout",
        bUsePolicy,
        5,
        900,
        &dwValue) == STATUS_SUCCESS)
    {
        pConfig->usEchoTimeout = (USHORT) dwValue;
    }

    if (LwIoReadConfigDword(
        pReg,
        "EchoInterval",
        bUsePolicy,
        30,
        1800,
        &dwValue) == STATUS_SUCCESS)
    {
        pConfig->usEchoInterval = (USHORT) dwValue;
    }

    if (LwIoReadConfigDword(
        pReg,
        "ConnectTimeout",
        bUsePolicy,
        5,
        900,
        &dwValue) == STATUS_SUCCESS)
    {
        pConfig->usConnectTimeout = (USHORT) dwValue;
    }

    if (LwIoReadConfigDword(
        pReg,
        "MinCreditReserve",
        bUsePolicy,
        1,
        100,
        &dwValue) == STATUS_SUCCESS)
    {
        pConfig->usMinCreditReserve = (USHORT) dwValue;
    }

cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    return status;

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
