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
RdrDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    
    switch (pIrp->Type)
    {
    case IRP_TYPE_CREATE:
        ntStatus = RdrCreate(
            DeviceHandle,
            pIrp
            );
        break;
        
    case IRP_TYPE_CLOSE:
        ntStatus = RdrClose(
            DeviceHandle,
            pIrp
            );
        break;
        
        
    case IRP_TYPE_READ:
        ntStatus = RdrRead(
            DeviceHandle,
            pIrp
            );
        break;
        
    case IRP_TYPE_WRITE:
        ntStatus = RdrWrite(
            DeviceHandle,
            pIrp
            );
        break;
        
    case IRP_TYPE_DEVICE_IO_CONTROL:
        ntStatus = STATUS_NOT_IMPLEMENTED;
        break;
        
    case IRP_TYPE_FS_CONTROL:
        ntStatus = RdrFsctl(
            DeviceHandle,
            pIrp
            );
        break;

    case IRP_TYPE_FLUSH_BUFFERS:
        ntStatus = STATUS_NOT_IMPLEMENTED;
        break;

    case IRP_TYPE_QUERY_INFORMATION:
        ntStatus = RdrQueryInformation(
            DeviceHandle,
            pIrp
            );
        break;
    case IRP_TYPE_QUERY_DIRECTORY:
        ntStatus = RdrQueryDirectory(
            DeviceHandle,
            pIrp
            );
        break;
    case IRP_TYPE_QUERY_VOLUME_INFORMATION:
        ntStatus = RdrQueryVolumeInformation(
            DeviceHandle,
            pIrp);
        break;
    case IRP_TYPE_SET_INFORMATION:
        ntStatus = RdrSetInformation(
            DeviceHandle,
            pIrp
            );
        break;
    case IRP_TYPE_QUERY_SECURITY:
        ntStatus = RdrQuerySecurity(
            DeviceHandle,
            pIrp
            );
        break;
    case IRP_TYPE_SET_SECURITY:
        ntStatus =  RdrSetSecurity(
            DeviceHandle,
            pIrp);
        break;
    default:
        ntStatus = STATUS_UNSUCCESSFUL;
    }
    
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
    NTSTATUS ntStatus = 0;
    PLW_THREAD_POOL_ATTRIBUTES pAttrs = NULL;

    memset(&gRdrRuntime, 0, sizeof(gRdrRuntime));

    pthread_mutex_init(&gRdrRuntime.Lock, NULL);
    gRdrRuntime.bLockConstructed = TRUE;

    /* Pid used for SMB Header */

    gRdrRuntime.SysPid = getpid();
    gRdrRuntime.config.usResponseTimeout = RDR_RESPONSE_TIMEOUT;
    gRdrRuntime.config.usEchoTimeout = RDR_ECHO_TIMEOUT;
    gRdrRuntime.config.usEchoInterval = RDR_ECHO_INTERVAL;
    gRdrRuntime.config.usConnectTimeout = RDR_CONNECT_TIMEOUT;
    
    ntStatus = SMBHashCreate(
                    19,
                    SMBHashCaselessWc16StringCompare,
                    SMBHashCaselessWc16String,
                    NULL,
                    &gRdrRuntime.pSocketHashByName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateThreadPoolAttributes(&pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateThreadPool(&gRdrRuntime.pThreadPool, pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pSocketTaskGroup);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pSocketTimerGroup);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pSessionTimerGroup);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTaskGroup(gRdrRuntime.pThreadPool, &gRdrRuntime.pTreeTimerGroup);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LwRtlFreeThreadPoolAttributes(&pAttrs);

    if (ntStatus)
    {
        RdrShutdown();
    }

    return ntStatus;
}

static
NTSTATUS
RdrShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

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

    return ntStatus;
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
    
    *ppContext = pContext;

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
        RTL_FREE(&pContext->Packet.pRawBuffer);        
        RTL_FREE(&pContext);
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

