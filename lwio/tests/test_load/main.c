#include "config.h"
#include <lw/base.h>
#include <lw/rtlgoto.h>
#include <lwio/lwio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <wc16str.h>
#include <termios.h>

#define PAYLOAD "Hello, World!\r\n"

typedef struct _LOAD_THREAD
{
    pthread_t Thread;
    ULONG ulNumber;
} LOAD_THREAD, *PLOAD_THREAD;

typedef struct _LOAD_FILE
{
    IO_FILE_NAME Filename;
    IO_FILE_HANDLE hHandle;
} LOAD_FILE, *PLOAD_FILE;

struct
{
    ULONG ulThreadCount;
    ULONG ulConnectionsPerThread;
    ULONG ulIterations;
    PCSTR pszServer;
    PCSTR pszShare;
    PCSTR pszUser;
    PCSTR pszDomain;
    PCSTR pszPassword;
    BOOLEAN volatile bStart;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    ULONG ulFailureCount;
    BOOLEAN bContinueOnError;
    
} gState =
{
    .ulThreadCount = 100,
    .ulConnectionsPerThread = 100,
    .ulIterations = 10,
    .bStart = FALSE,
    .Lock = PTHREAD_MUTEX_INITIALIZER,
    .Event = PTHREAD_COND_INITIALIZER,
    .ulFailureCount = 0,
    .bContinueOnError = FALSE
};

