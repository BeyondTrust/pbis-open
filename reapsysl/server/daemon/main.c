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
 *        Reaper for syslog
 * 
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

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

    dwError = RSysSrvInitialize();
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = RSysSrvParseArgs(argc, argv, &gServerInfo);
    BAIL_ON_RSYS_ERROR(dwError);

    RSYS_LOG_VERBOSE("Logging started");

    if (atexit(RSysSrvExitHandler) < 0)
    {
       dwError = errno;
       BAIL_ON_RSYS_ERROR(dwError);
    }

    if (RSysSrvShouldStartAsDaemon())
    {
       dwError = RSysSrvStartAsDaemon();
       BAIL_ON_RSYS_ERROR(dwError);
    }

    // Test system to see if dependent configuration tasks are completed prior to starting our process.
    dwError = RSysStartupPreCheck();
    BAIL_ON_RSYS_ERROR(dwError);

    // ISSUE-2008/07/03-dalmeida -- Should return/check for errors
    RSysSrvCreatePIDFile();

    dwError = RSysBlockSelectedSignals();
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = RSysSrvStartReaderThread();
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = RSysSrvStartListenThread();
    BAIL_ON_RSYS_ERROR(dwError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);
        
        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            RSYS_LOG_ERROR("Could not notify service manager: %s (%i)", strerror(errno), errno);
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }

        close(notifyFd);
    }

    // Handle signals, blocking until we are supposed to exit.
    dwError = RSysSrvHandleSignals();
    BAIL_ON_RSYS_ERROR(dwError);

 cleanup:

    RSYS_LOG_VERBOSE("Shutting down threads");

    RSysSrvStopProcess();

    RSysSrvStopListenThread();

    RSysSrvStopReaderThread();

    RSysSrvApiShutdown();

    RSYS_LOG_INFO("Reapsysl Service exiting...");

    RSysCloseGlobalLog();

    RSysSrvSetProcessExitCode(dwError);

    return dwError;

 error:

    RSYS_LOG_ERROR("Reapsysl Process exiting due to error [Code:%d]", dwError);

    goto cleanup;
}

DWORD
RSysStartupPreCheck(
    VOID
    )
{
    DWORD dwError = 0;
    return dwError;
}

DWORD
RSysSrvParseArgs(
    int argc,
    PCSTR argv[],
    PRSYS_SERVERINFO pRSysServerInfo
    )
{
    int iArg = 0;
    BOOLEAN bShowUsage = FALSE;
    BOOLEAN bError = FALSE;
    PRSYS_LOG_INFO pLogInfo = NULL;
    DWORD dwError = 0;

    dwError = RSysSrvGetLogInfo(
                    NULL,
                    &pLogInfo);
    BAIL_ON_RSYS_ERROR(dwError);

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
            if (pLogInfo->logTarget == RSYS_LOG_TARGET_CONSOLE)
            {
                pLogInfo->logTarget = RSYS_LOG_TARGET_SYSLOG;
            }
            pRSysServerInfo->dwStartAsDaemon = 1;
        }
        else if (!strcmp(pArg, "--logfile"))
        {
            if (iArg + 1 >= argc)
            {
                fprintf(stderr, "Missing required argument for %s option.\n", pArg);
                bError = TRUE;
                break;
            }
            pArg = argv[++iArg];

            pLogInfo->logTarget = RSYS_LOG_TARGET_FILE;
            RtlCStringFree(&pLogInfo->pszPath);
            dwError = RtlCStringDuplicate(
                            &pLogInfo->pszPath,
                            pArg);
            BAIL_ON_RSYS_ERROR(dwError);
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
                pLogInfo->maxAllowedLogLevel = RSYS_LOG_LEVEL_ERROR;
            }
            else if (!strcasecmp(pArg, "warning"))
            {
                pLogInfo->maxAllowedLogLevel = RSYS_LOG_LEVEL_WARNING;
            }
            else if (!strcasecmp(pArg, "info"))
            {
                pLogInfo->maxAllowedLogLevel = RSYS_LOG_LEVEL_INFO;
            }
            else if (!strcasecmp(pArg, "verbose"))
            {
                pLogInfo->maxAllowedLogLevel = RSYS_LOG_LEVEL_VERBOSE;
            }
            else if (!strcasecmp(pArg, "debug"))
            {
                pLogInfo->maxAllowedLogLevel = RSYS_LOG_LEVEL_DEBUG;
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
        ShowUsage(RSysGetProgramName(argv[0]));
        exit(bError ? 1 : 0);
    }

    dwError = RSysSrvSetLogInfo(
                    NULL,
                    pLogInfo);
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:
    if (pLogInfo)
    {
        RSysFreeLogInfo(pLogInfo);
    }
    return dwError;

