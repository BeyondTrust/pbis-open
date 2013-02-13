/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Server main logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

PLW_THREAD_POOL gpPool;

SM_GLOBAL_STATE gState =
{
    .pIpcContext = NULL,
    .pContolProtocol = NULL,
    .pControlServer = NULL,
    .bStartAsDaemon = FALSE,
    .notifyPipe = {-1, -1},
    .logLevel = 0,
    .pszLogFilePath = NULL,
    .bSyslog = FALSE,
    .bDisableAutostart = FALSE,
    .bWatchdog = TRUE,
    .ControlLock = -1
};

static
DWORD
LwSmParseArguments(
    int argc,
    char** ppszArgv
    );

static
DWORD
LwSmDaemonize(
    VOID
    );

static
DWORD
LwSmMain(
    VOID
    );

static
DWORD
LwSmConfigureLogging(
    PCSTR pszProgramName
    );

static
DWORD
LwSmIpcInit(
    VOID
    );

static
DWORD
LwSmStartIpcServer(
    VOID
    );

static
DWORD
LwSmStopIpcServer(
    VOID
    );

static
DWORD
LwSmShutdownServices(
    VOID
    );

static
DWORD
LwSmShutdownService(
    PSM_TABLE_ENTRY pEntry
    );

static
DWORD
LwSmNotify(
    DWORD Error
    );

static
DWORD
LwSmControlLock(
    VOID
    );

int
main(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;

    /* Parse command line */
    dwError = LwSmParseArguments(argc, ppszArgv);
    BAIL_ON_ERROR(dwError);

    /* Block all signals */
    dwError = LwNtStatusToWin32Error(LwRtlBlockSignals());
    BAIL_ON_ERROR(dwError);

    /* Fork into background if running as a daemon */
    if (gState.bStartAsDaemon)
    {
        dwError = LwSmDaemonize();
        BAIL_ON_ERROR(dwError);
    }

    /* If we're starting as the control server, acquire lock */
    if (!gState.bContainer)
    {
        dwError = LwSmControlLock();
        BAIL_ON_ERROR(dwError);
    }

    /* Create thread pool */
    dwError = LwNtStatusToWin32Error(LwRtlCreateThreadPool(&gpPool, NULL));
    BAIL_ON_ERROR(dwError);

    dwError = LWNetExtendEnvironmentForKrb5Affinity(FALSE);
    BAIL_ON_ERROR(dwError);

    /* Mac OS X - avoid potential circular calls into directory services */
    dwError = LwDsCacheAddPidException(getpid());
    BAIL_ON_ERROR(dwError);

    /* Initialize i18n */
    setlocale(LC_ALL, "");

    /* Initialize logging subsystem */
    LwSmLogInit();

    /* Set up logging */
    dwError = LwSmConfigureLogging(gState.pName);
    BAIL_ON_ERROR(dwError);

    /* Initialize the container subsystem */
    dwError = LwSmContainerInit();
    BAIL_ON_ERROR(dwError);

    /* Initialize the service table subsystem */
    dwError = LwSmTableInit();
    BAIL_ON_ERROR(dwError);

    /* Enter main loop */
    dwError = LwSmMain();
    BAIL_ON_ERROR(dwError);

error:

    /* If we are starting as a daemon and have not
       notified the parent process yet, notify it
       of an error now */
    if (gState.bStartAsDaemon && !gState.bNotified)
    {
        LwSmNotify(dwError);
    }

    /* Shut down service table */
    LwSmTableShutdown();

    /* Shut down containers */
    LwSmContainerShutdown();

    /* Remove DS cache exception */
    LwDsCacheRemovePidException(getpid());

    /* Free thread pool */
    LwRtlFreeThreadPool(&gpPool);

    /* Free the service thread pool to ensure the service finished shutting
     * down.*/
    LwRtlSvcmFreePool();

    /* Shut down logging */
    LwSmLoggingShutdown();

    /* Close control file if it is open */
    if (gState.ControlLock >= 0)
    {
        close(gState.ControlLock);
    }

    if (dwError)
    {
        fprintf(stderr, "Error: %s (%d)\n", LwWin32ExtErrorToName(dwError), (int) dwError);
    }

    return dwError ? 1 : 0;
}

