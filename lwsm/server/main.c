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

static struct
{
    PLW_THREAD_POOL pPool;
    LWMsgContext* pIpcContext;
    LWMsgProtocol* pIpcProtocol;
    LWMsgServer* pIpcServer;
    BOOLEAN bStartAsDaemon;
    BOOLEAN bNotified;
    int notifyPipe[2];
    LW_SM_LOG_LEVEL logLevel;
    PCSTR pszLogFilePath;
    BOOLEAN bSyslog;
} gState = 
{
    .pPool = NULL,
    .pIpcContext = NULL,
    .pIpcProtocol = NULL,
    .pIpcServer = NULL,
    .bStartAsDaemon = FALSE,
    .notifyPipe = {-1, -1},
    .logLevel = LW_SM_LOG_LEVEL_WARNING,
    .pszLogFilePath = NULL,
    .bSyslog = FALSE
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
LwSmPopulateTable(
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
    int status
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

    /* Set up logging */
    dwError = LwSmConfigureLogging(ppszArgv[0]);
    BAIL_ON_ERROR(dwError);

    /* Initialize the service loader subsystem */
    dwError = LwSmLoaderInitialize(&gTableCalls);
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
        LwSmNotify(1);
    }

    /* Shut down service table */
    LwSmTableShutdown();

    /* Shut down loaders */
    LwSmLoaderShutdown();

    /* Shut down logging */
    dwError = LwSmSetLogger(NULL, NULL);
    BAIL_ON_ERROR(dwError);

    return dwError ? 1 : 0;
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

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(ppszArgv[i], "--start-as-daemon"))
        {
            gState.bStartAsDaemon = TRUE;
            gState.bSyslog = TRUE;
            gState.logLevel = LW_SM_LOG_LEVEL_INFO;
        }
        else if (!strcmp(ppszArgv[i], "--syslog"))
        {
            gState.bSyslog = TRUE;
            gState.logLevel = LW_SM_LOG_LEVEL_INFO;
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
    char c = 1;
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

        /* Wait until daemon process indicates it is fully started */
        do
        {
            ret = read(gState.notifyPipe[0], &c, sizeof(c));
        } while (ret < 0 && errno == EINTR);

        /* Exit with code sent by daemon */
        exit((int) c);
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
    int status
    )
{
    DWORD dwError = 0;
    char c = (char) status;
    int ret = 0;

    do 
    {
        ret = write(gState.notifyPipe[1], &c, sizeof(c));
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

    /* Bootstrap ourselves by adding and starting any
       services we need to run (e.g. registry) */
    dwError = LwSmBootstrap();
    BAIL_ON_ERROR(dwError);
    
    /* Read configuration and populate service table */
    dwError = LwSmPopulateTable();
    BAIL_ON_ERROR(dwError);
    
    /* Start IPC server */
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

    /* Shut down all running services  */
    dwError = LwSmShutdownServices();
    BAIL_ON_ERROR(dwError);
    
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
        
        status = LwRtlQueueWorkItem(gState.pPool, Startup, NULL, 0);
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
                status = LwRtlQueueWorkItem(gState.pPool, Shutdown, NULL, 0);
                BAIL_ON_ERROR(status);
                *pWaitMask = LW_TASK_EVENT_COMPLETE;
                goto cleanup;
            case SIGHUP:
                SM_LOG_VERBOSE("Refreshing configuration on SIGHUP");
                /* Refreshing config reads from the registry, which is a blocking operation */
                status = LwRtlQueueWorkItem(gState.pPool, RefreshConfig, NULL, 0);
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

    dwError = LwNtStatusToWin32Error(LwRtlCreateThreadPool(&gState.pPool, NULL));
    BAIL_ON_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(LwRtlCreateTask(
                                         gState.pPool,
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

     LwRtlFreeThreadPool(&gState.pPool);

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

    LwSmSetMaxLogLevel(gState.logLevel);

    if (gState.pszLogFilePath)
    {
        dwError = LwSmSetLoggerToPath(gState.pszLogFilePath);
        BAIL_ON_ERROR(dwError);
    }
    else if (gState.bSyslog)
    {
        dwError = LwSmSetLoggerToSyslog(pszProgramName);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LwSmSetLoggerToFile(stderr);
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

    if (LwSmGetMaxLogLevel() >= smLevel)
    {
        if (pszMessage)
        {
            LwSmLogMessage(
                smLevel,
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
LwSmStartIpcServer(
    VOID
    )
{
    DWORD dwError = 0;

    SM_LOG_VERBOSE("Starting IPC server");

    dwError = MAP_LWMSG_STATUS(lwmsg_context_new(NULL, &gState.pIpcContext));
    BAIL_ON_ERROR(dwError);

    lwmsg_context_set_log_function(
        gState.pIpcContext,
        LwSmLogIpc,
        NULL);
    
    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(gState.pIpcContext, &gState.pIpcProtocol));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_add_protocol_spec(
                                   gState.pIpcProtocol,
                                   LwSmIpcGetProtocolSpec()));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_new(
                                   gState.pIpcContext,
                                   gState.pIpcProtocol,
                                   &gState.pIpcServer));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_add_dispatch_spec(
                                   gState.pIpcServer,
                                   LwSmGetDispatchSpec()));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_set_endpoint(
                                   gState.pIpcServer,
                                   LWMSG_CONNECTION_MODE_LOCAL,
                                   SM_ENDPOINT,
                                   0666));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_start(gState.pIpcServer));
    BAIL_ON_ERROR(dwError);

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

    if (gState.pIpcServer)
    {
        dwError = MAP_LWMSG_STATUS(lwmsg_server_stop(gState.pIpcServer));
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    if (gState.pIpcServer)
    {
        lwmsg_server_delete(gState.pIpcServer);
    }

    if (gState.pIpcProtocol)
    {
        lwmsg_protocol_delete(gState.pIpcProtocol);
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
LwSmPopulateTable(
    VOID
    )
{
    DWORD dwError = 0;
    HANDLE hReg = NULL;
    PWSTR* ppwszNames = NULL;
    PWSTR pwszName = NULL;
    PLW_SERVICE_INFO pInfo = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;
    size_t i = 0;

    SM_LOG_VERBOSE("Populating service table");

    dwError = RegOpenServer(&hReg);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryEnumServices(hReg, &ppwszNames);
    switch (dwError)
    {
    case LWREG_ERROR_NO_SUCH_KEY_OR_VALUE:
        /* No services in registry */
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszNames[i]; i++)
    {
        pwszName = ppwszNames[i];

        LwSmCommonFreeServiceInfo(pInfo);
        pInfo = NULL;

        dwError = LwSmRegistryReadServiceInfo(hReg, pwszName, &pInfo);
        switch (dwError)
        {
        case LWREG_ERROR_NO_SUCH_KEY_OR_VALUE:
            dwError = 0;
            continue;
        default:
            break;
        }
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableGetEntry(pwszName, &pEntry);
        if (!dwError)
        {
            dwError = LwSmTableUpdateEntry(pEntry, pInfo, LW_SERVICE_INFO_MASK_ALL);
            BAIL_ON_ERROR(dwError);
        }
        else if (dwError == LW_ERROR_NO_SUCH_SERVICE)
        {
            dwError = LwSmTableAddEntry(pInfo, &pEntry);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            BAIL_ON_ERROR(dwError);
        }

        LwSmTableReleaseEntry(pEntry);
        pEntry = NULL;
    }

cleanup:

    LwSmFreeStringList(ppwszNames);
    LwSmCommonFreeServiceInfo(pInfo);

    if (hReg)
    {
        RegCloseServer(hReg);
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
