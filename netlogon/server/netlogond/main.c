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
 *        Likewise Site Manager
 * 
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

static
VOID
LWNetInitRtlLogging(
    VOID
    );

int
main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError = 0;
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;
    int ret = 0;

    dwError = LWNetSrvSetDefaults();
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvParseArgs(argc, argv, &gServerInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    LWNetInitRtlLogging();

    dwError = LWNetSrvInitLogging(LWNetGetProgramName(argv[0]));
    BAIL_ON_LWNET_ERROR(dwError);

    LWNET_LOG_VERBOSE("Logging started");

    if (LWNetSrvShouldStartAsDaemon())
    {
       dwError = LWNetSrvStartAsDaemon();
       BAIL_ON_LWNET_ERROR(dwError);
    }

    if (atexit(LWNetSrvExitHandler) < 0)
    {
       dwError = LwMapErrnoToLwError(errno);
       BAIL_ON_LWNET_ERROR(dwError);
    }

    // Test system to see if dependent configuration tasks are completed prior to starting our process.
    dwError = LWNetStartupPreCheck();
    BAIL_ON_LWNET_ERROR(dwError);

#ifdef ENABLE_PIDFILE
    // ISSUE-2008/07/03-dalmeida -- Should return/check for errors
    LWNetSrvCreatePIDFile();
#endif

    dwError = LWNetBlockSelectedSignals();
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwDsCacheAddPidException(getpid());
    if (dwError == LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK)
    {
        LWNET_LOG_ERROR("Could not register process pid (%d) with Mac DirectoryService Cache plugin", (int) getpid());
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetSrvInitialize();
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvStartListenThread();
    BAIL_ON_LWNET_ERROR(dwError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);

        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            LWNET_LOG_ERROR("Could not notify service manager: %s (%i)", strerror(errno), errno);
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LWNET_ERROR(dwError);
        }

        close(notifyFd);
    }

    // Post service started event to eventlog
    LWNetSrvLogProcessStartedEvent();

    // Handle signals, blocking until we are supposed to exit.
    dwError = LWNetSrvHandleSignals();
    BAIL_ON_LWNET_ERROR(dwError);

 cleanup:

    LWNET_LOG_VERBOSE("LWNet main cleaning up");

#ifdef ENABLE_PIDFILE
    // On successful exit, remove the pid file. Ignore errors
    unlink(PID_FILE);
#endif

    // Post service stopped event to eventlog
    LWNetSrvLogProcessStoppedEvent(dwError);

    LWNetSrvStopListenThread();

    LwDsCacheRemovePidException(getpid());

    LWNetSrvApiShutdown();

    LWNET_LOG_INFO("LWNET Service exiting...");

    lwnet_close_log();

    LWNetSrvSetProcessExitCode(dwError);

    return dwError;

 error:

    LWNET_LOG_ERROR("LWNET Process exiting due to error [Code:%d]", dwError);

    // Post service failed event to eventlog
    LWNetSrvLogProcessFailureEvent(dwError);

    goto cleanup;
}

