/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Test Program for exercising SMB Client API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"

pthread_mutex_t gNpClientTestMutex = PTHREAD_MUTEX_INITIALIZER;
ULONG   gNumIterations = 5;
BOOLEAN gbExitProcess = FALSE;

static
NTSTATUS
CreatePipeClient(
    const char * pipename,
    IO_FILE_HANDLE * pFileHandle
    );

static
PVOID
ClientPipeThread(
    PIO_FILE_HANDLE pFileHandle
    );

static
NTSTATUS
HandleSignals(
    VOID
    );

static
VOID
InterruptHandler(
    int sig
    );

static
NTSTATUS
BlockSignals(
    VOID
    );

static
int
GetNextSignal(
    VOID
    );

static
VOID
GetBlockedSignals(
    sigset_t* pBlockedSignals
    );

static
VOID
GetBlockedSignalsSansInterrupt(
    sigset_t* pBlockedSignals
    );

static
BOOLEAN
ProcessMustExit(
    VOID
    );

static
VOID
SetProcessMustExit(
    BOOLEAN bExit
    );

int
main(
    int    argc,
    char** argv
    )
{
    NTSTATUS ntStatus = 0;
    ULONG i = 0;
    int nConnections = 0;
    int nActiveConnections = 0;
    char *pipename = NULL;
    IO_FILE_HANDLE  FileHandles[100];
    pthread_t* pThreadArray = NULL;

    memset(FileHandles, 0, sizeof(FileHandles));

    if (argc < 4)
    {
        printf("Usage: test_npclient2 <pipename> <number of connections> <number of iterations>\n");
        exit(1);
    }

    pipename = argv[1];
    nConnections = atoi(argv[2]);
    gNumIterations = atoi(argv[3]);

    if (nConnections < 0)
    {
        printf("Usage: test_npclient2 <pipename> <number of connections> <number of iterations>\n");
        exit(1);
    }

    ntStatus = BlockSignals();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwIoAllocateMemory(
                    sizeof(pthread_t) * nConnections,
                    (PVOID*)&pThreadArray);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < nConnections; i++)
    {
        pthread_t* pThread = &pThreadArray[i];

        ntStatus = 0;

        do
        {
            ntStatus = CreatePipeClient(
                            pipename,
                            &FileHandles[i]
                            );
            if (ntStatus == STATUS_PIPE_NOT_AVAILABLE)
            { 
                printf("pipe not available. waiting...\n");
                sleep(5);
            }
            else
            {
                BAIL_ON_NT_STATUS(ntStatus);
            }

        } while (!ProcessMustExit() && (ntStatus == STATUS_PIPE_NOT_AVAILABLE));

        ntStatus = LwErrnoToNtStatus(pthread_create(pThread, NULL, (void *)&ClientPipeThread, &FileHandles[i]));

        BAIL_ON_NT_STATUS(ntStatus);

        nActiveConnections++;
    }

    ntStatus = HandleSignals();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (nActiveConnections)
    {
        int iConnection = 0;

        for(; iConnection < nActiveConnections; iConnection++)
        {
            pthread_t* pThread = &pThreadArray[iConnection];

            // TODO: Need these threads to be interruptible
            pthread_join(*pThread, NULL);
        }

        LwIoFreeMemory(pThreadArray);
    }

    return ntStatus;

error:

    goto cleanup;
}


