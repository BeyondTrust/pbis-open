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
 *        Registry
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#include "registryd.h"

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;
    int ret = 0;

    dwError = RegSrvSetDefaults();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvParseArgs(argc,
        argv,
        &gServerInfo);

    BAIL_ON_REG_ERROR(dwError);

    dwError = RegInitLogging_r(
        RegGetProgramName(argv[0]),
        gServerInfo.logTarget,
        gServerInfo.maxAllowedLogLevel,
        gServerInfo.szLogFilePath);

    BAIL_ON_REG_ERROR(dwError);

    REG_LOG_VERBOSE("Logging started");

    if (atexit(RegSrvExitHandler) < 0)
    {
       dwError = errno;
       BAIL_ON_REG_ERROR(dwError);
    }

    if (RegSrvShouldStartAsDaemon())
    {
       dwError = RegSrvStartAsDaemon();
       BAIL_ON_REG_ERROR(dwError);
    }

    RegSrvCreatePIDFile();

    dwError = RegBlockSelectedSignals();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvInitialize();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvStartListenThread();
    BAIL_ON_REG_ERROR(dwError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);
        
        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            REG_LOG_ERROR("Could not notify service manager: %s (%i)", strerror(errno), errno);
            dwError = RegMapErrnoToLwRegError(errno);
            BAIL_ON_REG_ERROR(dwError);
        }

        close(notifyFd);
    }

    //RegSrvLogProcessStartedEvent();

    // Handle signals, blocking until we are supposed to exit.
    dwError = RegSrvHandleSignals();
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    REG_LOG_VERBOSE("Reg main cleaning up");

    RegSrvStopProcess();

    RegSrvStopListenThread();

    RegSrvApiShutdown();

    REG_LOG_INFO("REG Service exiting...");

    RegSrvSetProcessExitCode(dwError);

    RegShutdownLogging_r();

    return dwError;

error:

    REG_LOG_ERROR("REG Process exiting due to error [Code:%d]", dwError);

    //RegSrvLogProcessFailureEvent(dwError);

    goto cleanup;
}

DWORD
RegSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    gpServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_ERROR;

    *(gpServerInfo->szLogFilePath) = '\0';

    strcpy(gpServerInfo->szCachePath, CACHEDIR);
    strcpy(gpServerInfo->szPrefixPath, PREFIXDIR);

    setlocale(LC_ALL, "");

    return (dwError);
}

DWORD
RegSrvParseArgs(
    int argc,
    PSTR argv[],
    PREGSERVERINFO pRegServerInfo
    )
{
    typedef enum
    {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    BOOLEAN bLogTargetSet = FALSE;

    do
    {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0')
        {
            break;
        }

        switch(parseMode)
        {

        case PARSE_MODE_OPEN:
            {
                if (strcmp(pArg, "--logfile") == 0)
                {
                    parseMode = PARSE_MODE_LOGFILE;
                }
                else if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0))
                {
                    ShowUsage(RegGetProgramName(argv[0]));
                    exit(0);
                }
                else if (strcmp(pArg, "--start-as-daemon") == 0)
                {
                    pRegServerInfo->dwStartAsDaemon = 1;

                    // If other arguments set before this set the log target
                    // don't over-ride that setting
                    if (!bLogTargetSet)
                    {
                        pRegServerInfo->logTarget = REG_LOG_TARGET_SYSLOG;
                    }
                }
                else if (strcmp(pArg, "--loglevel") == 0)
                {
                    parseMode = PARSE_MODE_LOGLEVEL;
                }
                else if (strcmp(pArg, "--syslog") == 0)
                {
                    gServerInfo.logTarget = REG_LOG_TARGET_SYSLOG;
                    bLogTargetSet = TRUE;
                }
                else
                {
                    REG_LOG_ERROR("Unrecognized command line option [%s]",pArg);
                    ShowUsage(RegGetProgramName(argv[0]));
                    exit(1);
                }
                break;
            }

        case PARSE_MODE_LOGFILE:
            {
                strcpy(pRegServerInfo->szLogFilePath, pArg);

                RegStripWhitespace(pRegServerInfo->szLogFilePath, TRUE, TRUE);

                if (!strcmp(pRegServerInfo->szLogFilePath, "."))
                {
                    pRegServerInfo->logTarget = REG_LOG_TARGET_CONSOLE;
                }
                else
                {
                    pRegServerInfo->logTarget = REG_LOG_TARGET_FILE;
                }

                bLogTargetSet = TRUE;

                parseMode = PARSE_MODE_OPEN;

                break;
            }

        case PARSE_MODE_LOGLEVEL:
            {
                if (!strcasecmp(pArg, "error"))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_ERROR;
                }
                else if (!strcasecmp(pArg, "warning"))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_WARNING;
                }
                else if (!strcasecmp(pArg, "info"))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_INFO;
                }
                else if (!strcasecmp(pArg, "verbose"))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_VERBOSE;
                }
                else if (!strcasecmp(pArg, "debug"))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_DEBUG;
                }
                else if (!strcasecmp(pArg, "trace"))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_TRACE;
                }
                else
                {
                    REG_LOG_ERROR("Error: Invalid log level [%s]", pArg);
                    ShowUsage(RegGetProgramName(argv[0]));
                    exit(1);
                }

                parseMode = PARSE_MODE_OPEN;
                break;
            }
        }
    } while (iArg < argc);

    if (pRegServerInfo->dwStartAsDaemon)
    {
        if (pRegServerInfo->logTarget == REG_LOG_TARGET_CONSOLE)
        {
            REG_LOG_ERROR("%s", "Error: Cannot log to console when executing as a daemon");

            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }
    }
    else
    {
        if (pRegServerInfo->logTarget != REG_LOG_TARGET_FILE &&
            pRegServerInfo->logTarget != REG_LOG_TARGET_SYSLOG)
        {
            pRegServerInfo->logTarget = REG_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}

PSTR
RegGetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0')
    {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do
    {
        if (*(pszNameStart - 1) == '/')
        {
            break;
        }

        pszNameStart--;

    } while(pszNameStart != pszFullProgramPath);

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
           "          [--loglevel {error, warning, info, verbose, trace}]\n",
           pszProgramName);
}

