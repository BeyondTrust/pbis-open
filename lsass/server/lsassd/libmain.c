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
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "config.h"
#include "lsassd.h"
#include "lwnet.h"
#include "lw/base.h"
#include "lwdscache.h"
#include "eventlog.h"
#include "lsasrvutils.h"

/* Needed for dcethread_fork() */
#include <dce/dcethread.h>

#ifdef ENABLE_STATIC_PROVIDERS
#ifdef ENABLE_AD
extern DWORD LsaInitializeProvider_ActiveDirectory(PCSTR*, PLSA_PROVIDER_FUNCTION_TABLE_2*);
#endif
#ifdef ENABLE_LOCAL
extern DWORD LsaInitializeProvider_Local(PCSTR*, PLSA_PROVIDER_FUNCTION_TABLE_2*);
#endif

static LSA_STATIC_PROVIDER gStaticProviders[] =
{
#ifdef ENABLE_AD
    { "lsa-activedirectory-provider", LsaInitializeProvider_ActiveDirectory },
#endif
#ifdef ENABLE_LOCAL
    { "lsa-local-provider", LsaInitializeProvider_Local },
#endif
    { 0 }
};
#endif // ENABLE_STATIC_PROVIDERS

#ifdef ENABLE_PIDFILE
static
VOID
LsaSrvCreatePIDFile(
    VOID
    );

static
pid_t
LsaSrvGetPidFromPidFile(
    VOID
    );

static
VOID
LsaSrvRemovePidFile(
    VOID
    );

#endif

