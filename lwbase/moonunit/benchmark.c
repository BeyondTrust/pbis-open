#include <lw/base.h>
#include <lw/rtlgoto.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <assert.h>

#include "benchmark.h"

#define ASSERT_SUCCESS(status) assert((status) == STATUS_SUCCESS)

static PLW_THREAD_POOL gpPool = NULL;
static PBENCHMARK_SETTINGS gpSettings = NULL;

static inline
NTSTATUS
TimeNow(
    PLONG64 pllNow
    )
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL))
    {
        return LwErrnoToNtStatus(errno);
    }
    else
    {
        *pllNow = 
            tv.tv_sec * 1000000000ll +
            tv.tv_usec * 1000ll;

        return STATUS_SUCCESS;
    }
}

typedef struct _SOCKET
{
    int Fd;
    PBYTE pBuffer;
    size_t Position;
    int Iteration;
    enum
    {
        STATE_SEND,
        STATE_RECV
    } State;
    PLW_TASK pTask;
    ULONG64 ullTotalTransferred;
} SOCKET, *PSOCKET;

static
VOID
Transceiver(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    PSOCKET pSocket = (PSOCKET) pContext;
    size_t sendSize = gpSettings->ulBufferSize / gpSettings->usSendSegments;
    ssize_t transferred = 0;
    long opts = 0;
    int err = 0;

    if (WakeMask & LW_TASK_EVENT_CANCEL ||
        pSocket->Iteration >= gpSettings->ulIterations)
    {
        *pWaitMask = 0;
        ASSERT_SUCCESS(
            LwRtlSetTaskFd(pTask, pSocket->Fd, 0));
        close(pSocket->Fd);
        pSocket->Fd = -1;
        return;
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        assert(pSocket->Iteration == 0);

        /* Put socket in nonblock mode */
        opts = fcntl(pSocket->Fd, F_GETFL, 0);
        assert(opts >= 0);
        opts |= O_NONBLOCK;
        err = fcntl(pSocket->Fd, F_SETFL, opts);
        assert(err == 0);
        
        ASSERT_SUCCESS(
            LwRtlSetTaskFd(
                pTask,
                pSocket->Fd,
                LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE));
    }

    switch(pSocket->State)
    {
    case STATE_SEND:
        if (sendSize > gpSettings->ulBufferSize - pSocket->Position)
            sendSize = gpSettings->ulBufferSize - pSocket->Position;

        transferred = write(
            pSocket->Fd,
            pSocket->pBuffer + pSocket->Position,
            sendSize);

        assert(transferred >= 0 || errno == EAGAIN);

        if (transferred < 0 && errno == EAGAIN)
        {
            *pWaitMask = LW_TASK_EVENT_FD_WRITABLE;
            break;
        }

        pSocket->Position += transferred;
        pSocket->ullTotalTransferred += transferred;

        if (pSocket->Position >= gpSettings->ulBufferSize)
        {
            pSocket->State = STATE_RECV;
            pSocket->Position = 0;
            pSocket->Iteration++;
        }

        *pWaitMask = LW_TASK_EVENT_YIELD;
        break;
    case STATE_RECV:
        transferred = read(
            pSocket->Fd,
            pSocket->pBuffer + pSocket->Position,
            gpSettings->ulBufferSize - pSocket->Position);

        assert(transferred >= 0 || errno == EAGAIN);

        if (transferred < 0 && errno == EAGAIN)
        {
            *pWaitMask = LW_TASK_EVENT_FD_READABLE;
            break;
        }       

        pSocket->Position += transferred;
        pSocket->ullTotalTransferred += transferred;

        if (pSocket->Position >= gpSettings->ulBufferSize)
        {
            pSocket->State = STATE_SEND;
            pSocket->Position = 0;
            pSocket->Iteration++;
        }

        *pWaitMask = LW_TASK_EVENT_YIELD;
        break;
    }
}

static
NTSTATUS
CreateSocketPair(
    PLW_TASK_GROUP pGroup,
    PSOCKET* ppSocket1,
    PSOCKET* ppSocket2
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSOCKET pSocket1 = NULL;
    PSOCKET pSocket2 = NULL;
    int socketFds[2] = {-1, -1};

    status = LW_RTL_ALLOCATE_AUTO(&pSocket1);
    GOTO_ERROR_ON_STATUS(status);

    status = LW_RTL_ALLOCATE_AUTO(&pSocket2);
    GOTO_ERROR_ON_STATUS(status);

    status = LW_RTL_ALLOCATE_ARRAY_AUTO(&pSocket1->pBuffer, gpSettings->ulBufferSize);
    GOTO_ERROR_ON_STATUS(status);

    status = LW_RTL_ALLOCATE_ARRAY_AUTO(&pSocket2->pBuffer, gpSettings->ulBufferSize);
    GOTO_ERROR_ON_STATUS(status);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketFds) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    pSocket1->Fd = socketFds[0];
    pSocket2->Fd = socketFds[1];
    
    pSocket1->State = STATE_SEND;
    pSocket2->State = STATE_RECV;

    status = LwRtlCreateTask(
        gpPool,
        &pSocket1->pTask,
        pGroup,
        Transceiver,
        pSocket1);
    GOTO_ERROR_ON_STATUS(status);

    status = LwRtlCreateTask(
        gpPool,
        &pSocket2->pTask,
        pGroup,
        Transceiver,
        pSocket2);
    GOTO_ERROR_ON_STATUS(status);

    *ppSocket1 = pSocket1;
    *ppSocket2 = pSocket2;

cleanup:
    
    return status;

error:

    goto cleanup;
}

VOID
BenchmarkThreadPool(
    PLW_THREAD_POOL pPool,
    PBENCHMARK_SETTINGS pSettings,
    PULONG64 pullDuration,
    PULONG64 pullBytesTransferred
    )
{
    PLW_TASK_GROUP pGroup = NULL;
    PSOCKET* ppSockets1 = NULL;
    PSOCKET* ppSockets2 = NULL;
    size_t i = 0;
    ULONG64 ullTotal = 0;
    LONG64 llStart = 0;
    LONG64 llEnd = 0;
    ULONG64 ullTime = 0;

    gpPool = pPool;
    gpSettings = pSettings;

    ASSERT_SUCCESS(LwRtlCreateTaskGroup(
                                 gpPool,
                                 &pGroup));

    ASSERT_SUCCESS(LW_RTL_ALLOCATE_ARRAY_AUTO(&ppSockets1, gpSettings->ulPairs));
    ASSERT_SUCCESS(LW_RTL_ALLOCATE_ARRAY_AUTO(&ppSockets2, gpSettings->ulPairs));

    for (i = 0; i < gpSettings->ulPairs; i++)
    {
        ASSERT_SUCCESS(
            CreateSocketPair(
                pGroup,
                &ppSockets1[i],
                &ppSockets2[i]));
    }
    
    ASSERT_SUCCESS(TimeNow(&llStart));
    
    LwRtlWakeTaskGroup(pGroup);
    LwRtlWaitTaskGroup(pGroup);
    
    ASSERT_SUCCESS(TimeNow(&llEnd));
    
    ullTime = (ULONG64) (llEnd - llStart);
    
    LwRtlFreeTaskGroup(&pGroup);

    for (i = 0; i < gpSettings->ulPairs; i++)
    {
        ullTotal += ppSockets1[i]->ullTotalTransferred;
    }

    *pullDuration = ullTime;
    *pullBytesTransferred = ullTotal;
}
