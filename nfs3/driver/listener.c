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


struct __NFS3_LISTENER
{
    PLW_THREAD_POOL          pPool;              // Transport's pool
    PLW_TASK                 pListenerTask;
    int                      listenFd;
    PLW_TASK_GROUP           pSocketTaskGroup;
    NFS3_TRANSPORT_CALLBACKS callbacks;
};


static
NTSTATUS
Nfs3ListenerInitSocket(
    PNFS3_LISTENER pListener
    );

static
VOID
Nfs3ListenerProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    );

static
NTSTATUS
Nfs3ListenerProcessTaskInit(
    PNFS3_LISTENER pListener
    );


NTSTATUS
Nfs3ListenerCreate(
    PNFS3_LISTENER* ppListener,
    PLW_THREAD_POOL pPool,
    const PNFS3_TRANSPORT_CALLBACKS pCallbacks
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS3_LISTENER pListener;

    ntStatus = Nfs3AllocateMemoryClear(sizeof(*pListener), (PVOID*)&pListener);
    BAIL_ON_NT_STATUS(ntStatus)

    memset(pListener, 0, sizeof(*pListener));

    pListener->pPool = pPool;
    pListener->callbacks = *pCallbacks;

    ntStatus = Nfs3ListenerInitSocket(pListener);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTaskGroup(pPool, &pListener->pSocketTaskGroup);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTask(
                    pPool,
                    &pListener->pListenerTask,
                    NULL,   // No need to create taskgroop for listener
                    Nfs3ListenerProcessTask,
                    pListener);
    BAIL_ON_NT_STATUS(ntStatus);

    LwRtlWakeTask(pListener->pListenerTask);

    *ppListener = pListener;

cleanup:

    return ntStatus;

error:

    Nfs3ListenerFree(&pListener);
    *ppListener = NULL;

    goto cleanup;
}

VOID
Nfs3ListenerFree(
    PNFS3_LISTENER* ppListener
    )
{
    PNFS3_LISTENER pListener = *ppListener;

    // Force all sockets to finish their jobs
    if (pListener->pSocketTaskGroup)
    {
        LwRtlCancelTaskGroup(pListener->pSocketTaskGroup);
        LwRtlWaitTaskGroup(pListener->pSocketTaskGroup);
        LwRtlFreeTaskGroup(&pListener->pSocketTaskGroup);
    }

    // TODO - free all sockets memory

    // Cancel and free listener task
    LwRtlCancelTask(pListener->pListenerTask);
    LwRtlWaitTask(pListener->pListenerTask);
    LwRtlReleaseTask(&pListener->pListenerTask);

    Nfs3FreeMemory((PVOID*)&pListener);
    
    *ppListener = pListener;
}

static
NTSTATUS
Nfs3ListenerInitSocket(
    PNFS3_LISTENER pListener
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    struct sockaddr_in  addr = { 0 };
    SOCKLEN_T           addrLen = 0;
    int                 on = 1;
    long                opts = 0;
    int                 syserr = 0;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    addr.sin_port = htons(gNfs3Globals.config.ulTcpServerPort);
    addrLen = sizeof(addr);

    pListener->listenFd = socket(
                    ((const struct sockaddr*)&addr)->sa_family,
                    SOCK_STREAM,
                    0);
    BAIL_ON_SYS_ERROR(pListener->listenFd);

    syserr = setsockopt(pListener->listenFd, SOL_SOCKET, SO_REUSEADDR, &on,
                        sizeof(on));
    BAIL_ON_SYS_ERROR(syserr);

#ifdef TCP_NODELAY
    syserr = setsockopt(pListener->listenFd, IPPROTO_TCP, TCP_NODELAY, &on, 
                        sizeof(on));
    BAIL_ON_SYS_ERROR(syserr);
#endif

    /* Put socket in nonblock mode */
    opts = fcntl(pListener->listenFd, F_GETFL, 0);
    BAIL_ON_SYS_ERROR(opts);

    opts |= O_NONBLOCK;

    syserr = fcntl(pListener->listenFd, F_SETFL, opts);
    BAIL_ON_SYS_ERROR(syserr);

    syserr = bind(pListener->listenFd, (const struct sockaddr*)&addr, addrLen);
    BAIL_ON_SYS_ERROR(syserr);

    syserr = listen(pListener->listenFd, 
                    gNfs3Globals.config.ulTcpListenQueueLength);
    BAIL_ON_SYS_ERROR(syserr);

cleanup:

    return ntStatus;

error:

    if (pListener->listenFd >= 0)
    {
        close(pListener->listenFd);
        pListener->listenFd = -1;
    }

    goto cleanup;
}