static
VOID
LWNetLogCallback(
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
    DWORD dwError = 0;
    va_list ap;
    PSTR pMessage = NULL;

    va_start(ap, Format);
    dwError = LwNtStatusToWin32Error(
        LwRtlCStringAllocatePrintfV(&pMessage, Format, ap));
    va_end(ap);
    BAIL_ON_LWNET_ERROR(dwError);

    if (LwRtlLogGetLevel() >= LWNET_LOG_LEVEL_DEBUG)
    {
        lwnet_log_message(
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
        lwnet_log_message(
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
LWNetInitRtlLogging(
    VOID
    )
{
    LwRtlLogSetCallback(LWNetLogCallback, NULL);
}

DWORD
LWNetStartupPreCheck(
    VOID
    )
{
    DWORD dwError = 0;
#ifdef __LWI_DARWIN__
    PSTR  pszHostname = NULL;
    int  iter = 0;

    // Make sure that the local hostname has been setup by the system
    for (iter = 0; iter < STARTUP_PRE_CHECK_WAIT; iter++)
    {
        LWNET_SAFE_FREE_STRING(pszHostname);
        dwError = LWNetDnsGetHostInfo(&pszHostname, NULL);
        BAIL_ON_LWNET_ERROR(dwError);

        if (!strcasecmp(pszHostname, "localhost"))
        {
            sleep(10);
        }
        else
        {
            /* Hostname now looks correct */
            LWNET_LOG_INFO("LWNet Process start up check completed [Hostname = %s]", pszHostname);
            break;
        }
    }

    if (iter >= STARTUP_PRE_CHECK_WAIT)
    {
        dwError = ERROR_SERVICE_DEPENDENCY_FAIL;
        LWNET_LOG_ERROR("LWNet start up pre-check failed to get updated hostname after 2 minutes of waiting [Code:%d]", dwError);
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    LWNET_SAFE_FREE_STRING(pszHostname);

    return dwError;

error:

    LWNET_LOG_ERROR("LWNet Process exiting due to error checking hostname at startup [Code:%d]", dwError);

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LWNetSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    gpServerInfo->dwLogLevel = LWNET_LOG_LEVEL_ERROR;

    *(gpServerInfo->szLogFilePath) = '\0';

    strcpy(gpServerInfo->szCachePath, LWNET_CACHE_DIR);
    strcpy(gpServerInfo->szPrefixPath, PREFIXDIR);

    setlocale(LC_ALL, "");

    return (dwError);
}

DWORD
LWNetSrvParseArgs(
    int argc,
    PCSTR argv[],
    PLWNETSERVERINFO pLWNetServerInfo
    )
{
    int iArg = 0;
    BOOLEAN bShowUsage = FALSE;
    BOOLEAN bError = FALSE;

    for (iArg = 1; iArg < argc; iArg++)
    {
        PCSTR pArg = argv[iArg];

        if (!strcmp(pArg, "--help") ||
            !strcmp(pArg, "-h"))
        {
            bShowUsage = TRUE;
            break;
        }
        else if (!strcmp(pArg, "--start-as-daemon"))
        {
            pLWNetServerInfo->dwStartAsDaemon = 1;
        }
        else if (!strcmp(pArg, "--logfile"))
        {
            if (iArg + 1 >= argc)
            {
                fprintf(stderr, "Missing required argument for %s option.\n", pArg);
                bError = TRUE;
                break;
            }
            // ISSUE-2008/07/03-dalmeida -- not safe
            pArg = argv[++iArg];
            strcpy(pLWNetServerInfo->szLogFilePath, pArg);
        }
        else if (strcmp(pArg, "--syslog") == 0)
        {
            pLWNetServerInfo->bLogToSyslog = TRUE;
        }
        else if (strcmp(pArg, "--loglevel") == 0)
        {
            if (iArg + 1 >= argc)
            {
                fprintf(stderr, "Missing required argument for %s option.\n", pArg);
                bError = TRUE;
                break;
            }
            pArg = argv[++iArg];
            if (!strcasecmp(pArg, "error"))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_ERROR;
            }
            else if (!strcasecmp(pArg, "warning"))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_WARNING;
            }
            else if (!strcasecmp(pArg, "info"))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_INFO;
            }
            else if (!strcasecmp(pArg, "verbose"))
            {
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_VERBOSE;
            }
            else if (!strcasecmp(pArg, "debug"))
            {
                pLWNetServerInfo->bEnableDebugLogs = TRUE;
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_DEBUG;
            }
            else if (!strcasecmp(pArg, "trace"))
            {
                pLWNetServerInfo->bEnableDebugLogs = TRUE;
                pLWNetServerInfo->dwLogLevel = LWNET_LOG_LEVEL_TRACE;
            }
            else
            {
                fprintf(stderr, "Invalid log level specified: '%s'.\n", pArg);
                bError = TRUE;
                break;
            }
        }
        else
        {
            fprintf(stderr, "Unrecognized command line option: '%s'.\n", pArg);
            bError = TRUE;
            break;
        }
    }

    if (bShowUsage || bError)
    {
        ShowUsage(LWNetGetProgramName(argv[0]));
        exit(bError ? 1 : 0);
    }

    return 0;
}

PCSTR
LWNetGetProgramName(
    PCSTR pszFullProgramPath
    )
{
    PCSTR pszNameStart = NULL;

    if (IsNullOrEmptyString(pszFullProgramPath))
    {
        return "<UNKNOWN>";
    }

    // start from end of the string
    pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [options]\n"
           "\n"
           "  Options:\n"
           "\n"
           "    --start-as-daemon         start in daemon mode\n"
           "    --syslog                  log to syslog\n"
           "    --logfile <logFilePath>   log to specified file\n"
           "    --loglevel <logLevel>     log at the specified detail level, which\n"
           "                              can be one of:\n"
           "                                error, warning, info, verbose, debug.\n"
           "", pszProgramName);
}

VOID
LWNetSrvExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = LWNetSrvGetCachePath(&pszCachePath);
    BAIL_ON_LWNET_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/LWNetsd.err", pszCachePath);

    dwError = LwCheckFileTypeExists(
                    szErrCodeFilePath,
                    LWFILE_REGULAR,
                    &bFileExists);
    BAIL_ON_LWNET_ERROR(dwError);

    if (bFileExists) {
        dwError = LwRemoveFile(szErrCodeFilePath);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetSrvGetProcessExitCode(&dwExitCode);
    BAIL_ON_LWNET_ERROR(dwError);

    if (dwExitCode) {
       fp = fopen(szErrCodeFilePath, "w");
       if (fp == NULL) {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LWNET_ERROR(dwError);
       }
       fprintf(fp, "%d\n", dwExitCode);
    }

error:

    LWNET_SAFE_FREE_STRING(pszCachePath);

    if (fp != NULL) {
       fclose(fp);
    }
}

DWORD
LWNetSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    
    dwError = LWNetSrvApiInit();
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

BOOLEAN
LWNetSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LWNET_LOCK_SERVERINFO(bInLock);

    bResult = (gpServerInfo->dwStartAsDaemon != 0);

    LWNET_UNLOCK_SERVERINFO(bInLock);

    return bResult;
}

