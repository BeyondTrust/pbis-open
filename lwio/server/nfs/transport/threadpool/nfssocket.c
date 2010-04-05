#include "includes.h"

static
inline
VOID
NFS_SOCKET_LOCK(
    IN PNFS_SOCKET pSocket
    )
{
    LwRtlLockMutex(&pSocket->Mutex);
}

static
inline
VOID
NFS_SOCKET_UNLOCK(
    IN PNFS_SOCKET pSocket
    )
{
    LwRtlUnlockMutex(&pSocket->Mutex);
}

static
inline
VOID
NFS_SOCKET_LOCK_WITH(
    IN OUT PBOOLEAN pbIsLocked,
    IN PNFS_SOCKET pSocket
    )
{
    LWIO_ASSERT(!*pbIsLocked);
    NFS_SOCKET_LOCK(pSocket);
    *pbIsLocked = TRUE;
}

static
inline
VOID
NFS_SOCKET_UNLOCK_WITH(
    IN OUT PBOOLEAN pbIsLocked,
    IN PNFS_SOCKET pSocket
    )
{
    if (*pbIsLocked)
    {
        NFS_SOCKET_UNLOCK(pSocket);
        *pbIsLocked = FALSE;
    }
}

static
VOID
NfsSocketAcquire(
    IN PNFS_SOCKET pSocket
    );

static
inline
PNFS_TRANSPORT_PROTOCOL_DISPATCH
NfsSocketGetDispatch(
    IN PNFS_SOCKET pSocket
    )
{
    return &pSocket->pListener->pTransport->Dispatch;
}

static
inline
VOID
NfsSocketSetDoneStatusIf(
    IN PNFS_SOCKET pSocket,
    IN NTSTATUS Status
    )
{
    LWIO_ASSERT(Status);

    if (!pSocket->DoneStatus)
    {
        pSocket->DoneStatus = Status;
    }
}

static
VOID
NfsSocketFree(
    IN OUT PNFS_SOCKET pSocket
    );

static
NTSTATUS
NfsSocketProcessTaskWrite(
    IN OUT PNFS_SOCKET pSocket
    );

static
VOID
NfsSocketProcessTaskDisconnect(
    IN PNFS_SOCKET pSocket,
    IN NTSTATUS Status
    );

static
VOID
NfsSocketProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    );

// Implementations

PCSTR
NfsSocketAddressToString(
    IN struct sockaddr* pSocketAddress,
    OUT PSTR pszAddress,
    IN ULONG AddressLength
    )
{
    PCSTR pszResult = NULL;
    PVOID pAddressPart = NULL;

    switch (pSocketAddress->sa_family)
    {
        case AF_INET:
            pAddressPart = &((struct sockaddr_in*)pSocketAddress)->sin_addr;
            break;
#ifdef LW_USE_INET6
        case AF_INET6:
            pAddressPart = &((struct sockaddr_in6*)pSocketAddress)->sin6_addr;
            break;
#endif
        default:
            goto error;
    }

    pszResult = inet_ntop(pSocketAddress->sa_family,
                          pAddressPart,
                          pszAddress,
                          AddressLength);

cleanup:

    return pszResult;

error:

    pszResult = NULL;

    // Terminate output buffer
    if (AddressLength > 0)
    {
        pszAddress[0] = 0;
    }

    goto cleanup;
}

