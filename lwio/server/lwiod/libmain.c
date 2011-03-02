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
 *        Likewise Security and Authentication Subsystem (SMBSS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"
#include "ioinit.h"
#include "ioipc.h"

static
NTSTATUS
LwioSrvSetDefaults(
    PLWIO_CONFIG pConfig
    );

static
DWORD
LwioSrvParseArgs(
    int argc,
    PSTR argv[],
    PSMBSERVERINFO pSMBServerInfo
    );

static
PSTR
SMBGetProgramName(
    PSTR pszFullProgramPath
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

static
DWORD
SMBSrvInitialize(
    VOID
    );

static
DWORD
SMBInitCacheFolders(
    VOID
    );

static
BOOLEAN
SMBSrvShouldStartAsDaemon(
    VOID
    );

static
DWORD
SMBSrvStartAsDaemon(
    VOID
    );

static
DWORD
SMBSrvExecute(
    VOID
    );

static
DWORD
SMBHandleSignals(
    VOID
    );

static
VOID
SMBSrvInterruptHandler(
    int sig
    );

static
VOID
SMBSrvInterruptDummyHandler(
    int sig
    );

static
VOID
SMBSrvBlockSignals(
    VOID
    );

static
int
SMBSrvGetNextSignal(
    VOID
    );

static
VOID
SMBSrvGetBlockedSignals(
    sigset_t* pBlockedSignals
    );

static
VOID
SMBSrvGetBlockedSignalsSansInterrupt(
    sigset_t* pBlockedSignals
    );

#ifdef ENABLE_STATIC_DRIVERS

extern NTSTATUS IO_DRIVER_ENTRY(rdr)(IO_DRIVER_HANDLE, ULONG);
extern NTSTATUS IO_DRIVER_ENTRY(srv)(IO_DRIVER_HANDLE, ULONG);
extern NTSTATUS IO_DRIVER_ENTRY(npfs)(IO_DRIVER_HANDLE, ULONG);
extern NTSTATUS IO_DRIVER_ENTRY(pvfs)(IO_DRIVER_HANDLE, ULONG);

static IO_STATIC_DRIVER gStaticDrivers[] =
{
#ifdef ENABLE_RDR
    IO_STATIC_DRIVER_ENTRY(rdr),
#endif
#ifdef ENABLE_SRV
    IO_STATIC_DRIVER_ENTRY(srv),
#endif
#ifdef ENABLE_PVFS
    IO_STATIC_DRIVER_ENTRY(pvfs),
#endif
#ifdef ENABLE_NPFS
    IO_STATIC_DRIVER_ENTRY(npfs),
#endif
#ifdef ENABLE_DFS
    IO_STATIC_DRIVER_ENTRY(dfs),
#endif

    IO_STATIC_DRIVER_END
};

#endif

static
VOID
LwIoInitRtlLogging(
    VOID
    );

int
lwiod_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwioSrvInitializeConfig(&gLwioServerConfig);
    dwError = LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_LWIO_ERROR(dwError);

    pthread_mutex_init(&gLwioServerInfo.lock, NULL);
    gLwioServerInfo.pLock = &gLwioServerInfo.lock;

    dwError = LwioSrvParseArgs(argc,
                              argv,
                              &gLwioServerInfo);
    BAIL_ON_LWIO_ERROR(dwError);

    LwIoInitRtlLogging();

    dwError = LwioInitLogging_r(
                    SMBGetProgramName(argv[0]),
                    gLwioServerInfo.logTarget,
                    gLwioServerInfo.maxAllowedLogLevel,
                    gLwioServerInfo.szLogFilePath);
    BAIL_ON_LWIO_ERROR(dwError);

    LWIO_LOG_VERBOSE("Logging started");

    if (SMBSrvShouldStartAsDaemon()) {
       dwError = SMBSrvStartAsDaemon();
       BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    BAIL_ON_LWIO_ERROR(dwError);

    SMBSrvBlockSignals();

    dwError = LwDsCacheAddPidException(getpid());
    if (dwError == LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK)
    {
        LWIO_LOG_ERROR("Could not register process pid (%d) with Mac DirectoryService Cache plugin", (int) getpid());
        BAIL_ON_LWIO_ERROR(dwError);
    }

    ntStatus = LwioSrvRefreshConfig(&gLwioServerConfig);
    dwError = LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_LWIO_ERROR(dwError);

    ntStatus = LwioSrvSetDefaults(&gLwioServerConfig);
    dwError = LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBSrvInitialize();
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBSrvExecute();
    BAIL_ON_LWIO_ERROR(dwError);

cleanup:

    LWIO_LOG_VERBOSE("LWIO main cleaning up");

    IoCleanup();

    LWIO_LOG_INFO("LWIO Service exiting...");

    LwDsCacheRemovePidException(getpid());

    LwioShutdownLogging_r();

    if (gLwioServerInfo.pLock)
    {
        pthread_mutex_destroy(&gLwioServerInfo.lock);
        gLwioServerInfo.pLock = NULL;
    }

    return dwError;

error:

    LWIO_LOG_ERROR("LWIO Process exiting due to error [Code:%d]", dwError);

    goto cleanup;
}

static
VOID
LwIoLogCallback(
    LW_IN LW_OPTIONAL LW_PVOID Context,
    LW_IN LW_RTL_LOG_LEVEL Level,
    LW_IN LW_OPTIONAL LW_PCSTR ComponentName,
    LW_IN LW_PCSTR FunctionName,
    LW_IN LW_PCSTR FileName,
    LW_IN LW_ULONG LineNumber,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    va_list ap;
    PSTR pMessage = NULL;

    va_start(ap, Format);
    status = LwRtlCStringAllocatePrintfV(&pMessage, Format, ap);
    va_end(ap);
    BAIL_ON_NT_STATUS(status);

    if (LwRtlLogGetLevel() >= LWIO_LOG_LEVEL_DEBUG)
    {
        LwioLogMessage(
            gpfnLwioLogger,
            ghLwioLog,
            Level,
            "0x%lx: [%s %s:%d] %s",
            (unsigned long) pthread_self(),
            FunctionName,
            FileName,
            LineNumber,
            pMessage);
    }
    else
    {
        LwioLogMessage(
            gpfnLwioLogger,
            ghLwioLog,
            Level,
            "0x%lx: %s",
            (unsigned long) pthread_self(),
            pMessage);
    }

error:

    RTL_FREE(&pMessage);

    return;
}

static
VOID
LwIoInitRtlLogging(
    VOID
    )
{
    LwRtlLogSetCallback(LwIoLogCallback, NULL);
}

static
NTSTATUS
LwioSrvSetDefaults(
    PLWIO_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWIO_CONFIG defaultConfig;

    gpLwioServerInfo->maxAllowedLogLevel = LWIO_LOG_LEVEL_ERROR;

    *(gpLwioServerInfo->szLogFilePath) = '\0';

    strncpy(gpLwioServerInfo->szCachePath, CACHEDIR, PATH_MAX);
    gpLwioServerInfo->szCachePath[PATH_MAX] = '\0';

    strncpy(gpLwioServerInfo->szPrefixPath, PREFIXDIR, PATH_MAX);
    gpLwioServerInfo->szPrefixPath[PATH_MAX] = '\0';

    setlocale(LC_ALL, "");

    // Enforce configuration settings

    ntStatus = LwioSrvInitializeConfig(&defaultConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LwioSrvFreeConfigContents(&defaultConfig);

    return ntStatus;

error:

    goto cleanup;    
}

static
DWORD
LwioSrvParseArgs(
    int argc,
    PSTR argv[],
    PSMBSERVERINFO pSMBServerInfo
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    BOOLEAN bLogTargetSet = FALSE;

    do {

      pArg = argv[iArg++];
      if (pArg == NULL || *pArg == '\0') {
        break;
      }

      switch(parseMode) {

      case PARSE_MODE_OPEN:

        {
          if (strcmp(pArg, "--logfile") == 0)    {
            parseMode = PARSE_MODE_LOGFILE;
          }
          else if ((strcmp(pArg, "--help") == 0) ||
                   (strcmp(pArg, "-h") == 0)) {
            ShowUsage(SMBGetProgramName(argv[0]));
            exit(0);
          }
          else if (strcmp(pArg, "--start-as-daemon") == 0) {
            pSMBServerInfo->dwStartAsDaemon = 1;

            // If other arguments set before this set the log target
            // don't over-ride that setting
            if (!bLogTargetSet)
            {
                pSMBServerInfo->logTarget = LWIO_LOG_TARGET_SYSLOG;
            }
          }
          else if (strcmp(pArg, "--syslog") == 0)
          {
            bLogTargetSet = TRUE;
            pSMBServerInfo->logTarget = LWIO_LOG_TARGET_SYSLOG;
          }
          else if (strcmp(pArg, "--loglevel") == 0) {
            parseMode = PARSE_MODE_LOGLEVEL;
          } else {
            LWIO_LOG_ERROR("Unrecognized command line option [%s]",
                          pArg);
            ShowUsage(SMBGetProgramName(argv[0]));
            exit(1);
          }

          break;
        }

      case PARSE_MODE_LOGFILE:

        {
          strncpy(pSMBServerInfo->szLogFilePath, pArg, PATH_MAX);
          pSMBServerInfo->szLogFilePath[PATH_MAX] = '\0';

          SMBStripWhitespace(pSMBServerInfo->szLogFilePath, TRUE, TRUE);

          if (!strcmp(pSMBServerInfo->szLogFilePath, "."))
          {
              pSMBServerInfo->logTarget = LWIO_LOG_TARGET_CONSOLE;
          }
          else
          {
              pSMBServerInfo->logTarget = LWIO_LOG_TARGET_FILE;
          }

          bLogTargetSet = TRUE;

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      case PARSE_MODE_LOGLEVEL:

        {
          if (!strcasecmp(pArg, "error")) {

            pSMBServerInfo->maxAllowedLogLevel = LWIO_LOG_LEVEL_ERROR;

          } else if (!strcasecmp(pArg, "warning")) {

            pSMBServerInfo->maxAllowedLogLevel = LWIO_LOG_LEVEL_WARNING;

          } else if (!strcasecmp(pArg, "info")) {

            pSMBServerInfo->maxAllowedLogLevel = LWIO_LOG_LEVEL_INFO;

          } else if (!strcasecmp(pArg, "verbose")) {

            pSMBServerInfo->maxAllowedLogLevel = LWIO_LOG_LEVEL_VERBOSE;

          } else if (!strcasecmp(pArg, "debug")) {

            pSMBServerInfo->maxAllowedLogLevel = LWIO_LOG_LEVEL_DEBUG;

          } else if (!strcasecmp(pArg, "trace")) {

            pSMBServerInfo->maxAllowedLogLevel = LWIO_LOG_LEVEL_TRACE;

          } else {

            LWIO_LOG_ERROR("Error: Invalid log level [%s]", pArg);
            ShowUsage(SMBGetProgramName(argv[0]));
            exit(1);

          }

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      }

    } while (iArg < argc);

    if (pSMBServerInfo->dwStartAsDaemon)
    {
        if (pSMBServerInfo->logTarget == LWIO_LOG_TARGET_CONSOLE)
        {
            LWIO_LOG_ERROR("%s", "Error: Cannot log to console when executing as a daemon");

            dwError = LWIO_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWIO_ERROR(dwError);
        }
    }
    else
    {
        if (!bLogTargetSet)
        {
            pSMBServerInfo->logTarget = LWIO_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}

static
PSTR
SMBGetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

static
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
           "          [--logfile logFilePath]\n"
           "          [--syslog]\n"
           "          [--loglevel {error, warning, info, verbose, debug, trace}]\n", pszProgramName);
}

static
DWORD
SMBSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = SMBInitCacheFolders();
    BAIL_ON_LWIO_ERROR(dwError);

#ifdef ENABLE_STATIC_DRIVERS
    dwError = IoInitialize(gStaticDrivers);
#else
    dwError = IoInitialize(NULL);
#endif
    BAIL_ON_LWIO_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
SMBInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = SMBCheckDirectoryExists(
                        CACHEDIR,
                        &bExists);
    BAIL_ON_LWIO_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = SMBCreateDirectory(CACHEDIR, cacheDirMode);
        BAIL_ON_LWIO_ERROR(dwError);
    }

error:

    return dwError;
}

static
BOOLEAN
SMBSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_SERVERINFO(bInLock);

    bResult = (gpLwioServerInfo->dwStartAsDaemon != 0);

    LWIO_UNLOCK_SERVERINFO(bInLock);

    return bResult;
}

static
DWORD
SMBSrvStartAsDaemon(
    VOID
    )
{
    DWORD dwError = 0;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    if ((pid = fork()) != 0) {
        // Parent terminates
        exit (0);
    }

    // Let the first child be a session leader
    setsid();

    // Ignore SIGHUP, because when the first child terminates
    // it would be a session leader, and thus all processes in
    // its session would receive the SIGHUP signal. By ignoring
    // this signal, we are ensuring that our second child will
    // ignore this signal and will continue execution.
    if (signal(SIGHUP, SIG_IGN) < 0) {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_LWIO_ERROR(dwError);

    // Clear our file mode creation mask
    umask(0);

    for (iFd = 0; iFd < 3; iFd++)
        close(iFd);

    for (iFd = 0; iFd < 3; iFd++)    {

        fd = open("/dev/null", O_RDWR, 0);
        if (fd < 0) {
            fd = open("/dev/null", O_WRONLY, 0);
        }
        if (fd < 0) {
            exit(1);
        }
        if (fd != iFd) {
            exit(1);
        }
    }

    return (dwError);

 error:

    return (dwError);
}

LWMsgBool
LwIoDaemonLogIpc (
    LWMsgLogLevel level,
    const char* pszMessage,
    const char* pszFunction,
    const char* pszFilename,
    unsigned int line,
    void* pData
    )
{
    LWIO_LOG_LEVEL ioLevel = LWIO_LOG_LEVEL_DEBUG;
    LWMsgBool result = LWMSG_FALSE;

    switch (level)
    {
    case LWMSG_LOGLEVEL_ALWAYS:
        ioLevel = LWIO_LOG_LEVEL_ALWAYS;
        break;
    case LWMSG_LOGLEVEL_ERROR:
        ioLevel = LWIO_LOG_LEVEL_ERROR;
        break;
    case LWMSG_LOGLEVEL_WARNING:
        ioLevel = LWIO_LOG_LEVEL_WARNING;
        break;
    case LWMSG_LOGLEVEL_INFO:
        ioLevel = LWIO_LOG_LEVEL_INFO;
        break;
    case LWMSG_LOGLEVEL_VERBOSE:
        ioLevel = LWIO_LOG_LEVEL_VERBOSE;
        break;
    case LWMSG_LOGLEVEL_DEBUG:
        ioLevel = LWIO_LOG_LEVEL_DEBUG;
        break;
    case LWMSG_LOGLEVEL_TRACE:
        ioLevel = LWIO_LOG_LEVEL_TRACE;
        break;
    }
    
    result = LwRtlLogGetLevel() >= ioLevel;

    if (pszMessage && result)
    {
        LW_RTL_LOG_RAW(ioLevel, "lwio-ipc", pszFunction, pszFilename, line, "%s", pszMessage);
    }

    return result;
}

static
DWORD
SMBSrvExecute(
    VOID
    )
{
    DWORD dwError = 0;
    LWMsgContext* pContext = NULL;
    LWMsgProtocol* pProtocol = NULL;
    LWMsgPeer* pServer = NULL;
    LWMsgTime timeout = { 30, 0 }; /* 30 seconds */
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;
    int ret = 0;

    dwError = MAP_LWMSG_STATUS(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LWIO_ERROR(dwError);

    lwmsg_context_set_log_function(pContext, LwIoDaemonLogIpc, NULL);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(pContext, &pProtocol));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwIoDaemonIpcAddProtocolSpec(pProtocol);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = IoIpcAddProtocolSpec(pProtocol);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_new(pContext, pProtocol, &pServer));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwIoDaemonIpcAddDispatch(pServer);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = IoIpcAddDispatch(pServer);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_listen_endpoint(
                    pServer,
                    LWMSG_ENDPOINT_LOCAL,
                    LWIO_SERVER_FILENAME,
                    (S_IRWXU | S_IRWXG | S_IRWXO)));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_set_max_listen_clients(
                    pServer,
                    512));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_set_timeout(
                    pServer,
                    LWMSG_TIMEOUT_IDLE,
                    &timeout));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_start_listen(pServer));
    BAIL_ON_LWIO_ERROR(dwError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);
        
        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            LWIO_LOG_ERROR("Could not notify service manager: %s (%i)", strerror(errno), errno);
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LWIO_ERROR(dwError);
        }

        close(notifyFd);
    }

    dwError = SMBHandleSignals();
    BAIL_ON_LWIO_ERROR(dwError);