int
lsassd_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;
    int ret = 0;

    // Register a signal handler for program crashes such that it prints out a
    // backtrace.
    dwError = LsaSrvRegisterCrashHandler();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvSetDefaults();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvParseArgs(argc,
                              argv,
                              &gServerInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaInitLogging_r(
                    LsaGetProgramName(argv[0]),
                    gServerInfo.logTarget,
                    gServerInfo.maxAllowedLogLevel,
                    gServerInfo.szLogFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_VERBOSE("Logging started");

    dwError = LsaInitTracing_r();
    BAIL_ON_LSA_ERROR(dwError);

    if (atexit(LsaSrvExitHandler) < 0) {
       dwError = LwMapErrnoToLwError(errno);
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (LsaSrvShouldStartAsDaemon()) {
       dwError = LsaSrvStartAsDaemon();
       BAIL_ON_LSA_ERROR(dwError);
    }

    // Ignore any errors returned by this function.
    LsaSrvRaiseMaxFiles(1024);

    dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBlockSelectedSignals();
    BAIL_ON_LSA_ERROR(dwError);

    // Test system to see if dependent configuration tasks are completed prior to starting our process.
    dwError = LsaSrvStartupPreCheck();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvStartEventLoggingThread();
    BAIL_ON_LSA_ERROR(dwError);

#ifdef ENABLE_PIDFILE
    LsaSrvCreatePIDFile();
#endif

    dwError = LwDsCacheAddPidException(getpid());
    if (dwError == LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK)
    {
        LSA_LOG_ERROR("Could not register process pid (%d) with Mac DirectoryService Cache plugin", (int) getpid());
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Start NTLM IPC server before we initialize providers
       because the providers might end up attempting to use
       NTLM via gss-api */
    dwError = NtlmSrvStartListenThread();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvInitialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvStartListenThread();
    BAIL_ON_LSA_ERROR(dwError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);
        
        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            LSA_LOG_ERROR("Could not notify service manager: %s (%i)",
                    strerror(errno), errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        close(notifyFd);
    }

    LsaSrvLogProcessStartedEvent();

    // Handle signals, blocking until we are supposed to exit.
    dwError = LsaSrvHandleSignals();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_LOG_VERBOSE("Lsa main cleaning up");

    LsaSrvStopProcess();

    LsaSrvStopListenThread();

    LwDsCacheRemovePidException(getpid());

    NtlmSrvStopListenThread();

    LsaSrvApiShutdown();

    LwIoShutdown();

    NtlmClientIpcShutdown();

    LSA_LOG_INFO("LSA Service exiting...");

    dwError = LsaSrvStopEventLoggingThread();

    LsaSrvSetProcessExitCode(dwError);

    LsaShutdownLogging_r();

    LsaShutdownTracing_r();

#ifdef ENABLE_PIDFILE
    LsaSrvRemovePidFile();
#endif

    return dwError;

error:

    LSA_LOG_ERROR("LSA Process exiting due to error [Code:%u]", dwError);

    LsaSrvLogProcessFailureEvent(dwError);

    goto cleanup;
}

DWORD
LsaSrvStartupPreCheck(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszHostname = NULL;
    int  iter = 0;

#ifdef __LWI_DARWIN__
    // Make sure that the local hostname has been setup by the system
    for (iter = 0; iter < STARTUP_PRE_CHECK_WAIT; iter++)
    {
        LW_SAFE_FREE_STRING(pszHostname);
        dwError = LsaDnsGetHostInfo(&pszHostname);
        BAIL_ON_LSA_ERROR(dwError);

        if (!strcasecmp(pszHostname, "localhost"))
        {
            sleep(10);
        }
        else
        {
            /* Hostname now looks correct */
            LSA_LOG_INFO("LSA Process start up check for hostname complete [hostname:%s]", pszHostname);
            break;
        }
    }

    if (iter >= STARTUP_PRE_CHECK_WAIT)
    {
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        LSA_LOG_ERROR("LSA start up pre-check failed to get updated hostname after %u seconds of waiting [Code:%u]",
                      STARTUP_PRE_CHECK_WAIT*10,
                      dwError);
        BAIL_ON_LSA_ERROR(dwError);
    }
#endif

    for (iter = 0; iter < STARTUP_NETLOGON_WAIT; iter++)
    {
        dwError = LsaSrvVerifyNetLogonStatus();

        if (dwError)
        {
            sleep(1);
        }
        else
        {
            /* NetLogon is responsive */
            LSA_LOG_INFO("LSA Process start up check for NetLogon complete");
            break;
        }
    }

    if (iter >= STARTUP_NETLOGON_WAIT)
    {
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        LSA_LOG_ERROR("LSA start up pre-check failed to be able to use NetLogonD after %u seconds of waiting [Code:%u]",
                      STARTUP_NETLOGON_WAIT,
                      dwError);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iter = 0; iter < STARTUP_LWIO_WAIT; iter++)
    {
        dwError = LsaSrvVerifyLwIoStatus();

        if (dwError)
        {
            sleep(1);
        }
        else
        {
            /* LwIo is responsive */
            LSA_LOG_INFO("LSA Process start up check for LwIo complete");
            break;
        }
    }

    if (iter >= STARTUP_LWIO_WAIT)
    {
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        LSA_LOG_ERROR("LSA start up pre-check failed to be able to use LwIoD after %u seconds of waiting [Code:%u]",
                      STARTUP_LWIO_WAIT,
                      dwError);
        BAIL_ON_LSA_ERROR(dwError);
    }

#if defined (__LWI_DARWIN_)
    // Now that we are running, we need to flush the DirectoryService process of any negative cache entries
    dwError = LsaSrvFlushSystemCache();
    BAIL_ON_LSA_ERROR(dwError);
#endif

cleanup:

    LW_SAFE_FREE_STRING(pszHostname);

    return dwError;

error:

    LSA_LOG_ERROR("LSA Process exiting due to error checking hostname at startup [Code:%u]", dwError);

    goto cleanup;
}

DWORD
LsaSrvVerifyNetLogonStatus(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;

    dwError = LWNetGetCurrentDomain(&pszDomain);
    LSA_LOG_INFO("LsaSrvVerifyNetLogonStatus call to LWNet API returned %u", dwError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
    }

    return dwError;

error:

    if (dwError == ERROR_NOT_JOINED)
    {
        dwError = 0;
    }

    goto cleanup;
}

DWORD
LsaSrvVerifyLwIoStatus(
    VOID
    )
{
    DWORD dwError = 0;
    PLWIO_LOG_INFO pLogInfo = NULL;
    PIO_CONTEXT pContext = NULL;

    dwError = LwIoOpenContext(&pContext);
    LSA_LOG_INFO("LsaSrvVerifyLwIoStatus call to LwIo API returned %u", dwError);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoGetLogInfo(
                  (HANDLE) pContext,
                  &pLogInfo);
    LSA_LOG_INFO("LsaSrvVerifyLwIoStatus call to LwIo API returned %u", dwError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pLogInfo)
    {
        LwIoFreeLogInfo(pLogInfo);
    }

    if (pContext != NULL)
    {
        LwIoCloseContext(pContext);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvRaiseMaxFiles(
    DWORD dwMaxFiles
    )
{
    DWORD dwError = 0;
    struct rlimit maxFiles = {0};
    BOOLEAN bRaised = FALSE;

    if (getrlimit(RLIMIT_NOFILE, &maxFiles) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (maxFiles.rlim_cur < dwMaxFiles)
    {
        LSA_LOG_INFO("Raising soft max fd limit from %lu to %u",
                maxFiles.rlim_cur,
                dwMaxFiles);
        maxFiles.rlim_cur = dwMaxFiles;
        bRaised = TRUE;
    }
    if (maxFiles.rlim_max < dwMaxFiles)
    {
        LSA_LOG_INFO("Raising hard max fd limit from %lu to %u",
                maxFiles.rlim_max,
                dwMaxFiles);
        maxFiles.rlim_max = dwMaxFiles;
        bRaised = TRUE;
    }
    
    if (bRaised)
    {
        if (setrlimit(RLIMIT_NOFILE, &maxFiles) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            LSA_LOG_ERROR("Raising max fd limit failed");
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    gpServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;

    *(gpServerInfo->szLogFilePath) = '\0';

    strcpy(gpServerInfo->szCachePath, CACHEDIR);
    strcpy(gpServerInfo->szPrefixPath, PREFIXDIR);

    setlocale(LC_ALL, "");

    return (dwError);
}

DWORD
LsaSrvParseArgs(
    int argc,
    PSTR argv[],
    PLSASERVERINFO pLsaServerInfo
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
            ShowUsage(LsaGetProgramName(argv[0]));
            exit(0);
          }
          else if (strcmp(pArg, "--start-as-daemon") == 0) {
            pLsaServerInfo->dwStartAsDaemon = 1;

            // If other arguments set before this set the log target
            // don't over-ride that setting
            if (!bLogTargetSet)
            {
                pLsaServerInfo->logTarget = LSA_LOG_TARGET_SYSLOG;
            }
          }
          else if (strcmp(pArg, "--syslog") == 0)
          {
            bLogTargetSet = TRUE;
            pLsaServerInfo->logTarget = LSA_LOG_TARGET_SYSLOG;
          }
          else if (strcmp(pArg, "--loglevel") == 0) {
            parseMode = PARSE_MODE_LOGLEVEL;
          } else {
            LSA_LOG_ERROR("Unrecognized command line option [%s]",
                          pArg);
            ShowUsage(LsaGetProgramName(argv[0]));
            exit(1);
          }

          break;
        }

      case PARSE_MODE_LOGFILE:

        {
          strcpy(pLsaServerInfo->szLogFilePath, pArg);

          LwStripWhitespace(pLsaServerInfo->szLogFilePath, TRUE, TRUE);

          if (!strcmp(pLsaServerInfo->szLogFilePath, "."))
          {
              pLsaServerInfo->logTarget = LSA_LOG_TARGET_CONSOLE;
          }
          else
          {
              pLsaServerInfo->logTarget = LSA_LOG_TARGET_FILE;
          }

          bLogTargetSet = TRUE;

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      case PARSE_MODE_LOGLEVEL:

        {
          if (!strcasecmp(pArg, "error")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;

          } else if (!strcasecmp(pArg, "warning")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_WARNING;

          } else if (!strcasecmp(pArg, "info")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_INFO;

          } else if (!strcasecmp(pArg, "verbose")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_VERBOSE;

          } else if (!strcasecmp(pArg, "debug")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_DEBUG;

          } else if (!strcasecmp(pArg, "trace")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_TRACE;

          } else {

            LSA_LOG_ERROR("Error: Invalid log level [%s]", pArg);
            ShowUsage(LsaGetProgramName(argv[0]));
            exit(1);

          }

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      }

    } while (iArg < argc);

    if (pLsaServerInfo->dwStartAsDaemon)
    {
        if (pLsaServerInfo->logTarget == LSA_LOG_TARGET_CONSOLE)
        {
            LSA_LOG_ERROR("%s", "Error: Cannot log to console when executing as a daemon");

            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        if (!bLogTargetSet)
        {
            pLsaServerInfo->logTarget = LSA_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}

PSTR
LsaGetProgramName(
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

VOID
LsaSrvExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = LsaSrvGetCachePath(&pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/lsasd.err", pszCachePath);

    dwError = LsaCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bFileExists) {
        dwError = LsaRemoveFile(szErrCodeFilePath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvGetProcessExitCode(&dwExitCode);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogProcessStoppedEvent(dwExitCode);

    if (dwExitCode) {
       fp = fopen(szErrCodeFilePath, "w");
       if (fp == NULL) {
          dwError = LwMapErrnoToLwError(errno);
          BAIL_ON_LSA_ERROR(dwError);
       }
       fprintf(fp, "%u\n", dwExitCode);
    }

error:

    LW_SAFE_FREE_STRING(pszCachePath);

    if (fp != NULL) {
       fclose(fp);
    }
}

DWORD
LsaSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = LsaInitCacheFolders();
    BAIL_ON_LSA_ERROR(dwError);

#ifdef ENABLE_STATIC_PROVIDERS
    dwError = LsaSrvApiInit(gStaticProviders);
#else
    dwError = LsaSrvApiInit(NULL);
#endif
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LsaSrvGetCachePath(&pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckDirectoryExists(
                        pszCachePath,
                        &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = LsaCreateDirectory(pszCachePath, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszCachePath);

    return dwError;

error:

    goto cleanup;
}

BOOLEAN
LsaSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    bResult = (gpServerInfo->dwStartAsDaemon != 0);

    LSA_UNLOCK_SERVERINFO(bInLock);

    return bResult;
}

DWORD
LsaSrvStartAsDaemon(
    VOID
    )
{
    DWORD dwError = 0;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    /* Use dcethread_fork() rather than fork() because we link with DCE/RPC */
    if ((pid = dcethread_fork()) != 0) {
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
    dwError = LsaSrvIgnoreSIGHUP();
    BAIL_ON_LSA_ERROR(dwError);

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_LSA_ERROR(dwError);

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
LsaSrvGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    *pdwExitCode = gpServerInfo->dwExitCode;

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;
}

VOID
LsaSrvSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    gpServerInfo->dwExitCode = dwExitCode;

    LSA_UNLOCK_SERVERINFO(bInLock);
}

DWORD
LsaSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    if (LW_IS_NULL_OR_EMPTY_STR(gpServerInfo->szCachePath)) {
      dwError = LW_ERROR_INVALID_CACHE_PATH;
      BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(gpServerInfo->szCachePath, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LW_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
LsaSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    if (LW_IS_NULL_OR_EMPTY_STR(gpServerInfo->szPrefixPath)) {
      dwError = LW_ERROR_INVALID_PREFIX_PATH;
      BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(gpServerInfo->szPrefixPath, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LW_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

#ifdef ENABLE_PIDFILE
VOID
LsaSrvCreatePIDFile(
    VOID
    )
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = LsaSrvGetPidFromPidFile();
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

static
pid_t
LsaSrvGetPidFromPidFile(
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

static
VOID
LsaSrvRemovePidFile(
    VOID
    )
{
    if (LsaSrvGetPidFromPidFile() == getpid())
    {
        unlink(PID_FILE);
    }
}

#endif

DWORD
LsaBlockSelectedSignals(
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
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

BOOLEAN
LsaSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    bExit = gpServerInfo->bProcessShouldExit;

    LSA_UNLOCK_SERVERINFO(bInLock);

    return bExit;
}

VOID
LsaSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    gpServerInfo->bProcessShouldExit = bExit;

    LSA_UNLOCK_SERVERINFO(bInLock);
}

VOID
LsaSrvLogProcessStartedEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise authentication service was started.");
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_INFO_SERVICE_STARTED,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LsaSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise authentication service was stopped");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwExitCode,
                         &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwExitCode)
    {
        LsaSrvLogServiceFailureEvent(
                LSASS_EVENT_ERROR_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }
    else
    {
        LsaSrvLogServiceSuccessEvent(
                LSASS_EVENT_INFO_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

VOID
LsaSrvLogProcessFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise authentication service stopped running due to an error");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceFailureEvent(
            LSASS_EVENT_ERROR_SERVICE_START_FAILURE,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            pszData);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