NTSTATUS
NfsSocketCreate(
    IN PNFS_TRANSPORT_LISTENER pListener,
    IN int fd,
    IN struct sockaddr* pClientAddress,
    IN SOCKLEN_T ClientAddressLength,
    OUT PNFS_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_SOCKET pSocket = NULL;
    PCSTR pszAddress = NULL;

    if (ClientAddressLength > sizeof(pSocket->ClientAddress))
    {
        LWIO_LOG_ERROR("Client address is too long at %d bytes",
                       ClientAddressLength);
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NfsAllocateMemory(sizeof(*pSocket), OUT_PPVOID(&pSocket));
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->RefCount = 1;
    LwListInit(&pSocket->SendHead);

    ntStatus = LwRtlInitializeMutex(&pSocket->Mutex, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->pListener = pListener;
    pSocket->fd = fd;

    memcpy(&pSocket->ClientAddress, pClientAddress, ClientAddressLength);
    pSocket->ClientAddressLength = ClientAddressLength;

    pszAddress = NfsSocketAddressToString(
                            &pSocket->ClientAddress.Generic,
                            pSocket->AddressStringBuffer,
                            sizeof(pSocket->AddressStringBuffer));
    if (!pszAddress)
    {
        // This should never happen.
        LWIO_LOG_ERROR("Cannot fetch address string for fd = %d", fd);
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // TODO-Use separeate listener and socket task groups
    // in case of shutdown as we are adding a new socket task.
    ntStatus = LwRtlCreateTask(
                    pListener->pPool,
                    &pSocket->pTask,
                    pListener->pTaskGroup,
                    NfsSocketProcessTask,
                    pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    // Referenced by task
    NfsSocketAcquire(pSocket);

    LwRtlWakeTask(pSocket->pTask);

cleanup:

    *ppSocket = pSocket;

    return ntStatus;

error:

    if (pSocket)
    {
        if (pSocket->pTask)
        {
            pSocket->DoneStatus = ntStatus;
            LwRtlWakeTask(pSocket->pTask);
        }

        NfsSocketRelease(pSocket);
        pSocket = NULL;
    }

    goto cleanup;
}

static
VOID
NfsSocketAcquire(
    IN PNFS_SOCKET pSocket
    )
{
    LONG count = InterlockedIncrement(&pSocket->RefCount);
    LWIO_ASSERT(count > 1);
}

VOID
NfsSocketRelease(
    IN OUT PNFS_SOCKET pSocket
    )
{
    if (pSocket)
    {
        LONG count = InterlockedDecrement(&pSocket->RefCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            NfsSocketFree(pSocket);
        }
    }
}

static
VOID
NfsSocketFree(
    IN OUT PNFS_SOCKET pSocket
    )
{
    LWIO_ASSERT(!pSocket->pConnection);
    LWIO_ASSERT(LwListIsEmpty(&pSocket->SendHead));
    if (pSocket->pTask)
    {
        LwRtlReleaseTask(&pSocket->pTask);
    }
    if (pSocket->fd >= 0)
    {
        close(pSocket->fd);
    }
    LwRtlCleanupMutex(&pSocket->Mutex);
    NfsFreeMemory(pSocket);
}

VOID
NfsSocketGetAddress(
    IN PNFS_SOCKET pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT size_t* pAddressLength
    )
{
    // immutable, so lock not needed.
    *ppAddress = &pSocket->ClientAddress.Generic;
    *pAddressLength = pSocket->ClientAddressLength;
}

PCSTR
NfsSocketGetAddressString(
    IN PNFS_SOCKET pSocket
    )
{
    // immutable, so lock not needed.
    return pSocket->AddressStringBuffer;
}

int
NfsSocketGetFileDescriptor(
    IN PNFS_SOCKET pSocket
    )
{
    // immutable, so lock not needed.
    return pSocket->fd;
}
    

NTSTATUS
NfsSocketSetBuffer(
    IN PNFS_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (Minimum > Size)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    else if ((Size > 0) && !pBuffer)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if (ntStatus)
    {
        // This can never happen!
        LWIO_ASSERT(FALSE);

        NFS_SOCKET_LOCK(pSocket);
        pSocket->pBuffer = NULL;
        pSocket->Size = 0;
        pSocket->Minimum = 0;
        pSocket->Offset = 0;
        NFS_SOCKET_UNLOCK(pSocket);
    }
    else
    {
        NFS_SOCKET_LOCK(pSocket);
        pSocket->pBuffer = pBuffer;
        pSocket->Size = Size;
        pSocket->Minimum = Minimum;
        pSocket->Offset = 0;
        if (Size > 0)
        {
            // Notify task that there is a buffer
            LwRtlWakeTask(pSocket->pTask);
        }
        NFS_SOCKET_UNLOCK(pSocket);
    }

    return ntStatus;
}

static
NTSTATUS
NfsSocketSendReplyCommon(
    IN PNFS_SOCKET pSocket,
    IN PNFS_SEND_CONTEXT pSendContext,
    IN OPTIONAL PLW_ZCT_VECTOR pZct,
    IN OPTIONAL PVOID pBuffer,
    IN ULONG Size
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_SEND_ITEM pSendItem = NULL;
    BOOLEAN bIsLocked = FALSE;

    LWIO_ASSERT(!IS_BOTH_OR_NEITHER(pZct, pBuffer));
    LWIO_ASSERT(!pZct || (0 == Size));
    LWIO_ASSERT(!pBuffer || (Size > 0));

    ntStatus = NfsAllocateMemory(sizeof(*pSendItem), OUT_PPVOID(&pSendItem));
    BAIL_ON_NT_STATUS(ntStatus);

    pSendItem->pSendContext = pSendContext;
    if (pZct)
    {
        pSendItem->pZct = pZct;
    }
    else
    {
        pSendItem->pBuffer = pBuffer;
        pSendItem->Length = Size;
    }

    NFS_SOCKET_LOCK_WITH(&bIsLocked, pSocket);

    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsSocketGetDispatch(pSocket)->pfnSendPrepare(pSendContext);
    BAIL_ON_NT_STATUS(ntStatus);

    // Cannot fail after this.

    LwListInsertTail(&pSocket->SendHead, &pSendItem->SendLinks);

    if (IsSetFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_WRITABLE))
    {
        // Try to write inline.
        ntStatus = NfsSocketProcessTaskWrite(pSocket);
    }

    // Wake task to process list or send error
    if (!LwListIsEmpty(&pSocket->SendHead) || ntStatus)
    {
        LwRtlWakeTask(pSocket->pTask);
    }

    ntStatus = STATUS_SUCCESS;

cleanup:

    NFS_SOCKET_UNLOCK_WITH(&bIsLocked, pSocket);

    return ntStatus;

error:

    if (pSendItem)
    {
        NfsFreeMemory(pSendItem);
    }

    goto cleanup;
}

NTSTATUS
NfsSocketSendReply(
    IN PNFS_SOCKET pSocket,
    IN PNFS_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    )
{
    return NfsSocketSendReplyCommon(
                pSocket,
                pSendContext,
                NULL,
                pBuffer,
                Size);
}

NTSTATUS
NfsSocketSendZctReply(
    IN PNFS_SOCKET pSocket,
    IN PNFS_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    )
{
    return NfsSocketSendReplyCommon(
                pSocket,
                pSendContext,
                pZct,
                NULL,
                0);
}

VOID
NfsSocketDisconnect(
    IN PNFS_SOCKET pSocket
    )
{
    NFS_SOCKET_LOCK(pSocket);
    NfsSocketSetDoneStatusIf(pSocket, STATUS_CONNECTION_DISCONNECTED);
    LwRtlWakeTask(pSocket->pTask);
    NFS_SOCKET_UNLOCK(pSocket);
}

VOID
NfsSocketClose(
    IN OUT PNFS_SOCKET pSocket
    )
{
    NFS_SOCKET_LOCK(pSocket);

    // Check that caller is not doing something wrong.
    LWIO_ASSERT(!IsSetFlag(pSocket->StateMask, NFS_SOCKET_STATE_CLOSED));
    SetFlag(pSocket->StateMask, NFS_SOCKET_STATE_CLOSED);

    // No connection any more.  This disables notifications to the PTD.
    pSocket->pConnection = NULL;

    NfsSocketSetDoneStatusIf(pSocket, STATUS_CONNECTION_DISCONNECTED);
    LwRtlWakeTask(pSocket->pTask);

    NFS_SOCKET_UNLOCK(pSocket);

    // Release PTD's reference.
    NfsSocketRelease(pSocket);
}

static
NTSTATUS
NfsSocketProcessTaskInit(
    IN PNFS_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // Notify PTD about new connection.
    ntStatus = NfsSocketGetDispatch(pSocket)->pfnConnectionNew(
                    &pSocket->pConnection,
                    pSocket->pListener->pTransport->pContext,
                    pSocket);
    if (ntStatus)
    {
        LWIO_LOG_ERROR("New connection failed in protocol for fd = %d, address = '%s' with status = 0x%08x",
                       pSocket->fd,
                       pSocket->AddressStringBuffer,
                       ntStatus);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_ASSERT(pSocket->pConnection);

    // Take a reference for the PTD's new connection object.
    NfsSocketAcquire(pSocket);

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
NfsSocketRead(
    IN int FileDescriptor,
    OUT PVOID pBuffer,
    IN ULONG Length,
    OUT PULONG pBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t bytesTransferred = 0;

    for (;;)
    {
        bytesTransferred = read(FileDescriptor, pBuffer, Length);
        if (bytesTransferred >= 0)
        {
            break;
        }
        if (EINTR == errno)
        {
            continue;
        }
        else if (EAGAIN == errno)
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ntStatus = LwErrnoToNtStatus(errno);
            LWIO_ASSERT(ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

cleanup:

    *pBytesTransferred = bytesTransferred;

    return ntStatus;

error:

    bytesTransferred = 0;

    goto cleanup;
}

static
NTSTATUS
NfsSocketWrite(
    IN int FileDescriptor,
    OUT PVOID pBuffer,
    IN ULONG Length,
    OUT PULONG pBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t bytesTransferred = 0;

    for (;;)
    {
        bytesTransferred = write(FileDescriptor, pBuffer, Length);
        if (bytesTransferred >= 0)
        {
            break;
        }
        if (EINTR == errno)
        {
            continue;
        }
        else if (EAGAIN == errno)
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ntStatus = LwErrnoToNtStatus(errno);
            LWIO_ASSERT(ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

cleanup:

    *pBytesTransferred = bytesTransferred;

    return ntStatus;

error:

    bytesTransferred = 0;

    goto cleanup;
}

static
NTSTATUS
NfsSocketProcessTaskRead(
    IN OUT PNFS_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG bytesRemaining = 0;
    ULONG initialOffset = pSocket->Offset;

    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!IsSetFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_READABLE))
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    LWIO_ASSERT(pSocket->Offset <= pSocket->Size);
    bytesRemaining = pSocket->Size - pSocket->Offset;
    while (bytesRemaining > 0)
    {
        PVOID pCurrent = LwRtlOffsetToPointer(pSocket->pBuffer, pSocket->Offset);
        ULONG bytesTransferred = 0;

        ntStatus = NfsSocketRead(pSocket->fd, pCurrent, bytesRemaining, &bytesTransferred);
        if (STATUS_MORE_PROCESSING_REQUIRED == ntStatus)
        {
            ClearFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_READABLE);
            ntStatus = STATUS_SUCCESS;
            break;
        }
        else if (STATUS_SUCCESS != ntStatus)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (0 == bytesTransferred)
        {
            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pSocket->Offset += bytesTransferred;

        LWIO_ASSERT(pSocket->Offset <= pSocket->Size);
        bytesRemaining = pSocket->Size - pSocket->Offset;
    }

    // Call protocol transport driver, if needed.
    if ((pSocket->Offset > initialOffset) &&
        (pSocket->Offset >= pSocket->Minimum))
    {
        ntStatus = NfsSocketGetDispatch(pSocket)->pfnConnectionData(
                        pSocket->pConnection,
                        pSocket->Offset);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    pSocket->DoneStatus = ntStatus;

    goto cleanup;
}

static
NTSTATUS
NfsSocketProcessTaskWrite(
    IN OUT PNFS_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!IsSetFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_WRITABLE))
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    while (!LwListIsEmpty(&pSocket->SendHead))
    {
        PLW_LIST_LINKS pLinks = pSocket->SendHead.Next;
        PNFS_SEND_ITEM pSendItem = LW_STRUCT_FROM_FIELD(pLinks, NFS_SEND_ITEM, SendLinks);
        ULONG bytesRemaining = 0;
        ULONG bytesTransferred = 0;

        if (pSendItem->pZct)
        {
            ntStatus = LwZctWriteSocketIo(
                            pSendItem->pZct,
                            pSocket->fd,
                            &bytesTransferred,
                            &bytesRemaining);
        }
        else
        {
            PVOID pCurrent = LwRtlOffsetToPointer(pSendItem->pBuffer, pSendItem->Offset);

            LWIO_ASSERT(pSendItem->Length > pSendItem->Offset);

            bytesRemaining = pSendItem->Length - pSendItem->Offset;

            ntStatus = NfsSocketWrite(pSocket->fd, pCurrent, bytesRemaining, &bytesTransferred);
            if (STATUS_SUCCESS == ntStatus)
            {
                pSendItem->Offset += bytesTransferred;
                
                LWIO_ASSERT(pSendItem->Offset <= pSendItem->Length);
                bytesRemaining = pSendItem->Length - pSendItem->Offset;
            }
        }

        if (STATUS_MORE_PROCESSING_REQUIRED == ntStatus)
        {
            ClearFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_WRITABLE);
            ntStatus = STATUS_SUCCESS;
            break;
        }

        LWIO_ASSERT(NT_SUCCESS_OR_NOT(ntStatus));
        BAIL_ON_NT_STATUS(ntStatus);

        if (!bytesRemaining)
        {
            NfsSocketGetDispatch(pSocket)->pfnSendDone(
                    pSendItem->pSendContext,
                    STATUS_SUCCESS);

            LwListRemove(&pSendItem->SendLinks);
            NfsFreeMemory(pSendItem);
        }
    }

cleanup:

    return ntStatus;

error:

    pSocket->DoneStatus = ntStatus;

    goto cleanup;
}

static
VOID
NfsSocketProcessTaskDisconnect(
    IN PNFS_SOCKET pSocket,
    IN NTSTATUS Status
    )
{
    NfsSocketSetDoneStatusIf(pSocket, Status);

    LWIO_ASSERT(pSocket->DoneStatus);

    // For debugging only - the state machine should call in here just once
    // because this function will cancel the task.

    LWIO_ASSERT(!IsSetFlag(pSocket->StateMask, NFS_SOCKET_STATE_DISCONNECTED));
    SetFlag(pSocket->StateMask, NFS_SOCKET_STATE_DISCONNECTED);

    if (pSocket->pConnection)
    {
        while (!LwListIsEmpty(&pSocket->SendHead))
        {
            PLW_LIST_LINKS pLinks = pSocket->SendHead.Next;
            PNFS_SEND_ITEM pSendItem = LW_STRUCT_FROM_FIELD(pLinks, NFS_SEND_ITEM, SendLinks);
        
            NfsSocketGetDispatch(pSocket)->pfnSendDone(
                            pSendItem->pSendContext,
                            pSocket->DoneStatus);
        
            LwListRemove(&pSendItem->SendLinks);
            NfsFreeMemory(pSendItem);
        }

        NfsSocketGetDispatch(pSocket)->pfnConnectionDone(
                pSocket->pConnection,
                pSocket->DoneStatus);

        pSocket->pConnection = NULL;
    }

    LWIO_ASSERT(LwListIsEmpty(&pSocket->SendHead));

    if (pSocket->fd >= 0)
    {
        NTSTATUS ntStatus = LwRtlSetTaskFd(pSocket->pTask, pSocket->fd, 0);
        if (ntStatus)
        {
            LWIO_LOG_ERROR("Unexpected set task FD error for client '%s', "
                           "fd = %d0x%08x, status = 0x%08x",
                           pSocket->AddressStringBuffer,
                           pSocket->fd,
                           ntStatus);
        }
        close(pSocket->fd);
        pSocket->fd = -1;
    }

    LwRtlCancelTask(pSocket->pTask);
}

static
VOID
NfsSocketProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_SOCKET pSocket = (PNFS_SOCKET) pDataContext;
    BOOLEAN bIsLocked = FALSE;
    LW_TASK_EVENT_MASK waitMask = 0;

    // Handle EVENT_CANCEL - this only happens when the socket is closed
    // by the protocol transport driver.

    // TODO-what about listener shutdown?!  -- NEED TO HANDLE THAT CASE,
    // probably by setting a boolean on the listener, waking the group,
    // and checking for that to do a disconnect, which will cause the
    // protocol to close the socket.

    NFS_SOCKET_LOCK_WITH(&bIsLocked, pSocket);

    if (IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL))
    {
        if (pSocket->pConnection)
        {
            ntStatus = STATUS_CANCELLED;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        NFS_SOCKET_UNLOCK_WITH(&bIsLocked, pSocket);

        NfsSocketRelease(pSocket);
        pSocket = NULL;

        waitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    // Process any error conditions.
    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

    // Handle EVENT_INIT:
    if (IsSetFlag(WakeMask, LW_TASK_EVENT_INIT))
    {
        ntStatus = NfsSocketProcessTaskInit(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // If we have made it this far, there must be an active connection.
    LWIO_ASSERT(pSocket->pConnection);

    //
    // Handle EVENT_FD_{WRITABLE,READABLE}
    //

    // Save FD state (in case we cannot fully act on it now).
    if (IsSetFlag(WakeMask, LW_TASK_EVENT_FD_WRITABLE))
    {
        SetFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_WRITABLE);
    }
    if (IsSetFlag(WakeMask, LW_TASK_EVENT_FD_READABLE))
    {
        SetFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_READABLE);
    }

    // Process FD state, handling write first so we can free up
    // memory.

    ntStatus = NfsSocketProcessTaskWrite(pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsSocketProcessTaskRead(pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    // Determine what wait mask is needed.
    if ((pSocket->Size - pSocket->Offset) > 0)
    {
        SetFlag(waitMask, LW_TASK_EVENT_FD_READABLE);
        if (IsSetFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_READABLE))
        {
            SetFlag(waitMask, LW_TASK_EVENT_YIELD);
        }
    }
    if (!LwListIsEmpty(&pSocket->SendHead))
    {
        SetFlag(waitMask, LW_TASK_EVENT_FD_WRITABLE);
        if (IsSetFlag(pSocket->StateMask, NFS_SOCKET_STATE_FD_WRITABLE))
        {
            SetFlag(waitMask, LW_TASK_EVENT_YIELD);
        }
    }
    if (!waitMask)
    {
        waitMask = LW_TASK_EVENT_EXPLICIT;
    }

cleanup:

    NFS_SOCKET_UNLOCK_WITH(&bIsLocked, pSocket);

    // waitMask can only be 0 (aka COMPLETE) for EVENT_CANCEL.
    LWIO_ASSERT(waitMask ||
                ((LW_TASK_EVENT_COMPLETE == waitMask) &&
                 IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL)));

    *pWaitMask = waitMask;

    return;

error:

    NfsSocketProcessTaskDisconnect(pSocket, ntStatus);

    // Connection needs to close the socket (i.e., cancel the task).
    waitMask = LW_TASK_EVENT_EXPLICIT;

    goto cleanup;
}