static
PVOID
LoadThread(
    PVOID pData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLOAD_THREAD pThread = (PLOAD_THREAD) pData;
    ULONG ulIter = 0;
    ULONG ulFile = 0;
    PLOAD_FILE pFiles = NULL;
    PLOAD_FILE pFile = NULL;
    static const CHAR szPayload[] = PAYLOAD;
    CHAR szCompare[sizeof(szPayload)];
    IO_STATUS_BLOCK ioStatus = {0};
    ULONG64 offset = 0;
    CHAR szHostname[256] = {0};
    LW_PIO_CREDS pCreds = NULL;

    if (gethostname(szHostname, sizeof(szHostname) -1) != 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pFiles, LOAD_FILE, sizeof(*pFiles) * gState.ulConnectionsPerThread);
    GOTO_ERROR_ON_STATUS(status);

    printf("[%u] Creating start up environment...\n",
           pThread->ulNumber);

    for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
    {
        pFile = &pFiles[ulFile];

        status = LwRtlUnicodeStringAllocatePrintfW(
            &pFile->Filename.Name,
            L"/rdr/%s@%u/%s/test-load-%s-%u.txt",
            gState.pszServer,
            pThread->ulNumber * gState.ulThreadCount + ulFile,
            gState.pszShare,
            szHostname,
            pThread->ulNumber * gState.ulThreadCount + ulFile);
        GOTO_ERROR_ON_STATUS(status);
    }

    if (gState.pszUser && gState.pszDomain && gState.pszPassword)
    {
        status = LwIoCreatePlainCredsA(gState.pszUser, gState.pszDomain, gState.pszPassword, &pCreds);
        GOTO_ERROR_ON_STATUS(status);

        status = LwIoSetThreadCreds(pCreds);
        GOTO_ERROR_ON_STATUS(status);

        LwIoDeleteCreds(pCreds);
    }

    pthread_mutex_lock(&gState.Lock);
    while (!gState.bStart)
    {
        pthread_cond_wait(&gState.Event, &gState.Lock);
    }
    pthread_mutex_unlock(&gState.Lock);


    for (ulIter = 0; ulIter < gState.ulIterations; ulIter++)
    {
        printf("[%u] Starting iteration %d of %d...\n",
               pThread->ulNumber,
               ulIter + 1,
               gState.ulIterations);

        /* Pass 1 -- open files for writing */

        printf("[%u] Opening %u files for writing...\n",
               pThread->ulNumber,
               gState.ulConnectionsPerThread);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            status = LwNtCreateFile(
                &pFile->hHandle,       /* File handle */
                NULL,                  /* Async control block */
                &ioStatus,             /* IO status block */
                &pFile->Filename,      /* Filename */
                NULL,                  /* Security descriptor */
                NULL,                  /* Security QOS */
                FILE_GENERIC_WRITE,    /* Desired access mask */
                0,                     /* Allocation size */
                0,                     /* File attributes */
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,     /* Share access */
                FILE_OVERWRITE_IF,     /* Create disposition */
                0,                     /* Create options */
                NULL,                  /* EA buffer */
                0,                     /* EA length */
                NULL,                  /* ECP list */
                NULL);
            if (status != STATUS_SUCCESS)
            {
                gState.ulFailureCount++;

                fprintf(stderr, "Error Opening File: %s (%x).  Failure Count == %d \n",
                        LwNtStatusToName(status), 
                        status,  
                        gState.ulFailureCount);

                if (!gState.bContinueOnError)
                {
                    GOTO_ERROR_ON_STATUS(status);
                }
            }
        }
      
        /* Pass 2 -- write payload into each file */

        printf("[%u] Writing to files...\n", pThread->ulNumber);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            offset = 0;

            if (pFile->hHandle)
            {
                status = LwNtWriteFile(
                    pFile->hHandle, /* File handle */
                    NULL, /* Async control block */
                    &ioStatus, /* IO status block */
                    (PVOID) szPayload, /* Buffer */
                    sizeof(szPayload), /* Buffer size */
                    &offset, /* File offset */
                    NULL); /* Key */
                GOTO_ERROR_ON_STATUS(status);
            }
        }

        /* Pass 3 -- reopen each file for reading */

        printf("[%u] Reopening files for reading...\n", pThread->ulNumber);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            if (pFile->hHandle)
            {
                status = LwNtCloseFile(pFile->hHandle);
                GOTO_ERROR_ON_STATUS(status);

                pFile->hHandle = NULL;

                status = LwNtCreateFile(
                    &pFile->hHandle,       /* File handle */
                    NULL,                  /* Async control block */
                    &ioStatus,             /* IO status block */
                    &pFile->Filename,      /* Filename */
                    NULL,                  /* Security descriptor */
                    NULL,                  /* Security QOS */
                    FILE_GENERIC_READ |
                    DELETE,                /* Desired access mask */
                    0,                     /* Allocation size */
                    0,                     /* File attributes */
                    FILE_SHARE_READ |
                    FILE_SHARE_WRITE |
                    FILE_SHARE_DELETE,     /* Share access */
                    FILE_OPEN,             /* Create disposition */
                    FILE_DELETE_ON_CLOSE,  /* Create options */
                    NULL,                  /* EA buffer */
                    0,                     /* EA length */
                    NULL,                  /* ECP list */
                    NULL);
                GOTO_ERROR_ON_STATUS(status);
            }
        }

        /* Pass 4 -- read back each payload and compare */

        printf("[%u] Reading files...\n", pThread->ulNumber);

        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            offset = 0;

            if (pFile->hHandle)
            {
                status = LwNtReadFile(
                    pFile->hHandle, /* File handle */
                    NULL, /* Async control block */
                    &ioStatus, /* IO status block */
                    szCompare, /* Buffer */
                    sizeof(szCompare), /* Buffer size */
                    &offset, /* File offset */
                    NULL); /* Key */
                GOTO_ERROR_ON_STATUS(status);

                if (ioStatus.BytesTransferred != sizeof(szCompare) ||
                    memcmp(szCompare, szPayload, sizeof(szCompare)))
                {
                    status = STATUS_UNSUCCESSFUL;
                    GOTO_ERROR_ON_STATUS(status);
                }
            }
        }

        /* Pass 5 -- close handles */

        printf("[%u] Closing files...\n", pThread->ulNumber);
                                                                 
        for (ulFile = 0; ulFile < gState.ulConnectionsPerThread; ulFile++)
        {
            pFile = &pFiles[ulFile];

            if (pFile->hHandle)
            {
                status = LwNtCloseFile(pFile->hHandle);
                GOTO_ERROR_ON_STATUS(status);
            }
        }
    }

    fprintf(stderr, "Total number of failed opens == %d \n",
            gState.ulFailureCount);

error:

    if (status != STATUS_SUCCESS)
    {
        fprintf(stderr, "Error: %s (%x)\n", LwNtStatusToName(status), status);
        abort();
    }


    return NULL;
}

