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
    LWMsgContext* pIpcContext;
    LWMsgProtocol* pIpcProtocol;
    LWMsgServer* pIpcServer;
    BOOLEAN bStartAsDaemon;
    int notifyPipe[2];
    LW_SM_LOG_LEVEL logLevel;
    PCSTR pszLogFilePath;
    BOOLEAN bSyslog;
} gState = 
{
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
LwSmConfigureSignals(
    VOID
    );

static
DWORD
LwSmWaitSignals(
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
    BOOLEAN bNotified = FALSE;

    /* Parse command line */
    dwError = LwSmParseArguments(argc, ppszArgv);
    BAIL_ON_ERROR(dwError);

    /* Block signals */
    dwError = LwSmConfigureSignals();
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
        bNotified = TRUE;
    }

    /* Loop waiting for signals */
    dwError = LwSmWaitSignals();
    BAIL_ON_ERROR(dwError);

    /* Stop IPC server */
    dwError = LwSmStopIpcServer();
    BAIL_ON_ERROR(dwError);

    /* Shut down all running services as we exit */
    dwError = LwSmShutdownServices();
    BAIL_ON_ERROR(dwError);

    /* Shut down logging */
    dwError = LwSmSetLogger(NULL, NULL);
    BAIL_ON_ERROR(dwError);

error:

    /* If we are starting as a daemon and have not
       notified the parent process yet, notify it
       of an error now */
    if (gState.bStartAsDaemon && !bNotified)
    {
        LwSmNotify(1);
    }

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
        else if (!strcmp(ppszArgv[i], "--log-level"))
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
        else if (!strcmp(ppszArgv[i], "--log-file"))
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
LwSmHandleSigint(
    int sig
    )
{
    raise(SIGTERM);
}

static
VOID
LwSmHandleSig(
    int sig
    )
{
    return;
}

static
DWORD
LwSmConfigureSignals(
    VOID
    )
{
    DWORD dwError = 0;
    sigset_t set;
    static int blockSignals[] =
    {
        SIGTERM,
        SIGCHLD,
        SIGHUP,
        SIGPIPE,
        -1
    };
    int i = 0;
    struct sigaction action;

    memset(&action, 0, sizeof(action));

    if (sigemptyset(&set) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    for (i = 0; blockSignals[i] != -1; i++)
    {
        if (sigaddset(&set, blockSignals[i]) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError); 
        }

	action.sa_handler = LwSmHandleSig;
	action.sa_flags = 0;

	if (sigaction(blockSignals[i], &action, NULL) < 0)
        {
             dwError = LwMapErrnoToLwError(errno);
             BAIL_ON_ERROR(dwError);
        }
    }

    dwError = LwMapErrnoToLwError(pthread_sigmask(SIG_SETMASK, &set, NULL));
    BAIL_ON_ERROR(dwError); 

    action.sa_handler = LwSmHandleSigint;
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmWaitSignals(
    VOID
    )
{
    DWORD dwError = 0;
    sigset_t set;
    static int waitSignals[] =
    {
        SIGTERM,
        SIGHUP,
        -1
    };
    int sig = -1;
    int i = 0;

    if (sigemptyset(&set) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    for (i = 0; waitSignals[i] != -1; i++)
    {
        if (sigaddset(&set, waitSignals[i]) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError); 
        }
    }

    for (;;)
    {
        if (sigwait(&set, &sig) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        switch (sig)
        {
        case SIGTERM:
            SM_LOG_ALWAYS("Shutting down on SIGTERM");
            goto cleanup;
        case SIGHUP:
            dwError = LwSmPopulateTable();
            BAIL_ON_ERROR(dwError);
            break;
        default:
            break;
        }
    }

cleanup:

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
                __FUNCTION__,
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

        LwSmCommonFreeServiceInfo(pInfo);
        pInfo = NULL;
    }

cleanup:

    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
        pInfo = NULL;
    }

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
        BAIL_ON_ERROR(dwError);

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