DWORD
LWNetSrvStartAsDaemon(
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
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LWNET_ERROR(dwError);
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
    BAIL_ON_LWNET_ERROR(dwError);

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

DWORD
LWNetSrvGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LWNET_LOCK_SERVERINFO(bInLock);

    *pdwExitCode = gpServerInfo->dwExitCode;

    LWNET_UNLOCK_SERVERINFO(bInLock);

    return dwError;
}

VOID
LWNetSrvSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;
    
    LWNET_LOCK_SERVERINFO(bInLock);

    gpServerInfo->dwExitCode = dwExitCode;

    LWNET_UNLOCK_SERVERINFO(bInLock);
}

DWORD
LWNetSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;
  
    LWNET_LOCK_SERVERINFO(bInLock);
    
    if (IsNullOrEmptyString(gpServerInfo->szCachePath)) {
      dwError = ERROR_PATH_NOT_FOUND;
      BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateString(gpServerInfo->szCachePath, &pszPath);
    BAIL_ON_LWNET_ERROR(dwError);

    *ppszPath = pszPath;
    
 cleanup:

    LWNET_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    LWNET_SAFE_FREE_STRING(pszPath);
    
    *ppszPath = NULL;

    goto cleanup;
}

DWORD
LWNetSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;
  
    LWNET_LOCK_SERVERINFO(bInLock);
    
    if (IsNullOrEmptyString(gpServerInfo->szPrefixPath)) {
      dwError = ERROR_PATH_NOT_FOUND;
      BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateString(gpServerInfo->szPrefixPath, &pszPath);
    BAIL_ON_LWNET_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:
    
    LWNET_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    LWNET_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

VOID
LWNetSrvCreatePIDFile(
    VOID
    )
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = LWNetSrvGetPidFromPidFile();
    if (pid > 0) {
        fprintf(stderr, "Daemon already running as %d\n", (int) pid);
        result = -1;
        goto error;
    }

    fd = open(PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0) {
        fprintf(stderr, "Could not create pid file: %s\n", strerror(errno));
        result = 1;
        goto error;
    }

    pid = getpid();
    snprintf(contents, sizeof(contents)-1, "%d\n", (int) pid);
    contents[sizeof(contents)-1] = 0;
    len = strlen(contents);

    result = (int) write(fd, contents, len);
    if ( result != (int) len ) {
        fprintf(stderr, "Could not write to pid file: %s\n", strerror(errno));
        result = -1;
        goto error;
    }

    result = 0;

 error:
    if (fd != -1) {
        close(fd);
    }

    if (result < 0) {
        exit(1);
    }
}

pid_t
LWNetSrvGetPidFromPidFile(
    VOID
    )
{
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[PID_FILE_CONTENTS_SIZE];

    fd = open(PID_FILE, O_RDONLY, 0644);
    if (fd < 0) {
        goto error;
    }

    result = read(fd, contents, sizeof(contents)-1);
    if (result <= 0) {
        goto error;
    }
    contents[result-1] = 0;

    result = atoi(contents);
    if (result <= 0) {
        result = -1;
        goto error;
    }

    pid = (pid_t) result;
    result = kill(pid, 0);
    if (result != 0 || errno == ESRCH) {
        unlink(PID_FILE);
        pid = 0;
    }

 error:
    if (fd != -1) {
        close(fd);
    }

    return pid;
}

