/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (LWNFSSVC)
 *
 */

#include "includes.h"

static
DWORD
NfsSvcSetServerDefaults(
    VOID
    );

static
DWORD
NfsSvcParseArgs(
    int                     argc,
    PSTR                    argv[],
    PNFSSVC_RUNTIME_GLOBALS pServerInfo
    );

static
void
ShowUsage(
    PCSTR pszProgramName
    );

static
PSTR
get_program_name(
    PSTR pszFullProgramPath
    );

static
VOID
NfsSvcExitHandler(
    VOID
    );

static
BOOLEAN
NfsSvcProcessShouldExit(
    VOID
    );

static
DWORD
NfsSvcGetProcessExitCode(
    PDWORD pdwExitCode
    );

static
VOID
NfsSvcSetProcessExitCode(
    DWORD dwExitCode
    );

static
DWORD
NfsSvcStartAsDaemon(
    VOID
    );

static
pid_t
pid_from_pid_file(
    VOID
    );

static
VOID
NfsSvcCreatePIDFile(
    VOID
    );

static
DWORD
NfsSvcRpcInitialize(
    VOID
    );

static
VOID
NfsSvcRpcShutdown(
    VOID
    );

static
DWORD
NfsSvcDSNotify(
    VOID
    );

static
VOID
NfsSvcDSShutdown(
    VOID
    );

static
DWORD
NfsSvcSMNotify(
    VOID
    );

int
main(
    int argc,
    char* argv[])
{
    DWORD dwError = 0;

    dwError = NfsSvcSetServerDefaults();
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcParseArgs(argc, argv, &gServerInfo);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcInitLogging_r(
                    get_program_name(argv[0]),
                    gServerInfo.logTarget,
                    gServerInfo.maxAllowedLogLevel,
                    gServerInfo.szLogFilePath);
    BAIL_ON_NFSSVC_ERROR(dwError);

    NFSSVC_LOG_VERBOSE("Logging Started");

    if (atexit(NfsSvcExitHandler) < 0)
    {
        dwError = errno;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    if (gServerInfo.dwStartAsDaemon)
    {
        dwError = NfsSvcStartAsDaemon();
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    NfsSvcCreatePIDFile();

    dwError = NfsSvcReadConfigSettings();
    BAIL_ON_NFSSVC_ERROR(dwError);

    NfsSvcBlockSelectedSignals();

    dwError = NfsSvcRpcInitialize();
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcDSNotify();
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcSMNotify();
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcInitSecurity();
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcHandleSignals();
    BAIL_ON_NFSSVC_ERROR(dwError);

 cleanup:

    NFSSVC_LOG_INFO("Likewise Server Service exiting...");

    NfsSvcSetProcessShouldExit(TRUE);

    NfsSvcDSShutdown();

    NfsSvcRpcShutdown();

    NfsSvcShutdownLogging_r();

    NfsSvcSetProcessExitCode(dwError);

    exit (dwError);

error:

    NFSSVC_LOG_ERROR("Likewise Server Service exiting due to error [code:%d]",
                        dwError);

    goto cleanup;
}

static
DWORD
NfsSvcSetServerDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    setlocale(LC_ALL, "");

    dwError = NfsSvcReadConfigSettings();

    return dwError;
}

static
DWORD
NfsSvcParseArgs(
    int                     argc,
    PSTR                    argv[],
    PNFSSVC_RUNTIME_GLOBALS pServerInfo
    )
{
    DWORD dwError = 0;

    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    BOOLEAN bLogTargetSet = FALSE;

    do
    {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
            break;
        }

        switch(parseMode)
        {
            case PARSE_MODE_OPEN:

                if (strcmp(pArg, "--logfile") == 0)
                {
                    parseMode = PARSE_MODE_LOGFILE;
                }
                else if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0))
                {
                    ShowUsage(get_program_name(argv[0]));
                    exit(0);
                }
                else if (strcmp(pArg, "--start-as-daemon") == 0)
                {
                    pServerInfo->dwStartAsDaemon = 1;

                    // If other arguments set before this set the log target
                    // don't over-ride that setting
                    if (!bLogTargetSet)
                    {
                        pServerInfo->logTarget = NFSSVC_LOG_TARGET_SYSLOG;
                    }
                }
                else if (strcmp(pArg, "--syslog") == 0)
                {
                    bLogTargetSet = TRUE;
                    pServerInfo->logTarget = NFSSVC_LOG_TARGET_SYSLOG;
                }
                else if (strcmp(pArg, "--loglevel") == 0)
                {
                    parseMode = PARSE_MODE_LOGLEVEL;
                }
                else
                {
                    NFSSVC_LOG_ERROR("Unrecognized command line option [%s]",
                                    pArg);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }

                break;

            case PARSE_MODE_LOGFILE:

                strcpy(pServerInfo->szLogFilePath, pArg);

                NfsSvcStripWhitespace(pServerInfo->szLogFilePath, TRUE, TRUE);

                if (!strcmp(pServerInfo->szLogFilePath, "."))
                {
                    pServerInfo->logTarget = NFSSVC_LOG_TARGET_CONSOLE;
                }
                else
                {
                    pServerInfo->logTarget = NFSSVC_LOG_TARGET_FILE;
                }

                bLogTargetSet = TRUE;

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_LOGLEVEL:

                if (!strcasecmp(pArg, "error"))
                {
                    pServerInfo->maxAllowedLogLevel = NFSSVC_LOG_LEVEL_ERROR;
                }
                else if (!strcasecmp(pArg, "warning"))
                {
                    pServerInfo->maxAllowedLogLevel = NFSSVC_LOG_LEVEL_WARNING;
                }
                else if (!strcasecmp(pArg, "info"))
                {
                    pServerInfo->maxAllowedLogLevel = NFSSVC_LOG_LEVEL_INFO;
                }
                else if (!strcasecmp(pArg, "verbose"))
                {
                    pServerInfo->maxAllowedLogLevel = NFSSVC_LOG_LEVEL_VERBOSE;
                }
                else if (!strcasecmp(pArg, "debug"))
                {
                    pServerInfo->maxAllowedLogLevel = NFSSVC_LOG_LEVEL_DEBUG;
                }
                else
                {
                    NFSSVC_LOG_ERROR("Error: Invalid log level [%s]", pArg);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }

                parseMode = PARSE_MODE_OPEN;

                break;
        }
    } while (iArg < argc);

    if (pServerInfo->dwStartAsDaemon)
    {
        if (pServerInfo->logTarget == NFSSVC_LOG_TARGET_CONSOLE)
        {
            NFSSVC_LOG_ERROR("%s", "Error: Cannot log to console when executing as a daemon");

            dwError = NFSSVC_ERROR_INVALID_PARAMETER;
            BAIL_ON_NFSSVC_ERROR(dwError);
        }
    }
    else
    {
        if (!bLogTargetSet)
        {
            pServerInfo->logTarget = NFSSVC_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}

static
void
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
            "          [--syslog]\n"
            "          [--logfile logFilePath]\n"
            "          [--loglevel {error, warning, info, verbose, debug}]\n",
            pszProgramName);
}