static
VOID
Nfs3ListenerProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK wakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    PNFS3_LISTENER  pListener = (PNFS3_LISTENER)pDataContext;
    int             connFd = -1;
    PNFS3_SOCKET    pSocket = NULL;
    NFS3_SOCKADDR   cliAddr = { { 0 }, sizeof(struct sockaddr_in) };
    NFS3_SOCKADDR   srvAddr = { { 0 }, sizeof(struct sockaddr_in) };
    CHAR            cliAddrBuf[NFS3_MAX_INET_ADDRSTRLEN] = { 0 };
    CHAR            srvAddrBuf[NFS3_MAX_INET_ADDRSTRLEN] = { 0 };
    LW_TASK_EVENT_MASK  waitMask = 0;

    if (wakeMask & LW_TASK_EVENT_INIT)
    {
        LWIO_LOG_DEBUG("Nfs3 listener starting");
        
        ntStatus = Nfs3ListenerProcessTaskInit(pListener);
        // Note that task will terminate on error.
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (wakeMask & LW_TASK_EVENT_CANCEL)
    {
        LWIO_LOG_DEBUG("Nfs3 listener stopping");
        
        if (pListener->listenFd >= 0)
        {
            close(pListener->listenFd);
            pListener->listenFd = -1;
        }

        waitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    connFd = accept(pListener->listenFd, (struct sockaddr*)&cliAddr.addr,
                    &cliAddr.len);
    if (connFd < 0)
    {
        if (errno == EPROTO || errno == ECONNABORTED || errno == EINTR)
        {
            waitMask = LW_TASK_EVENT_YIELD;
            goto cleanup;
        }
        else if (errno == EMFILE)
        {
            LWIO_LOG_ERROR("Failed to accept connection due to too many open files");
            waitMask = LW_TASK_EVENT_YIELD;
            goto cleanup;
        }
        else if (errno == EAGAIN)
        {
            waitMask = LW_TASK_EVENT_FD_READABLE;
            goto cleanup;
        }
        else
        {
            // Note that the task will terminate.
            ntStatus = LwErrnoToNtStatus(errno);
            LWIO_LOG_ERROR("Failed to accept connection (errno = %d, status = "
                           "0x%08x)", errno, ntStatus);
            NFS3_ASSERT(ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (getsockname(connFd, (struct sockaddr*)&srvAddr.addr, &srvAddr.len) < 0)
    {
        // Note that the task will terminate.
        ntStatus = LwErrnoToNtStatus(errno);
        LWIO_LOG_ERROR("Failed to find the local socket address for fd = %d "
                       "(errno = %d, status = 0x%08x)", connFd, errno, ntStatus);
        NFS3_ASSERT(ntStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = Nfs3SocketAddressToString((struct sockaddr*)&cliAddr.addr,
                                         cliAddrBuf, sizeof(cliAddrBuf));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = Nfs3SocketAddressToString((struct sockaddr*)&srvAddr.addr, 
                                         srvAddrBuf, sizeof(srvAddrBuf));
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOG_INFO("Handling client from '%s' [server address: %s] on fd = %d",
                  cliAddrBuf, srvAddrBuf, connFd);

    ntStatus = Nfs3SocketCreate(&pSocket, pListener->pPool, &pListener->callbacks,
                                pListener->pSocketTaskGroup, connFd, 
                                &cliAddr, &srvAddr);
    if (ntStatus)
    {
        // Do not terminate on this error.
        LWIO_LOG_ERROR("Failed to create transport socket for fd = %d, address"
                       " = '%s' (status = 0x%08x)", connFd, cliAddrBuf,
                       ntStatus);
        ntStatus = STATUS_SUCCESS;
        waitMask = LW_TASK_EVENT_YIELD;
        goto cleanup;
    }

    connFd = -1;

    waitMask = LW_TASK_EVENT_YIELD;

cleanup:

    if (connFd >= 0)
    {
        close(connFd);
    }

    // waitMask can only be 0 (aka COMPLETE) for EVENT_CANCEL or error.
    NFS3_ASSERT(waitMask ||
                ((LW_TASK_EVENT_COMPLETE == waitMask) &&
                 (IsSetFlag(wakeMask, LW_TASK_EVENT_CANCEL) || ntStatus)));

    *pWaitMask = waitMask;

    return;

error:

    waitMask = LW_TASK_EVENT_COMPLETE;

    goto cleanup;
}

static
NTSTATUS
Nfs3ListenerProcessTaskInit(
    PNFS3_LISTENER pListener
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    /* Register fd with thread pool */
    ntStatus = LwRtlSetTaskFd(
        pListener->pListenerTask,
        pListener->listenFd,
        LW_TASK_EVENT_FD_READABLE);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    if (pListener->listenFd >= 0)
    {
        close(pListener->listenFd);
        pListener->listenFd = -1;
    }

    goto cleanup;
}