static
DWORD
Usage(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;

    printf("Usage: %s [ options ... ]\n\n", pArgv[0]);
    printf("Options:\n"
           "    --start-as-daemon      Start as a background process\n"
           "    --syslog               Log to syslog (default when starting as daemon)\n"
           "    --logfile              Log to file\n"
           "    --loglevel <level>     Set log level to <level>\n"
           "                           (error, warning, info, verbose, debug, trace)\n"
           "    --container <group>    Start as a container for service group <group>\n"
           "    --help                 Show usage information\n");

    return dwError;
}

static
DWORD
LwSmParseArguments(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;
    int i = 0;

    if (!strcmp(ppszArgv[0], CONTAINER_PROCESS_NAME))
    {
        if (argc != 2)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_ERROR(dwError);
        }
        gState.bContainer = TRUE;
        gState.pName = ppszArgv[1];
    }
    else
    {
        gState.pName = ppszArgv[0];

        for (i = 1; i < argc; i++)
        {
            if (!strcmp(ppszArgv[i], "--start-as-daemon"))
            {
                gState.bStartAsDaemon = TRUE;
            }
            else if (!strcmp(ppszArgv[i], "--syslog"))
            {
                gState.bSyslog = TRUE;
            }
            else if (!strcmp(ppszArgv[i], "--loglevel"))
            {
                if (++i >= argc)
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_ERROR(dwError);
                }

                dwError = LwSmLogLevelNameToLogLevel(
                    ppszArgv[i],
                    &gState.logLevel);
                BAIL_ON_ERROR(dwError);
            }
            else if (!strcmp(ppszArgv[i], "--logfile"))
            {
                if (++i >= argc)
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_ERROR(dwError);
                }

                gState.pszLogFilePath = ppszArgv[i];
            }
            else if (!strcmp(ppszArgv[i], "--disable-autostart"))
            {
                gState.bDisableAutostart = TRUE;
            }
            else if (!strcmp(ppszArgv[i], "--container"))
            {
                if (++i >= argc)
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_ERROR(dwError);
                }

                dwError = LwMbsToWc16s(ppszArgv[i], &gState.pGroup);
                BAIL_ON_ERROR(dwError);

                gState.bContainer = TRUE;
            }
            else if (!strcmp(ppszArgv[i], "--help"))
            {
                Usage(argc, ppszArgv);
                exit(0);
            }
        }
    }

error:

    return dwError;
}

static
DWORD
LwSmControlLock(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    struct flock lock = {0};

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if ((gState.ControlLock = open(CONTROL_LOCK, O_WRONLY | O_CREAT | O_TRUNC, 0200)) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_ERROR(dwError);
    }

    if (fcntl(gState.ControlLock, F_SETFD, FD_CLOEXEC) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_ERROR(dwError);
    }

    if (fcntl(gState.ControlLock, F_SETLK, &lock) < 0)
    {
        switch(errno)
        {
        case EACCES:
        case EAGAIN:
            dwError = ERROR_SERVICE_ALREADY_RUNNING;
            break;
        default:
            dwError = LwErrnoToWin32Error(errno);
            break;
        }

        BAIL_ON_ERROR(dwError);
    }

error:

    return dwError;
}