static
NTSTATUS
CreatePipeClient(
    const char * pipename,
    IO_FILE_HANDLE * pFileHandle
    )
{
    NTSTATUS ntStatus = 0;
    PSTR smbpath = NULL;
    //PIO_ACCESS_TOKEN acctoken = NULL;
    IO_FILE_NAME filename = { 0 };
    IO_STATUS_BLOCK io_status = { 0 };
    IO_FILE_HANDLE FileHandle = NULL;

    ntStatus = LwRtlCStringAllocatePrintf(
                    &smbpath,
                    "\\npfs\\%s",
                    (char*) pipename);
    BAIL_ON_NT_STATUS(ntStatus);

    filename.RootFileHandle = NULL;
    filename.IoNameOptions = 0;

    ntStatus = LwRtlWC16StringAllocateFromCString(
                        &filename.FileName,
                        smbpath
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
                        &FileHandle,
                        NULL,
                        &io_status,
                        &filename,
                        NULL,
                        NULL,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        0,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        FILE_OPEN,
                        0,
                        NULL,
                        0,
                        NULL,
                        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    *pFileHandle = FileHandle;

    return(ntStatus);

    

error:

    *pFileHandle = NULL;

    return ntStatus;
}


PVOID
ClientPipeThread(
    PIO_FILE_HANDLE pFileHandle
    )
{
    NTSTATUS ntStatus = 0;
    BYTE InBuffer[2048];
    BYTE OutBuffer[2048];
    ULONG InLength = 0;
    ULONG InBytesRead = 0;
    ULONG OutBytesWritten = 0;
    IO_STATUS_BLOCK io_status = {0};
    IO_FILE_HANDLE FileHandle = NULL;
    ULONG i = 0;

    printf("Client thread [%zd] starting\n", (size_t)pthread_self());

    FileHandle =  *pFileHandle;
    while (!ProcessMustExit() && (i++ < gNumIterations)) {

        memset(OutBuffer, 0, sizeof(OutBuffer));
        sprintf((char *)OutBuffer,"The quick brown fox jumps over the lazy dog\n");
        OutBytesWritten = strlen((char*)OutBuffer)+1;

        ntStatus = NtWriteFile(
                        FileHandle,
                        NULL,
                        &io_status,
                        OutBuffer,
                        OutBytesWritten,
                        NULL,
                        NULL
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        OutBytesWritten = io_status.BytesTransferred;
    
        memset(&io_status, 0, sizeof(io_status));
        memset(InBuffer, 0, sizeof(InBuffer));
        InLength = sizeof(InBuffer);

        ntStatus = NtReadFile(
                        FileHandle,
                        NULL,
                        &io_status,
                        InBuffer,
                        InLength,
                        NULL,
                        NULL
                        );
        BAIL_ON_NT_STATUS(ntStatus);
        InBytesRead = io_status.BytesTransferred;

        if (InBytesRead ==  OutBytesWritten) {
            printf("Successful echo [Thr:%zd]:%s\n", (size_t)pthread_self(), InBuffer);
        }
    }

cleanup:

    if (FileHandle)
    {
        NtCloseFile(FileHandle);
    }

    printf("Client thread [%zd] ending\n", (size_t)pthread_self());

    return NULL;

error:

    goto cleanup;
}

static
NTSTATUS
HandleSignals(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bDone = FALSE;
    struct sigaction action;
    sigset_t catch_signal_mask;

    // After starting up threads, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = InterruptHandler;

    if (sigaction(SIGINT, &action, NULL) != 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    ntStatus = LwErrnoToNtStatus(pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL));
    BAIL_ON_NT_STATUS(ntStatus);

    while (!bDone)
    {
        switch (GetNextSignal())
        {
            case SIGINT:
            case SIGTERM:

                SetProcessMustExit(TRUE);

                bDone = TRUE;
                break;

            default:

                break;
        }
    }

error:

    return ntStatus;
}

static
VOID
InterruptHandler(
    int sig
    )
{
    if (sig == SIGINT)
    {
        raise(SIGTERM);
    }
}

static
NTSTATUS
BlockSignals(
    VOID
    )
{
    sigset_t blockedSignals;

    GetBlockedSignals(&blockedSignals);

    return LwErrnoToNtStatus(pthread_sigmask(SIG_BLOCK, &blockedSignals, NULL));
}

static
int
GetNextSignal(
    VOID
    )
{
    sigset_t blockedSignals;
    int sig = 0;

    GetBlockedSignalsSansInterrupt(&blockedSignals);

    sigwait(&blockedSignals, &sig);

    return sig;
}

static
VOID
GetBlockedSignals(
    sigset_t* pBlockedSignals
    )
{
    sigemptyset(pBlockedSignals);
    sigaddset(pBlockedSignals, SIGTERM);
    sigaddset(pBlockedSignals, SIGINT);
    sigaddset(pBlockedSignals, SIGPIPE);
    sigaddset(pBlockedSignals, SIGHUP);
}

static
VOID
GetBlockedSignalsSansInterrupt(
    sigset_t* pBlockedSignals
    )
{
    sigemptyset(pBlockedSignals);
    sigaddset(pBlockedSignals, SIGTERM);
    sigaddset(pBlockedSignals, SIGPIPE);
    sigaddset(pBlockedSignals, SIGHUP);
}

static
BOOLEAN
ProcessMustExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    pthread_mutex_lock(&gNpClientTestMutex);

    bExit = gbExitProcess;

    pthread_mutex_unlock(&gNpClientTestMutex);

    return bExit;
}

static
VOID
SetProcessMustExit(
    BOOLEAN bExit
    )
{
    pthread_mutex_lock(&gNpClientTestMutex);

    gbExitProcess = bExit;

    pthread_mutex_unlock(&gNpClientTestMutex);
}
