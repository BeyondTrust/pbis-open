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

#include "includes.h"


struct __NFS3_SOCKET
{
    int              fd;
    NFS3_SOCKADDR    cliAddr;
    NFS3_SOCKADDR    srvAddr;
    
    PLW_TASK         pTask;
    
    // Read buffer
    NFS3_READ_BUFFER readBuffer;

    // Send queue - (NFS3_SEND_ITEM.SendLinks)
    LW_LIST_LINKS    sendItems;
    pthread_mutex_t  sendItemsLock;

    NFS3_TRANSPORT_CALLBACKS callbacks;
};


static
VOID
Nfs3SocketFree(
    PNFS3_SOCKET*   ppSocket
    );

static
VOID
Nfs3SocketProcessTask(
    PLW_TASK            pTask,
    PVOID               pDataContext,
    LW_TASK_EVENT_MASK  wakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64             pllTime
    );

static
NTSTATUS
Nfs3SocketProcessTaskInit(
    PNFS3_SOCKET pSocket
    );

static
NTSTATUS
Nfs3SocketProcessTaskRead(
    PNFS3_SOCKET pSocket,
    PLW_TASK_EVENT_MASK pWaitMask
    );

static
NTSTATUS
Nfs3SocketProcessTaskWrite(
    PNFS3_SOCKET pSocket,
    PLW_TASK_EVENT_MASK pWaitMask
    );

static
NTSTATUS
Nfs3SocketReadWrite(
    int     fd,
    PVOID   pBuffer,
    ULONG   ulLength,
    PULONG  pulBytesTransferred,
    BOOLEAN bRead
    );