static
NTSTATUS
PromptPassword(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    static CHAR szBuffer[128] = {0};
    struct termios old, new;
    int index = 0;

    if (tcgetattr(0, &old) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    memcpy(&new, &old, sizeof(struct termios));

    new.c_lflag &= ~(ECHO);

    if (tcsetattr(0, TCSANOW, &new) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    fprintf(stdout, "Password: ");
    fflush(stdout);

    for (index = 0; index < sizeof(szBuffer) - 1; index++)
    {
        if (read(0, &szBuffer[index], 1) < 0)
        {
            status = LwErrnoToNtStatus(errno);
            GOTO_ERROR_ON_STATUS(status);
        }
        
        if (szBuffer[index] == '\n')
        {
            szBuffer[index] = '\0';
            break;
        }
    }

    gState.pszPassword = szBuffer;

    if (tcsetattr(0, TCSANOW, &old) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        GOTO_ERROR_ON_STATUS(status);
    }

    fprintf(stdout, "\n");
    fflush(stdout);

error:

    return status;
}

static
NTSTATUS
Run(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLOAD_THREAD pThreads = NULL;
    PLOAD_THREAD pThread = NULL;
    ULONG ulThread = 0;

    if (gState.pszUser && gState.pszDomain && !gState.pszPassword)
    {
        status = PromptPassword();
        GOTO_ERROR_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pThreads, LOAD_THREAD, sizeof(*pThreads) * gState.ulThreadCount);
    GOTO_ERROR_ON_STATUS(status);

    for (ulThread = 0; ulThread < gState.ulThreadCount; ulThread++)
    {
        pThread = &pThreads[ulThread];

        pThread->ulNumber = ulThread;

        status = LwErrnoToNtStatus(
            pthread_create(
                &pThread->Thread,
                NULL,
                LoadThread,
                pThread));
        GOTO_ERROR_ON_STATUS(status);
    }

    pthread_mutex_lock(&gState.Lock);
    gState.bStart = TRUE;
    pthread_cond_broadcast(&gState.Event);
    pthread_mutex_unlock(&gState.Lock);
        

    for (ulThread = 0; ulThread < gState.ulThreadCount; ulThread++)
    {
        pThread = &pThreads[ulThread];

        status = LwErrnoToNtStatus(pthread_join(pThread->Thread, NULL));
        GOTO_ERROR_ON_STATUS(status);
    }

error:

    RTL_FREE(&pThreads);

    return status;
}

static
VOID
Usage(
    PCSTR pszProgram
    )
{
    printf("Usage: %s [ options ... ] server share\n", pszProgram);
}

static
VOID
Help(
    PCSTR pszProgram
    )
{
    Usage(pszProgram);

    printf(
        "\n"
        "Options:\n"
        "\n"
        "  --help                            Show this help\n"
        "  --user <name>                     Specify user name (default: use current Kerberos credentials)\n"
        "  --domain <domain>                 Specify domain of user (required when using --user)\n"
        "  --password <name>                 Specify user password (default: prompt interactively)\n"
        "  --iterations count                Number of iterations of open-write-read-close cycle\n"
        "  --threads count                   Number of threads to spawn\n"
        "  --connections count               Number of connections to create per thread\n");
}

static
VOID
ParseArgs(
    int argc,
    char** ppszArgv
    )
{
    int i = 0;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(ppszArgv[i], "--help"))
        {
            Help(ppszArgv[0]);
            exit(0);
        }
        else if (!strcmp(ppszArgv[i], "--iterations"))
        {
            if (i + 1 == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.ulIterations = atoi(ppszArgv[++i]);
        }
        else if (!strcmp(ppszArgv[i], "--threads"))
        {
            if (i + 1 == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.ulThreadCount = atoi(ppszArgv[++i]);
        }
        else if (!strcmp(ppszArgv[i], "--connections"))
        {
            if (i + 1 == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.ulConnectionsPerThread = atoi(ppszArgv[++i]);
        }
        else if (!strcmp(ppszArgv[i], "--user"))
        {
            if (i + 1 == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.pszUser = ppszArgv[++i];
        }
        else if (!strcmp(ppszArgv[i], "--domain"))
        {
            if (i + 1 == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.pszDomain = ppszArgv[++i];
        }
        else if (!strcmp(ppszArgv[i], "--password"))
        {
            if (i + 1 == argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }
            gState.pszPassword = ppszArgv[++i];
        }
        else if (!strcmp(ppszArgv[i], "--continue-on-error"))
        {
            gState.bContinueOnError = TRUE;
        }
        else
        {
            if (i + 1 >= argc)
            {
                Usage(ppszArgv[0]);
                exit(1);
            }

            gState.pszServer = ppszArgv[i];
            gState.pszShare = ppszArgv[++i]; 

            return;
        }
    }

    if (!gState.pszServer)
    {
        Usage(ppszArgv[0]);
        exit(1);
    }
}

int
main(
    int argc,
    char** ppszArgv
    )
{
    ParseArgs(argc, ppszArgv);

    return Run();
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