static
PSTR
get_program_name(
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
NfsSvcExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    CHAR  szCachePath[] = CACHEDIR;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    sprintf(szErrCodeFilePath, "%s/nfssvcd.err", szCachePath);

    dwError = NfsSvcCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (bFileExists) {
        dwError = NfsSvcRemoveFile(szErrCodeFilePath);
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    dwError = NfsSvcGetProcessExitCode(&dwExitCode);
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (dwExitCode) {
        fp = fopen(szErrCodeFilePath, "w");
        if (fp == NULL) {
            dwError = errno;
            BAIL_ON_NFSSVC_ERROR(dwError);
        }
        fprintf(fp, "%d\n", dwExitCode);
    }

error:

    if (fp != NULL) {
        fclose(fp);
    }
}

static
BOOLEAN
NfsSvcProcessShouldExit(
    VOID
    )
{
    BOOLEAN bResult = 0;
    BOOLEAN bInLock = FALSE;

    NFSSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    bResult = gServerInfo.bProcessShouldExit;

    NFSSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return bResult;
}

VOID
NfsSvcSetProcessShouldExit(
    BOOLEAN val
    )
{
    BOOLEAN bInLock = FALSE;

    NFSSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    gServerInfo.bProcessShouldExit = val;

    NFSSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);
}

static
DWORD
NfsSvcGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    NFSSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    *pdwExitCode = gServerInfo.dwExitCode;

    NFSSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return (dwError);
}

static
VOID
NfsSvcSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;

    NFSSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    gServerInfo.dwExitCode = dwExitCode;

    NFSSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);
}