cleanup:

    LWIO_LOG_VERBOSE("Exiting with dwError = %d", dwError);

    if (pServer)
    {
        LWMsgStatus status2 = lwmsg_peer_stop_listen(pServer);

        if (status2)
        {
            LWIO_LOG_ERROR("Error stopping server. [Error code:%d]", status2);
        }

        lwmsg_peer_delete(pServer);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    return dwError;

error:

    LWIO_LOG_ERROR("SMB Server stopping due to error [code: %d]", dwError);

    goto cleanup;
}

static
DWORD
SMBHandleSignals(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bDone = FALSE;
    struct sigaction action;
    sigset_t catch_signal_mask;

    // After starting up threads, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = SMBSrvInterruptHandler;

    if (sigaction(SIGINT, &action, NULL) != 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    // Register dummy handlers for the rest of the signals
    // caught by sigwait();
    memset(&action, 0, sizeof(action));
    action.sa_handler = SMBSrvInterruptDummyHandler;
    if (sigaction(SIGTERM, &action, NULL) != 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }
    if (sigaction(SIGPIPE, &action, NULL) != 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }
    if (sigaction(SIGHUP, &action, NULL) != 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    dwError = pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL);
    BAIL_ON_LWIO_ERROR(dwError);

    while (!bDone)
    {
        switch (SMBSrvGetNextSignal())
        {
            case SIGINT:
            case SIGTERM:

                bDone = TRUE;
                break;

            case SIGHUP:

                {
                    NTSTATUS ntStatus = STATUS_SUCCESS;

                    ntStatus = LwioSrvRefreshConfig(&gLwioServerConfig);
                    if (ntStatus)
                    {
                        LWIO_LOG_ERROR("Failed to refresh configuration "
                            "[code:%d]", ntStatus);
                    }
                }

                break;

            default:

                break;
        }
    }

error:

    return dwError;
}

static
VOID
SMBSrvInterruptHandler(
    int sig
    )
{
    if (sig == SIGINT)
    {
        raise(SIGTERM);
    }
}


static
VOID
SMBSrvInterruptDummyHandler(
    int sig
    )
{
    return;
}


static
VOID
SMBSrvBlockSignals(
    VOID
    )
{
    sigset_t blockedSignals;

    SMBSrvGetBlockedSignals(&blockedSignals);

    pthread_sigmask(SIG_BLOCK, &blockedSignals, NULL);
}

static
int
SMBSrvGetNextSignal(
    VOID
    )
{
    sigset_t blockedSignals;
    int sig = 0;

    SMBSrvGetBlockedSignalsSansInterrupt(&blockedSignals);

    sigwait(&blockedSignals, &sig);

    return sig;
}

static
VOID
SMBSrvGetBlockedSignals(
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
SMBSrvGetBlockedSignalsSansInterrupt(
    sigset_t* pBlockedSignals
    )
{
    sigemptyset(pBlockedSignals);
    sigaddset(pBlockedSignals, SIGTERM);
    sigaddset(pBlockedSignals, SIGPIPE);
    sigaddset(pBlockedSignals, SIGHUP);
}

BOOLEAN
SMBSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_SERVERINFO(bInLock);

    bExit = gpLwioServerInfo->bProcessShouldExit;

    LWIO_UNLOCK_SERVERINFO(bInLock);

    return bExit;
}

VOID
SMBSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_SERVERINFO(bInLock);

    gpLwioServerInfo->bProcessShouldExit = bExit;

    LWIO_UNLOCK_SERVERINFO(bInLock);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