VOID
RegSrvExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = RegSrvGetCachePath(&pszCachePath);
    BAIL_ON_REG_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/regsd.err", pszCachePath);

    dwError = RegCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_REG_ERROR(dwError);

    if (bFileExists)
    {
        dwError = RegRemoveFile(szErrCodeFilePath);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegSrvGetProcessExitCode(&dwExitCode);
    BAIL_ON_REG_ERROR(dwError);

    //RegSrvLogProcessStoppedEvent(dwExitCode);

    if (dwExitCode)
    {
        fp = fopen(szErrCodeFilePath, "w");
        if (fp == NULL)
        {
            dwError = errno;
            BAIL_ON_REG_ERROR(dwError);
        }
        fprintf(fp, "%d\n", dwExitCode);
    }

error:

    LWREG_SAFE_FREE_STRING(pszCachePath);

    if (fp != NULL)
    {
        fclose(fp);
    }
}

DWORD
RegSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = RegInitCacheFolders();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvApiInit();
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
RegInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    BOOLEAN bExists = FALSE;

    dwError = RegSrvGetCachePath(&pszCachePath);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCheckDirectoryExists(
        pszCachePath,
        &bExists);

    BAIL_ON_REG_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = RegCreateDirectory(pszCachePath, cacheDirMode);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    LWREG_SAFE_FREE_STRING(pszCachePath);

    return dwError;

error:

    goto cleanup;
}

BOOLEAN
RegSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    bResult = (gpServerInfo->dwStartAsDaemon != 0);

    REG_UNLOCK_SERVERINFO(bInLock);

    return bResult;
}

DWORD
RegSrvStartAsDaemon(
    VOID
    )
{
    DWORD dwError = 0;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    if ((pid = fork()) != 0)
    {
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
    dwError = RegSrvIgnoreSIGHUP();
    BAIL_ON_REG_ERROR(dwError);

    // Spawn a second child
    if ((pid = fork()) != 0)
    {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_REG_ERROR(dwError);

    // Clear our file mode creation mask
    umask(0);

    for (iFd = 0; iFd < 3; iFd++)
    {
        close(iFd);
    }

    for (iFd = 0; iFd < 3; iFd++)
    {
        fd = open("/dev/null", O_RDWR, 0);
        if (fd < 0)
        {
            fd = open("/dev/null", O_WRONLY, 0);
        }
        if (fd < 0)
        {
            exit(1);
        }
        if (fd != iFd)
        {
            exit(1);
        }
    }

    return (dwError);

 error:

    return (dwError);
}

DWORD
RegSrvGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    *pdwExitCode = gpServerInfo->dwExitCode;

    REG_UNLOCK_SERVERINFO(bInLock);

    return dwError;
}

VOID
RegSrvSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    gpServerInfo->dwExitCode = dwExitCode;

    REG_UNLOCK_SERVERINFO(bInLock);
}