static
DWORD
LwSmDaemonize(
    VOID
    )
{
    DWORD dwError = 0;
    pid_t pid = -1;
    int ret = 0;
    int devNull = -1;
    int i = 0;

    /* Open a pipe so the daemon process can notify us
       when it is ready to accept connections.  This means
       the foreground process will not exit until the daemon
       is fully usable */
    if (pipe(gState.notifyPipe) != 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    pid = fork();

    if (pid < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }
    else if (pid > 0)
    {
        /* We are the foreground process */

        /* Close the write end of the pipe since we don't need it */
        close(gState.notifyPipe[1]);

        /* Wait until daemon process indicates it is fully started by sending error code */
        do
        {
            ret = read(gState.notifyPipe[0], &dwError, sizeof(dwError));
        } while (ret < 0 && errno == EINTR);

        if (dwError)
        {
            fprintf(stderr, "Error: %s (%d)\n", LwWin32ExtErrorToName(dwError), (int) dwError);
        }

        exit(dwError ? 1 : 0);
    }
    
    /* We are the intermediate background process.
       Isolate ourselves from the state of the foreground process
       by changing directory, becoming a session leader,
       redirecting stdout/stderr/stdin to /dev/null, and setting
       a reasonable umask */
    if (chdir("/") != 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    if (setsid() < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    if ((devNull = open("/dev/null", O_RDWR)) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    for (i = 0; i <= 2; i++)
    {
        if (dup2(devNull, i) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
    }

    close(devNull);
    umask(0022);

    /* Now that we are isolated, fork the actual daemon process and exit */
    pid = fork();

    if (pid < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }
    else if (pid > 0)
    {
        /* The intermediate background process exits here */
        exit(0);
    }

    /* We are the actual daemon process, continue with startup */

    /* Close the read end of the notification pipe now since we don't need it */
    close(gState.notifyPipe[0]);

    /* Prevent the write end of the notification pipe from being inherited by children */
    if (fcntl(gState.notifyPipe[1], F_SETFD, FD_CLOEXEC) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

error:

    return dwError;
}

static
DWORD
LwSmNotify(
    DWORD Error
    )
{
    DWORD dwError = 0;
    int ret = 0;

    do 
    {
        ret = write(gState.notifyPipe[1], &Error, sizeof(Error));
    } while (ret < 0 && errno == EINTR);

    if (ret < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    close(gState.notifyPipe[1]);

    return dwError;

error:

    goto cleanup;
}

static
VOID
Startup(
    PVOID pUnused
    )
{
    DWORD dwError = 0;

    SM_LOG_INFO("Likewise Service Manager starting up");

    /* Initialize IPC system and direct container endpoint */
    dwError = LwSmIpcInit();
    BAIL_ON_ERROR(dwError);

    if (!gState.bContainer)
    {
        /* Bootstrap ourselves by adding and starting any
           services we need to run (e.g. registry) */
        dwError = LwSmBootstrap();
        BAIL_ON_ERROR(dwError);

        /* Read configuration and populate service table */
        dwError = LwSmPopulateTable();
        BAIL_ON_ERROR(dwError);

        /* Start services */
        if (!gState.bDisableAutostart)
        {
            dwError = LwSmAutostartServices();
            BAIL_ON_ERROR(dwError);
        }
    }

    /* Start IPC servers */
    dwError = LwSmStartIpcServer();
    BAIL_ON_ERROR(dwError);

    /* If we are starting as a daemon, indicate that we
       are ready to the parent process.  This ensures that
       the parent does not exit until we are actually accepting
       IPC connections */
    if (gState.bStartAsDaemon)
    {
        dwError = LwSmNotify(0);
        BAIL_ON_ERROR(dwError);
        gState.bNotified = TRUE;
    }

    SM_LOG_INFO("Likewise Service Manager startup complete");

    return;

error:

    LwRtlExitMain(STATUS_UNSUCCESSFUL);
}

static
VOID
RefreshConfig(
    PVOID pUnused
    )
{
    DWORD dwError = 0;

    dwError = LwSmPopulateTable();
    BAIL_ON_ERROR(dwError);

    return;

error:

    LwRtlExitMain(STATUS_UNSUCCESSFUL);
}

static
VOID
Shutdown(
    PVOID pUnused
    )
{
    DWORD dwError = 0;

    if (!gState.bContainer)
    {
        /* Shut down all running services  */
        dwError = LwSmShutdownServices();
        BAIL_ON_ERROR(dwError);
    }
    
    /* Stop IPC server */
    dwError = LwSmStopIpcServer();
    BAIL_ON_ERROR(dwError);

    /* Exit from main loop */
    LwRtlExitMain(STATUS_SUCCESS);

    return;

error:
    
    LwRtlExitMain(STATUS_UNSUCCESSFUL);
}
 

static
VOID
MainTask(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    siginfo_t info;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        status = LwRtlSetTaskUnixSignal(pTask, SIGTERM, TRUE);
        BAIL_ON_ERROR(status);

        status = LwRtlSetTaskUnixSignal(pTask, SIGINT, TRUE);
        BAIL_ON_ERROR(status);

        status = LwRtlSetTaskUnixSignal(pTask, SIGHUP, TRUE);
        BAIL_ON_ERROR(status);
        
        status = LwRtlQueueWorkItem(gpPool, Startup, NULL, 0);
        BAIL_ON_ERROR(status);

        *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;
    }
    else if (WakeMask & LW_TASK_EVENT_UNIX_SIGNAL)
    {
        while (LwRtlNextTaskUnixSignal(pTask, &info))
        {
            switch(info.si_signo)
            {
            case SIGTERM:
            case SIGINT:
                SM_LOG_VERBOSE(info.si_signo == SIGINT ?
                    "Shutting down on SIGINT" :
                    "Shutting down on SIGTERM");
                /* Shutting down stops all running services, which is a blocking operation */
                status = LwRtlQueueWorkItem(gpPool, Shutdown, NULL, 0);
                BAIL_ON_ERROR(status);
                *pWaitMask = LW_TASK_EVENT_COMPLETE;
                goto cleanup;
            case SIGHUP:
                SM_LOG_VERBOSE("Refreshing configuration on SIGHUP");
                /* Refreshing config reads from the registry, which is a blocking operation */
                status = LwRtlQueueWorkItem(gpPool, RefreshConfig, NULL, 0);
                BAIL_ON_ERROR(status);
                break;
            default:
                break;
            }
        }
        
        *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;
    }

cleanup:

    return;

error:

    if (status)
    {
        LwRtlExitMain(status);
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }

    goto cleanup;
}

static
DWORD
LwSmMain(
    VOID
    )
{
    DWORD dwError = 0;
    PLW_TASK pTask = NULL;

    dwError = LwNtStatusToWin32Error(LwRtlCreateTask(
                                         gpPool,
                                         &pTask,
                                         NULL,
                                         MainTask,
                                         NULL));
    BAIL_ON_ERROR(dwError);

    LwRtlWakeTask(pTask);

    dwError = LwNtStatusToWin32Error(LwRtlMain());
    BAIL_ON_ERROR(dwError);

cleanup:

     if (pTask)
     {
         LwRtlCancelTask(pTask);
         LwRtlWaitTask(pTask);
         LwRtlReleaseTask(&pTask);
     }

     return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmConfigureLogging(
    PCSTR pszProgramName
    )
{
    DWORD dwError = 0;

    LwSmLoggingInit(pszProgramName);

    if (gState.logLevel == 0)
    {
        if (gState.bStartAsDaemon || (gState.bContainer && !gState.pGroup))
        {
            gState.logLevel = LW_SM_LOG_LEVEL_WARNING;
        }
        else
        {
            gState.logLevel = LW_SM_LOG_LEVEL_VERBOSE;
        }
    }

    LwSmSetMaxLogLevel(NULL, gState.logLevel);

    if (gState.pszLogFilePath)
    {
        dwError = LwSmSetLoggerToPath(NULL, gState.pszLogFilePath);
        BAIL_ON_ERROR(dwError);
    }
    else if (gState.bSyslog || gState.bStartAsDaemon || (gState.bContainer && !gState.pGroup))
    {
        dwError = LwSmSetLoggerToSyslog(NULL);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LwSmSetLoggerToFile(NULL, stderr);
        BAIL_ON_ERROR(dwError);
    }

error:

    return dwError;
}

static
LWMsgBool
LwSmLogIpc (
    LWMsgLogLevel level,
    const char* pszMessage,
    const char* pszFunction,
    const char* pszFilename,
    unsigned int line,
    void* pData
    )
{
    LW_SM_LOG_LEVEL smLevel = LW_SM_LOG_LEVEL_DEBUG;
    LWMsgBool result = LWMSG_FALSE;

    switch (level)
    {
    case LWMSG_LOGLEVEL_ALWAYS:
        smLevel = LW_SM_LOG_LEVEL_ALWAYS;
        break;
    case LWMSG_LOGLEVEL_ERROR:
        smLevel = LW_SM_LOG_LEVEL_ERROR;
        break;
    case LWMSG_LOGLEVEL_WARNING:
        smLevel = LW_SM_LOG_LEVEL_WARNING;
        break;
    case LWMSG_LOGLEVEL_INFO:
        smLevel = LW_SM_LOG_LEVEL_INFO;
        break;
    case LWMSG_LOGLEVEL_VERBOSE:
        smLevel = LW_SM_LOG_LEVEL_VERBOSE;
        break;
    case LWMSG_LOGLEVEL_DEBUG:
        smLevel = LW_SM_LOG_LEVEL_DEBUG;
        break;
    case LWMSG_LOGLEVEL_TRACE:
        smLevel = LW_SM_LOG_LEVEL_TRACE;
        break;
    }

    if (LwRtlLogGetLevel() >= smLevel)
    {
        if (pszMessage)
        {
            LwSmLogMessage(
                smLevel,
                "lwsm-ipc",
                pszFunction,
                pszFilename,
                line,
                pszMessage);
        }
        result = LWMSG_TRUE;
    }
    else
    {
        result = LWMSG_FALSE;
    }

    return result;
}

static
DWORD
LwSmIpcInit(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = MAP_LWMSG_STATUS(lwmsg_context_new(NULL, &gState.pIpcContext));
    BAIL_ON_ERROR(dwError);

    lwmsg_context_set_log_function(
        gState.pIpcContext,
        LwSmLogIpc,
        NULL);
    
    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(gState.pIpcContext, &gState.pContainerProtocol));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_add_protocol_spec(
        gState.pContainerProtocol,
        LwSmGetContainerProtocolSpec()));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_new(
        gState.pIpcContext,
        gState.pContainerProtocol,
        &gState.pDirectServer));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_listen_endpoint(
        gState.pDirectServer,
        LWMSG_ENDPOINT_DIRECT,
        "Container",
        0));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_dispatch_spec(
        gState.pDirectServer,
        LwSmGetContainerDispatchSpec()));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_start_listen(gState.pDirectServer));
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
LwSmStartIpcServer(
    VOID
    )
{
    DWORD dwError = 0;

    SM_LOG_VERBOSE("Starting IPC server");

    if (!gState.bContainer)
    {
        dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(gState.pIpcContext, &gState.pContolProtocol));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_protocol_add_protocol_spec(
            gState.pContolProtocol,
            LwSmIpcGetProtocolSpec()));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_new(
            gState.pIpcContext,
            gState.pContolProtocol,
            &gState.pControlServer));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_dispatch_spec(
            gState.pControlServer,
            LwSmGetDispatchSpec()));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_listen_endpoint(
            gState.pControlServer,
            LWMSG_ENDPOINT_LOCAL,
            SM_ENDPOINT,
            0666));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_start_listen(gState.pControlServer));
        BAIL_ON_ERROR(dwError);
    }

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_new(
        gState.pIpcContext,
        gState.pContainerProtocol,
        &gState.pContainerServer));
    BAIL_ON_ERROR(dwError);

    if (gState.bContainer)
    {
        dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_dispatch_spec(
            gState.pContainerServer,
            LwSmGetContainerDispatchSpec()));
        BAIL_ON_ERROR(dwError);

        if (!gState.pGroup)
        {
            dwError = MAP_LWMSG_STATUS(lwmsg_peer_accept_fd(
                gState.pContainerServer,
                LWMSG_ENDPOINT_PAIR,
                4));
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_connect_endpoint(
                gState.pContainerServer,
                LWMSG_ENDPOINT_LOCAL,
                SC_ENDPOINT));
            BAIL_ON_ERROR(dwError);

            dwError = LwSmContainerRegister(gState.pContainerServer, gState.pGroup);
            BAIL_ON_ERROR(dwError);
        }
    }
    else
    {
        dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_listen_endpoint(
            gState.pContainerServer,
            LWMSG_ENDPOINT_LOCAL,
            SC_ENDPOINT,
            0666));

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_dispatch_spec(
            gState.pContainerServer,
            LwSmGetContainerRegisterDispatchSpec()));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_start_listen(gState.pContainerServer));
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStopIpcServer(
    VOID
    )
{
    DWORD dwError = 0;

    if (gState.pControlServer)
    {
        dwError = MAP_LWMSG_STATUS(lwmsg_peer_stop_listen(gState.pControlServer));
        BAIL_ON_ERROR(dwError);
    }

    if (gState.pContainerServer)
    {
        if (gState.pGroup)
        {
            dwError = MAP_LWMSG_STATUS(lwmsg_peer_disconnect(gState.pContainerServer));
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            dwError = MAP_LWMSG_STATUS(lwmsg_peer_stop_listen(gState.pContainerServer));
            BAIL_ON_ERROR(dwError);
        }
    }

    if (gState.pDirectServer)
    {
        dwError = MAP_LWMSG_STATUS(lwmsg_peer_stop_listen(gState.pDirectServer));
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    if (gState.pControlServer)
    {
        lwmsg_peer_delete(gState.pControlServer);
    }

    if (gState.pContainerServer)
    {
        lwmsg_peer_delete(gState.pContainerServer);
    }

    if (gState.pDirectServer)
    {
        lwmsg_peer_delete(gState.pDirectServer);
    }

    if (gState.pContolProtocol)
    {
        lwmsg_protocol_delete(gState.pContolProtocol);
    }

    if (gState.pContainerProtocol)
    {
        lwmsg_protocol_delete(gState.pContainerProtocol);
    }

    if (gState.pIpcContext)
    {
        lwmsg_context_delete(gState.pIpcContext);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmShutdownServices(
    VOID
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceNames = NULL;
    size_t i = 0;
    PSM_TABLE_ENTRY pEntry = NULL;

    SM_LOG_INFO("Shutting down running services");

    dwError = LwSmTableEnumerateEntries(&ppwszServiceNames);
    BAIL_ON_ERROR(dwError);
    
    for (i = 0; ppwszServiceNames[i]; i++)
    {
        dwError = LwSmTableGetEntry(ppwszServiceNames[i], &pEntry);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmShutdownService(pEntry);
        if (dwError)
        {
            /* Ignore errors and try to shut down everything we can */
            SM_LOG_WARNING("Could not shut down service: %u\n", (unsigned int) dwError);
            dwError = 0;
        }

        LwSmTableReleaseEntry(pEntry);
        pEntry = NULL;
    }

cleanup:

    if (ppwszServiceNames)
    {
        LwSmFreeStringList(ppwszServiceNames);
    }

    if (pEntry)
    {
        LwSmTableReleaseEntry(pEntry);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmShutdownService(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PSM_TABLE_ENTRY pDep = NULL;
    PWSTR* ppwszDeps = NULL;
    size_t i = 0;

    dwError = LwSmTableGetEntryReverseDependencyClosure(pEntry, &ppwszDeps);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDeps[i]; i++)
    {
        dwError = LwSmTableGetEntry(ppwszDeps[i], &pDep);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableStopEntry(pDep);
        BAIL_ON_ERROR(dwError);

        LwSmTableReleaseEntry(pDep);
        pDep = NULL;
    }

    dwError = LwSmTableStopEntry(pEntry);
    BAIL_ON_ERROR(dwError);

cleanup:

    if (pDep)
    {
        LwSmTableReleaseEntry(pDep);
    }

    if (ppwszDeps)
    {
        LwSmFreeStringList(ppwszDeps);
    }

    return dwError;

error:

    goto cleanup;
}