error:
    goto cleanup;
}

PCSTR
RSysGetProgramName(
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
           "    --logfile <logFilePath>   log to specified file\n"
           "    --loglevel <logLevel>     log at the specified detail level, which\n"
           "                              can be one of:\n"
           "                                error, warning, info, verbose, debug.\n"
           "", pszProgramName);
}

VOID
RSysSrvExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = RSysSrvGetCachePath(&pszCachePath);
    BAIL_ON_RSYS_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/reapsysl.err", pszCachePath);

    dwError = RSysCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_RSYS_ERROR(dwError);

    if (bFileExists) {
        dwError = RSysRemoveFile(szErrCodeFilePath);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    dwError = RSysSrvGetProcessExitCode(&dwExitCode);
    BAIL_ON_RSYS_ERROR(dwError);

    if (dwExitCode) {
       fp = fopen(szErrCodeFilePath, "w");
       if (fp == NULL) {
          dwError = errno;
          BAIL_ON_RSYS_ERROR(dwError);
       }
       fprintf(fp, "%d\n", dwExitCode);
    }

error:

    RtlCStringFree(&pszCachePath);

    if (fp != NULL) {
       fclose(fp);
    }
}

DWORD
RSysSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    
    dwError = RSysSrvApiInit();
    BAIL_ON_RSYS_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

BOOLEAN
RSysSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    RSYS_LOCK_SERVERINFO(bInLock);

    bResult = (gpServerInfo->dwStartAsDaemon != 0);

    RSYS_UNLOCK_SERVERINFO(bInLock);

    return bResult;
}

DWORD
RSysSrvStartAsDaemon(
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
        BAIL_ON_RSYS_ERROR(dwError);
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
    BAIL_ON_RSYS_ERROR(dwError);

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
RSysSrvGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    RSYS_LOCK_SERVERINFO(bInLock);

    *pdwExitCode = gpServerInfo->dwExitCode;

    RSYS_UNLOCK_SERVERINFO(bInLock);

    return dwError;
}

VOID
RSysSrvSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;
    
    RSYS_LOCK_SERVERINFO(bInLock);

    gpServerInfo->dwExitCode = dwExitCode;

    RSYS_UNLOCK_SERVERINFO(bInLock);
}

DWORD
RSysSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;
  
    RSYS_LOCK_SERVERINFO(bInLock);
    
    if (IsNullOrEmptyString(gpServerInfo->szCachePath)) {
      dwError = RSYS_ERROR_INVALID_CACHE_PATH;
      BAIL_ON_RSYS_ERROR(dwError);
    }
    
    dwError = RtlCStringDuplicate(&pszPath, gpServerInfo->szCachePath);
    BAIL_ON_RSYS_ERROR(dwError);

    *ppszPath = pszPath;
    
 cleanup:

    RSYS_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    RtlCStringFree(&pszPath);
    
    *ppszPath = NULL;

    goto cleanup;
}

DWORD
RSysSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;
  
    RSYS_LOCK_SERVERINFO(bInLock);
    
    if (IsNullOrEmptyString(gpServerInfo->szPrefixPath)) {
      dwError = RSYS_ERROR_INVALID_PREFIX_PATH;
      BAIL_ON_RSYS_ERROR(dwError);
    }
    
    dwError = RtlCStringDuplicate(&pszPath, gpServerInfo->szPrefixPath);
    BAIL_ON_RSYS_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:
    
    RSYS_UNLOCK_SERVERINFO(bInLock);
    
    return dwError;

 error:

    RtlCStringFree(&pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

VOID
RSysSrvCreatePIDFile(
    VOID
    )
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = RSysSrvGetPidFromPidFile();
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
RSysSrvGetPidFromPidFile(
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

VOID
RSysClearAllSignals(
    VOID
    )
{
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;
   
    sigemptyset(&default_signal_mask);
    pthread_sigmask(SIG_SETMASK,  &default_signal_mask, &old_signal_mask);
}

DWORD
RSysBlockSelectedSignals(
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
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

BOOLEAN
RSysSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    RSYS_LOCK_SERVERINFO(bInLock);
    
    bExit = gpServerInfo->bProcessShouldExit;

    RSYS_UNLOCK_SERVERINFO(bInLock);
    
    return bExit;
}

VOID
RSysSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;
    
    RSYS_LOCK_SERVERINFO(bInLock);

    gpServerInfo->bProcessShouldExit = bExit;

    RSYS_UNLOCK_SERVERINFO(bInLock);
}