static
DWORD
NfsSvcStartAsDaemon(
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
    if (signal(SIGHUP, SIG_IGN) < 0) {
        dwError = errno;
        BAIL_ON_NFSSVC_ERROR(dwError);
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
    BAIL_ON_NFSSVC_ERROR(dwError);

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

static
pid_t
pid_from_pid_file(
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
NfsSvcCreatePIDFile(
    VOID
    )
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = pid_from_pid_file();
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
DWORD
NfsSvcRpcInitialize(
    VOID
    )
{
    DWORD    dwError        = 0;
    DWORD    dwBindAttempts = 0;
    BOOLEAN  bInLock        = FALSE;
    static const DWORD dwMaxBindAttempts  = 5;
    static const DWORD dwBindSleepSeconds = 5;

    NFSSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    dwError = LwNtStatusToWin32Error(LsaRpcInitMemory());
    BAIL_ON_NT_STATUS(dwError);

    /* Binding to our RPC end-point might fail if dcerpcd is not
       yet ready when we start, so attempt it in a loop with
       a small delay between attempts */
    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = NfsSvcRegisterForRPC(
                        "Likewise Server Service",
                        &gServerInfo.pServerBinding);
        if (dwError)
        {
            NFSSVC_LOG_INFO("Failed to bind nfssvc endpoint; retrying in "
                            "%i seconds...",
                            (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_NFSSVC_ERROR(dwError);

    /* Now register the wkssvc pipe */

    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = WksSvcRegisterForRPC(
                        "Likewise Workstation Service",
                        &gServerInfo.pWkstaBinding);
        if (dwError)
        {
            NFSSVC_LOG_INFO("Failed to bind wkssvc endpoint; retrying in "
                            "%i seconds...",
                            (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_NFSSVC_ERROR(dwError);

    /* Now register the wkssvc pipe */

    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = WinRegRegisterForRPC(
                        "Likewise Registry Service",
                        &gServerInfo.pRegistryBinding);
        if (dwError)
        {
            NFSSVC_LOG_INFO("Failed to bind wkssvc endpoint; retrying in "
                            "%i seconds...",
                            (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwMapErrnoToLwError(dcethread_create(
                                        &gServerInfo.pRpcListenerThread,
                                        NULL,
                                        &NfsSvcListenForRPC,
                                        NULL));
    BAIL_ON_NFSSVC_ERROR(dwError);

    while (!NfsSvcRpcIsListening())
    {
    }

cleanup:

    NFSSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return dwError;

error:

    goto cleanup;
}

static
VOID
NfsSvcRpcShutdown(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    NFSSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    if (gServerInfo.pServerBinding)
    {
        dwError = NfsSvcUnregisterForRPC(gServerInfo.pServerBinding);
        BAIL_ON_NFSSVC_ERROR(dwError);

        gServerInfo.pServerBinding = NULL;
    }

    if (gServerInfo.pWkstaBinding)
    {
        dwError = WksSvcUnregisterForRPC(gServerInfo.pWkstaBinding);
        BAIL_ON_NFSSVC_ERROR(dwError);

        gServerInfo.pWkstaBinding = NULL;
    }

    if (gServerInfo.pRegistryBinding)
    {
        dwError = WinRegUnregisterForRPC(gServerInfo.pRegistryBinding);
        BAIL_ON_NFSSVC_ERROR(dwError);

        gServerInfo.pRegistryBinding = NULL;
    }

    NFSSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    dwError = NfsSvcRpcStopListening();
    BAIL_ON_NFSSVC_ERROR(dwError);

    NFSSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    if (gServerInfo.pRpcListenerThread)
    {
        dwError = LwMapErrnoToLwError(dcethread_interrupt(gServerInfo.pRpcListenerThread));
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = LwMapErrnoToLwError(dcethread_join(gServerInfo.pRpcListenerThread, NULL));
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

cleanup:

    NFSSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return;

error:

    goto cleanup;
}

static
DWORD
NfsSvcDSNotify(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = LwDsCacheAddPidException(getpid());
    if (dwError == LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK)
    {
        NFSSVC_LOG_ERROR(   "Could not register process pid (%d) "
                            "with Mac DirectoryService Cache plugin",
                            (int) getpid());
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

error:

    return dwError;
}

static
VOID
NfsSvcDSShutdown(
    VOID
    )
{
    LwDsCacheRemovePidException(getpid());
}

static
DWORD
NfsSvcSMNotify(
    VOID
    )
{
    DWORD dwError     = 0;
    PCSTR pszSmNotify = NULL;
    int   notifyFd    = -1;

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        int  ret        = 0;
        char notifyCode = 0;

        notifyFd = atoi(pszSmNotify);

        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            NFSSVC_LOG_ERROR("Could not notify service manager: %s (%i)",
                                strerror(errno),
                                errno);

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_NFSSVC_ERROR(dwError);
        }
    }

cleanup:

    if (notifyFd >= 0)
    {
        close(notifyFd);
    }

    return dwError;

error:

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