DWORD
LWNetSrvInitLogging(
    PCSTR pszProgramName
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LWNET_LOCK_SERVERINFO(bInLock);

    if ((gpServerInfo->dwStartAsDaemon &&
            gpServerInfo->szLogFilePath[0] == '\0') ||
            gpServerInfo->bLogToSyslog)
    {
      
      dwError = lwnet_init_logging_to_syslog(gpServerInfo->dwLogLevel,
                                           gpServerInfo->bEnableDebugLogs,
                                           pszProgramName,
                                           LOG_PID,
                                           LOG_DAEMON);
      BAIL_ON_LWNET_ERROR(dwError);
      
    } else {
      
      dwError = lwnet_init_logging_to_file(gpServerInfo->dwLogLevel,
                                         gpServerInfo->bEnableDebugLogs,
                                         gpServerInfo->szLogFilePath);
      BAIL_ON_LWNET_ERROR(dwError);
      
    }

 cleanup:

    LWNET_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    goto cleanup;
}

VOID
LWNetClearAllSignals(
    VOID
    )
{
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;
   
    sigemptyset(&default_signal_mask);
    pthread_sigmask(SIG_SETMASK,  &default_signal_mask, &old_signal_mask);
}

DWORD
LWNetBlockSelectedSignals(
    VOID
    )
{
    DWORD dwError = 0;
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    sigaddset(&default_signal_mask, SIGINT);
    sigaddset(&default_signal_mask, SIGTERM);
    sigaddset(&default_signal_mask, SIGHUP);
    sigaddset(&default_signal_mask, SIGQUIT);
    sigaddset(&default_signal_mask, SIGPIPE);

    dwError = pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

BOOLEAN
LWNetSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    LWNET_LOCK_SERVERINFO(bInLock);
    
    bExit = gpServerInfo->bProcessShouldExit;

    LWNET_UNLOCK_SERVERINFO(bInLock);
    
    return bExit;
}

VOID
LWNetSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;
    
    LWNET_LOCK_SERVERINFO(bInLock);

    gpServerInfo->bProcessShouldExit = bExit;

    LWNET_UNLOCK_SERVERINFO(bInLock);
}

VOID
LWNetSrvLogProcessStartedEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise site manager service was started.");
    BAIL_ON_LWNET_ERROR(dwError);

    LWNetSrvLogInformationEvent(
            LWNET_EVENT_INFO_SERVICE_STARTED,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LWNET_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LWNetSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise site manager service was stopped");
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetErrorMessageForLoggingEvent(
                         dwExitCode,
                         &pszData);
    BAIL_ON_LWNET_ERROR(dwError);

    if (dwExitCode)
    {
        LWNetSrvLogErrorEvent(
                LWNET_EVENT_ERROR_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }
    else
    {
        LWNetSrvLogInformationEvent(
                LWNET_EVENT_INFO_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }

cleanup:

    LWNET_SAFE_FREE_STRING(pszDescription);
    LWNET_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

VOID
LWNetSrvLogProcessFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise site manager service stopped running due to an error");
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
    BAIL_ON_LWNET_ERROR(dwError);

    LWNetSrvLogErrorEvent(
            LWNET_EVENT_ERROR_SERVICE_START_FAILURE,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            pszData);

cleanup:

    LWNET_SAFE_FREE_STRING(pszDescription);
    LWNET_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

DWORD
LWNetGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg
    )
{
    DWORD dwErrorBufferSize = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszErrorMsg = NULL;
    PSTR  pszErrorBuffer = NULL;

    dwErrorBufferSize = LwGetErrorString(dwErrCode, NULL, 0);

    if (!dwErrorBufferSize)
        goto cleanup;

    dwError = LWNetAllocateMemory(
                dwErrorBufferSize,
                (PVOID*)&pszErrorBuffer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwLen = LwGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);

    if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
    {
        dwError = LwAllocateStringPrintf(
                     &pszErrorMsg,
                     "Error: %s [error code: %d]",
                     pszErrorBuffer,
                     dwErrCode);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:

    LWNET_SAFE_FREE_STRING(pszErrorBuffer);

    return dwError;

error:

    LWNET_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}