DWORD
RegSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpServerInfo->szCachePath))
    {
        dwError = LWREG_ERROR_INVALID_CACHE_PATH;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwRtlCStringDuplicate(&pszPath, gpServerInfo->szCachePath);
    BAIL_ON_REG_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    REG_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LWREG_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
RegSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpServerInfo->szPrefixPath))
    {
        dwError = LWREG_ERROR_INVALID_PREFIX_PATH;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwRtlCStringDuplicate(&pszPath, gpServerInfo->szPrefixPath);
    BAIL_ON_REG_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    REG_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LWREG_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

VOID
RegSrvCreatePIDFile(
    VOID
    )
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = RegSrvGetPidFromPidFile();
    if (pid > 0)
    {
        fprintf(stderr, "Daemon already running as %d\n", (int) pid);
        result = -1;
        goto error;
    }

    fd = open(PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "Could not create pid file: %s\n", strerror(errno));
        result = -1;
        goto error;
    }

    pid = getpid();
    snprintf(contents, sizeof(contents)-1, "%d\n", (int) pid);
    contents[sizeof(contents)-1] = 0;
    len = strlen(contents);

    result = (int) write(fd, contents, len);
    if ( result != (int) len )
    {
        fprintf(stderr, "Could not write to pid file: %s\n", strerror(errno));
        result = -1;
        goto error;
    }

    result = 0;

 error:
    if (fd != -1)
    {
        close(fd);
    }

    if (result < 0)
    {
        exit(1);
    }
}

pid_t
RegSrvGetPidFromPidFile(
    VOID
    )
{
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[PID_FILE_CONTENTS_SIZE];

    fd = open(PID_FILE, O_RDONLY, 0644);
    if (fd < 0)
    {
        goto error;
    }

    result = read(fd, contents, sizeof(contents)-1);
    if (result <= 0)
    {
        goto error;
    }
    contents[result-1] = 0;

    result = atoi(contents);
    if (result <= 0)
    {
        result = -1;
        goto error;
    }

    pid = (pid_t) result;
    result = kill(pid, 0);
    if (result != 0 || errno == ESRCH)
    {
        unlink(PID_FILE);
        pid = 0;
    }

 error:
    if (fd != -1)
    {
        close(fd);
    }

    return pid;
}

DWORD
RegBlockSelectedSignals(
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
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

BOOLEAN
RegSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    bExit = gpServerInfo->bProcessShouldExit;

    REG_UNLOCK_SERVERINFO(bInLock);

    return bExit;
}

VOID
RegSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    gpServerInfo->bProcessShouldExit = bExit;

    REG_UNLOCK_SERVERINFO(bInLock);
}

#if 0
VOID
RegSrvLogProcessStartedEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
        &pszDescription,
        "The Likewise authentication service was started."
        );

    BAIL_ON_REG_ERROR(dwError);

    RegSrvLogServiceSuccessEvent(
        REG_EVENT_INFO_SERVICE_STARTED,
        SERVICE_EVENT_CATEGORY,
        pszDescription,
        NULL
        );

cleanup:

    LWREG_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
RegSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
        &pszDescription,
        "The Likewise authentication service was stopped"
        );

    BAIL_ON_REG_ERROR(dwError);

    dwError = RegGetErrorMessageForLoggingEvent(
        dwExitCode,
        &pszData
        );

    BAIL_ON_REG_ERROR(dwError);

    if (dwExitCode)
    {
        RegSrvLogServiceFailureEvent(
            REG_EVENT_ERROR_SERVICE_STOPPED,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            pszData
            );
    }
    else
    {
        RegSrvLogServiceSuccessEvent(
            REG_EVENT_INFO_SERVICE_STOPPED,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            pszData
            );
    }

cleanup:

    LWREG_SAFE_FREE_STRING(pszDescription);
    LWREG_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

VOID
RegSrvLogProcessFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
        &pszDescription,
        "The Likewise authentication service stopped running due to an error"
        );

    BAIL_ON_REG_ERROR(dwError);

    dwError = RegGetErrorMessageForLoggingEvent(
        dwErrCode,
        &pszData
        );

    BAIL_ON_REG_ERROR(dwError);

    RegSrvLogServiceFailureEvent(
        REGSS_EVENT_ERROR_SERVICE_START_FAILURE,
        SERVICE_EVENT_CATEGORY,
        pszDescription,
        pszData
        );

cleanup:

    LWREG_SAFE_FREE_STRING(pszDescription);
    LWREG_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}
#endif