NTSTATUS
RdrConvertPath(
    PCWSTR pwszIoPath,
    PWSTR* ppwszHost,
    PWSTR* ppwszShare,
    PWSTR* ppwszFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    enum
    {
        STATE_START,
        STATE_HOST,
        STATE_SHARE,
        STATE_FILE
    } state = STATE_START;
    ULONG ulSrcIndex = 0;
    ULONG ulDstIndex = 0;
    ULONG ulHostIndex = 0;
    PWSTR pwszHost = NULL;
    PWSTR pwszShare = NULL;
    PWSTR pwszFile = NULL;
    

    for (ulSrcIndex = 0; pwszIoPath[ulSrcIndex]; ulSrcIndex++)
    {
        switch (state)
        {
        case STATE_START:
            status = RTL_ALLOCATE(
                &pwszHost,
                WCHAR,
                (LwRtlWC16StringNumChars(pwszIoPath) + 1) * sizeof(WCHAR));
            BAIL_ON_NT_STATUS(status);
            if (pwszIoPath[ulSrcIndex] != '/')
            {
                status = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(status);
            }
            ulDstIndex = 0;
            state = STATE_HOST;
            break;
        case STATE_HOST:
            if (pwszIoPath[ulSrcIndex] == '/')
            {
                status = RTL_ALLOCATE(
                    &pwszShare,
                    WCHAR,
                    (LwRtlWC16StringNumChars(pwszHost) + 
                     LwRtlWC16StringNumChars(pwszIoPath + ulSrcIndex) + 4) * sizeof(WCHAR));
                BAIL_ON_NT_STATUS(status);
                ulDstIndex = 0;
                pwszShare[ulDstIndex++] = '\\';
                pwszShare[ulDstIndex++] = '\\';
                for (ulHostIndex = 0; pwszHost[ulHostIndex] && pwszHost[ulHostIndex] != '@'; ulHostIndex++)
                {
                    pwszShare[ulDstIndex++] = pwszHost[ulHostIndex];
                }
                pwszShare[ulDstIndex++] = '\\';
                state = STATE_SHARE;
            }
            else
            {
                pwszHost[ulDstIndex++] = pwszIoPath[ulSrcIndex];
            }
            break;
        case STATE_SHARE:
            if (pwszIoPath[ulSrcIndex] == '/')
            {
                status = RTL_ALLOCATE(
                    &pwszFile,
                    WCHAR,
                    (LwRtlWC16StringNumChars(pwszIoPath + ulSrcIndex) + 1) * sizeof(WCHAR));
                BAIL_ON_NT_STATUS(status);
                ulDstIndex = 0;
                pwszFile[ulDstIndex++] = '\\';
                state = STATE_FILE;
            }
            else
            {
                pwszShare[ulDstIndex++] = pwszIoPath[ulSrcIndex];
            }
            break;
        case STATE_FILE:
            if (pwszIoPath[ulSrcIndex] == '/')
            {
                if (pwszFile[ulDstIndex-1] != '\\')
                {
                    pwszFile[ulDstIndex++] = '\\';
                }
            }
            else
            {
                pwszFile[ulDstIndex++] = pwszIoPath[ulSrcIndex];
            }
            break;
        }
    }

    if (!pwszHost || !pwszShare)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    if (!pwszFile)
    {
        status = LwRtlWC16StringAllocateFromCString(&pwszFile, "/");
        BAIL_ON_NT_STATUS(status);
    }

    if (ppwszHost)
    {
        *ppwszHost = pwszHost;
    }
    else
    {
        RTL_FREE(&pwszHost);
    }

    if (ppwszShare)
    {
        *ppwszShare = pwszShare;
    }
    else
    {
        RTL_FREE(&pwszShare);
    }

    if (ppwszFile)
    {
        *ppwszFile = pwszFile;
    }
    else
    {
        RTL_FREE(&pwszFile);
    }

cleanup:

    return status;

error:

    RTL_FREE(&pwszHost);
    RTL_FREE(&pwszShare);
    RTL_FREE(&pwszFile);

    goto cleanup;
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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