NTSTATUS
Nfs3SocketCreate(
    PNFS3_SOCKET*                   ppSocket,
    PLW_THREAD_POOL                 pPool,
    const PNFS3_TRANSPORT_CALLBACKS pCallbacks,
    PLW_TASK_GROUP                  pTaskGroup,
    int                             fd,
    PNFS3_SOCKADDR                  pCliAddr,
    PNFS3_SOCKADDR                  pSrvAddr
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int syserr = 0;
    PNFS3_SOCKET pSocket = NULL;

    syserr = fcntl(fd, F_SETFL, O_NONBLOCK);
    BAIL_ON_SYS_ERROR(syserr);

    ntStatus = Nfs3AllocateMemory(sizeof(*pSocket), (PVOID*)&pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pSocket->sendItems);
    if ((syserr = pthread_mutex_init(&pSocket->sendItemsLock, NULL)) != 0)
    {
        ntStatus = LwErrnoToNtStatus(syserr);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSocket->fd = fd;
    pSocket->callbacks = *pCallbacks;

    memcpy(&pSocket->cliAddr, pCliAddr, sizeof(pSocket->cliAddr));
    memcpy(&pSocket->srvAddr, pSrvAddr, sizeof(pSocket->srvAddr));

    ntStatus = LwRtlCreateTask(
                    pPool,
                    &pSocket->pTask,
                    pTaskGroup,
                    Nfs3SocketProcessTask,
                    pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    LwRtlWakeTask(pSocket->pTask);

cleanup:

    *ppSocket = pSocket;

    return ntStatus;

error:

    Nfs3SocketFree(&pSocket);

    goto cleanup;
}

VOID
Nfs3SocketSetReadBuffer(
    PNFS3_SOCKET            pSocket,
    const PNFS3_READ_BUFFER pReadBuffer
    )
{
    NFS3_ASSERT(pReadBuffer->pBuf && 
                (pReadBuffer->ulMinimum <= pReadBuffer->ulBufLen));

    pSocket->readBuffer = *pReadBuffer;
}

VOID
Nfs3SocketGetReadBuffer(
    const PNFS3_SOCKET pSocket,
    PNFS3_READ_BUFFER  pReadBuffer
    )
{
    *pReadBuffer = pSocket->readBuffer;
}

static
VOID
Nfs3SocketFree(
    PNFS3_SOCKET*   ppSocket
    )
{
    PNFS3_SOCKET pSocket = *ppSocket;

    if (pSocket)
    {
        if (pSocket->pTask)
        {
            LwRtlCancelTask(pSocket->pTask);
            LwRtlWaitTask(pSocket->pTask);
        }

        // TODO - free writeItems / read buffer?

        Nfs3FreeMemory((PVOID*)&pSocket);
    }

    *ppSocket = pSocket;
}

static
VOID
Nfs3SocketProcessTask(
    PLW_TASK            pTask,
    PVOID               pDataContext,
    LW_TASK_EVENT_MASK  wakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64             pllTime
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PNFS3_SOCKET        pSocket = (PNFS3_SOCKET) pDataContext;
    LW_TASK_EVENT_MASK  waitMask = 0;

    if (IsSetFlag(wakeMask, LW_TASK_EVENT_INIT))
    {
        ntStatus = Nfs3SocketProcessTaskInit(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (IsSetFlag(wakeMask, LW_TASK_EVENT_CANCEL))
    {
        if (pSocket->fd >= 0)
        {
            close(pSocket->fd);
        }

        waitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    if (IsSetFlag(wakeMask, LW_TASK_EVENT_FD_WRITABLE))
    {
        ntStatus = Nfs3SocketProcessTaskWrite(pSocket, &waitMask);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    
    if (IsSetFlag(wakeMask, LW_TASK_EVENT_FD_READABLE))
    {
        ntStatus = Nfs3SocketProcessTaskRead(pSocket, &waitMask);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    
    if (!waitMask)
    {
        waitMask = LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE;
    }

cleanup:

    // waitMask can only be 0 (aka COMPLETE) for EVENT_CANCEL.
    NFS3_ASSERT(waitMask ||
                ((LW_TASK_EVENT_COMPLETE == waitMask) &&
                 IsSetFlag(wakeMask, LW_TASK_EVENT_CANCEL)));

    *pWaitMask = waitMask;

    return;

error:

    LwRtlCancelTask(pSocket->pTask);

    waitMask = LW_TASK_EVENT_EXPLICIT;

    goto cleanup;
}

static
NTSTATUS
Nfs3SocketProcessTaskInit(
    PNFS3_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = pSocket->callbacks.pfnInitSocket(pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    // Register file descriptor with thread pool for read/write.
    ntStatus = LwRtlSetTaskFd(
                    pSocket->pTask,
                    pSocket->fd,
                    LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
Nfs3SocketProcessTaskWrite(
    PNFS3_SOCKET pSocket,
    PLW_TASK_EVENT_MASK pWaitMask
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bWritten = FALSE;
    BOOLEAN bInLock = FALSE;

    NFS3_LOCK_MUTEX(bInLock, &pSocket->sendItemsLock);

    while (!LwListIsEmpty(&pSocket->sendItems))
    {
        PLW_LIST_LINKS pLinks = pSocket->sendItems.Next;
        PNFS3_SEND_ITEM pSendItem = LW_STRUCT_FROM_FIELD(pLinks, NFS3_SEND_ITEM,
                                                         links);
        ULONG ulBytesRemaining = 0;
        ULONG ulBytesTransferred = 0;
        PVOID pCurrent = LwRtlOffsetToPointer(pSendItem->pBuffer,
                                              pSendItem->ulOffset);

        NFS3_ASSERT(pSendItem->ulLength > pSendItem->ulOffset);

        ulBytesRemaining = pSendItem->ulLength - pSendItem->ulOffset;

        ntStatus = Nfs3SocketReadWrite(pSocket->fd, pCurrent, ulBytesRemaining, 
                                       &ulBytesTransferred, FALSE);
        if (ntStatus == STATUS_MORE_PROCESSING_REQUIRED)    // EWOULDBLOCK
        {
            ntStatus = STATUS_SUCCESS;
            break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        bWritten = TRUE;
        pSendItem->ulOffset += ulBytesTransferred;
        NFS3_ASSERT(pSendItem->ulOffset <= pSendItem->ulLength);
        ulBytesRemaining = pSendItem->ulLength - pSendItem->ulOffset;

        if (!ulBytesRemaining)
        {
            LwListRemove(&pSendItem->links);
            Nfs3FreeMemory((PVOID*)&pSendItem);
        }
    }

    if (bWritten)
    {
        *pWaitMask = LW_TASK_EVENT_YIELD;
    }
    
cleanup:

    NFS3_UNLOCK_MUTEX(bInLock, &pSocket->sendItemsLock);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
Nfs3SocketProcessTaskRead(
    PNFS3_SOCKET pSocket,
    PLW_TASK_EVENT_MASK pWaitMask
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulBytesRemaining = 0;
    PNFS3_READ_BUFFER pReadBuffer = &pSocket->readBuffer;
    ULONG ulInitialOffset = pReadBuffer->ulOffset;

    NFS3_ASSERT(pReadBuffer->ulOffset <= pReadBuffer->ulBufLen);
    ulBytesRemaining = pReadBuffer->ulBufLen - pReadBuffer->ulOffset;
    while (ulBytesRemaining > 0)
    {
        PVOID pCurrent = LwRtlOffsetToPointer(pReadBuffer->pBuf, 
                                              pReadBuffer->ulOffset);
        ULONG ulBytesTransferred = 0;

        ntStatus = Nfs3SocketReadWrite(pSocket->fd, pCurrent, ulBytesRemaining, 
                                       &ulBytesTransferred, TRUE);
        if (ntStatus == STATUS_MORE_PROCESSING_REQUIRED)    // EWOULDBLOCK
        {
            ntStatus = STATUS_SUCCESS;
            break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
        
        if (ulBytesTransferred == 0)
        {
            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pReadBuffer->ulOffset += ulBytesTransferred;
        NFS3_ASSERT(pReadBuffer->ulOffset <= pReadBuffer->ulBufLen);
        ulBytesRemaining = pReadBuffer->ulBufLen - pReadBuffer->ulOffset;
    }

    if (pReadBuffer->ulOffset > ulInitialOffset)
    {
        *pWaitMask = LW_TASK_EVENT_YIELD;
    }
    
    if ((pReadBuffer->ulOffset > ulInitialOffset) &&
        (pReadBuffer->ulOffset >= pReadBuffer->ulMinimum))
    {
        ntStatus = pSocket->callbacks.pfnDataReady(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
NTSTATUS
Nfs3SocketReadWrite(
    int     fd,
    PVOID   pBuffer,
    ULONG   ulLength,
    PULONG  pulBytesTransferred,
    BOOLEAN bRead
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ssize_t  bytesTransferred = 0;

    for (;;)
    {
        if (bRead)
        {
            bytesTransferred = read(fd, pBuffer, ulLength);
        }
        else
        {
            bytesTransferred = write(fd, pBuffer, ulLength);
        }

        if (bytesTransferred >= 0)
        {
            break;
        }
        if (errno == EINTR)
        {
            continue;
        }
        else if (errno == EWOULDBLOCK)
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ntStatus = LwErrnoToNtStatus(errno);
            NFS3_ASSERT(ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

cleanup:

    *pulBytesTransferred = bytesTransferred;

    return ntStatus;

error:

    bytesTransferred = 0;

    goto cleanup;
}
